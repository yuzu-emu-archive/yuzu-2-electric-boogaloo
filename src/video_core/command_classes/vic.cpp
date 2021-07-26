// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>

extern "C" {
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
#include <libswscale/swscale.h>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
}

#include "common/assert.h"
#include "common/logging/log.h"

#include "video_core/command_classes/nvdec.h"
#include "video_core/command_classes/vic.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/gpu.h"
#include "video_core/memory_manager.h"
#include "video_core/textures/decoders.h"

namespace Tegra {

Vic::Vic(GPU& gpu_, std::shared_ptr<Nvdec> nvdec_processor_)
    : gpu(gpu_),
      nvdec_processor(std::move(nvdec_processor_)), converted_frame_buffer{nullptr, av_free} {}

Vic::~Vic() = default;

void Vic::ProcessMethod(Method method, u32 argument) {
    LOG_DEBUG(HW_GPU, "Vic method 0x{:X}", static_cast<u32>(method));
    const u64 arg = static_cast<u64>(argument) << 8;
    switch (method) {
    case Method::Execute:
        Execute();
        break;
    case Method::SetConfigStructOffset:
        config_struct_address = arg;
        break;
    case Method::SetOutputSurfaceLumaOffset:
        output_surface_luma_address = arg;
        break;
    case Method::SetOutputSurfaceChromaUOffset:
        output_surface_chroma_u_address = arg;
        break;
    case Method::SetOutputSurfaceChromaVOffset:
        output_surface_chroma_v_address = arg;
        break;
    default:
        break;
    }
}

void Vic::Execute() {
    if (output_surface_luma_address == 0) {
        LOG_ERROR(Service_NVDRV, "VIC Luma address not set.");
        return;
    }
    const VicConfig config{gpu.MemoryManager().Read<u64>(config_struct_address + 0x20)};
    const AVFramePtr frame_ptr = nvdec_processor->GetFrame();
    const auto* frame = frame_ptr.get();
    if (!frame || frame->width == 0 || frame->height == 0) {
        return;
    }
    switch (frame->format) {
    case AV_PIX_FMT_YUV420P:
    case AV_PIX_FMT_NV12:
        break;
    default:
        UNIMPLEMENTED_MSG("Unknown video format from host graphics: {}", frame->format);
        return;
    }
    const VideoPixelFormat pixel_format =
        static_cast<VideoPixelFormat>(config.pixel_format.Value());
    switch (pixel_format) {
    case VideoPixelFormat::BGRA8:
    case VideoPixelFormat::RGBA8: {
        LOG_TRACE(Service_NVDRV, "Writing RGB Frame");

        if (scaler_ctx == nullptr || frame->width != scaler_width ||
            frame->height != scaler_height) {
            const AVPixelFormat target_format =
                (pixel_format == VideoPixelFormat::RGBA8) ? AV_PIX_FMT_RGBA : AV_PIX_FMT_BGRA;

            sws_freeContext(scaler_ctx);
            scaler_ctx = nullptr;

            // Software return YUV420 and VA-API returns NV12
            // Convert it to RGB
            scaler_ctx = sws_getContext(frame->width, frame->height,
                                        static_cast<AVPixelFormat>(frame->format), frame->width,
                                        frame->height, target_format, 0, nullptr, nullptr, nullptr);

            scaler_width = frame->width;
            scaler_height = frame->height;
        }
        // Get Converted frame
        const std::size_t linear_size = frame->width * frame->height * 4;

        // Only allocate frame_buffer once per stream, as the size is not expected to change
        if (!converted_frame_buffer) {
            converted_frame_buffer = AVMallocPtr{static_cast<u8*>(av_malloc(linear_size)), av_free};
        }

        const int converted_stride{frame->width * 4};
        u8* const converted_frame_buf_addr{converted_frame_buffer.get()};

        sws_scale(scaler_ctx, frame->data, frame->linesize, 0, frame->height,
                  &converted_frame_buf_addr, &converted_stride);

        const u32 blk_kind = static_cast<u32>(config.block_linear_kind);
        if (blk_kind != 0) {
            // swizzle pitch linear to block linear
            const u32 block_height = static_cast<u32>(config.block_linear_height_log2);
            const auto size = Tegra::Texture::CalculateSize(true, 4, frame->width, frame->height, 1,
                                                            block_height, 0);
            luma_buffer.resize(size);
            Tegra::Texture::SwizzleSubrect(frame->width, frame->height, frame->width * 4,
                                           frame->width, 4, luma_buffer.data(),
                                           converted_frame_buffer.get(), block_height, 0, 0);

            gpu.MemoryManager().WriteBlock(output_surface_luma_address, luma_buffer.data(), size);
        } else {
            // send pitch linear frame
            gpu.MemoryManager().WriteBlock(output_surface_luma_address, converted_frame_buf_addr,
                                           linear_size);
        }
        break;
    }
    case VideoPixelFormat::Yuv420: {
        LOG_TRACE(Service_NVDRV, "Writing YUV420 Frame");

        const std::size_t surface_width = config.surface_width_minus1 + 1;
        const std::size_t surface_height = config.surface_height_minus1 + 1;
        const auto frame_width = std::min(surface_width, static_cast<size_t>(frame->width));
        const auto frame_height = std::min(surface_height, static_cast<size_t>(frame->height));
        const std::size_t aligned_width = (surface_width + 0xff) & ~0xff;

        const auto stride = static_cast<size_t>(frame->linesize[0]);

        luma_buffer.resize(aligned_width * surface_height);
        chroma_buffer.resize(aligned_width * surface_height / 2);

        // Populate luma buffer
        {
            const u8* src = frame->data[0];
            u8* dst = luma_buffer.data();
            for (std::size_t y = frame_height; y--;) {
                memcpy(dst, src, frame_width);
                src += stride;
                dst += aligned_width;
            }
        }
        gpu.MemoryManager().WriteBlock(output_surface_luma_address, luma_buffer.data(),
                                       luma_buffer.size());

        // Chroma
        {
            const std::size_t half_height = frame_height / 2;
            const auto half_stride = static_cast<size_t>(frame->linesize[1]);

            if (frame->format == AV_PIX_FMT_YUV420P) {
                // Frame from FFmpeg software
                // Populate chroma buffer from both channels with interleaving.
                const std::size_t half_width = frame_width / 2;
                const u8* src_r = &frame->data[2][half_height * half_stride];
                const u8* src_b = &frame->data[1][half_height * half_stride];
                u8* dst = &chroma_buffer[half_height * aligned_width];
                for (std::size_t y = half_height; y--;) {
                    dst -= aligned_width;
                    src_r -= half_stride;
                    src_b -= half_stride;
                    for (std::size_t x = half_width; x--;) {
                        dst[x * 2 + 1] = src_r[x];
                        dst[x * 2] = src_b[x];
                    }
                }
            } else {
                // Frame from VA-API hardware
                // This is already interleaved so just copy
                const u8* src = frame->data[1];
                u8* dst = chroma_buffer.data();
                for (std::size_t y = half_height; y--;) {
                    memcpy(dst, src, frame_width);
                    src += half_stride;
                    dst += aligned_width;
                }
            }
        }
        gpu.MemoryManager().WriteBlock(output_surface_chroma_u_address, chroma_buffer.data(),
                                       chroma_buffer.size());
        break;
    }
    default:
        UNIMPLEMENTED_MSG("Unknown video pixel format {}", config.pixel_format.Value());
        break;
    }
}

} // namespace Tegra
