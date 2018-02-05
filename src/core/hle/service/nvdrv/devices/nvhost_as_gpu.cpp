// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/hle/service/nvdrv/devices/nvhost_as_gpu.h"

namespace Service {
namespace Nvidia {
namespace Devices {

u32 nvhost_as_gpu::ioctl(u32 command, const std::vector<u8>& input, std::vector<u8>& output) {
    LOG_DEBUG(Service_NVDRV, "Got Ioctl 0x%x, inputsz: 0x%x, outputsz: 0x%x", command, input.size(),
              output.size());

    switch (static_cast<IoctlCommand>(command)) {
    case IoctlCommand::IocInitalizeExCommand:
        return InitalizeEx(input, output);
    case IoctlCommand::IocAllocateSpaceCommand:
        return AllocateSpace(input, output);
    case IoctlCommand::IocMapBufferExCommand:
        return MapBufferEx(input, output);
    case IoctlCommand::IocBindChannelCommand:
        return BindChannel(input, output);
    case IoctlCommand::IocGetVaRegionsCommand:
        return GetVARegions(input, output);
    }
    return 0;
}

u32 nvhost_as_gpu::InitalizeEx(const std::vector<u8>& input, std::vector<u8>& output) {
    initalize_ex params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_WARNING(Service_NVDRV, "(STUBBED) called, big_page_size=0x%x", params.big_page_size);
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_as_gpu::AllocateSpace(const std::vector<u8>& input, std::vector<u8>& output) {
    alloc_space params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_WARNING(Service_NVDRV, "(STUBBED) called, pages=%x, page_size=%x, flags=%x", params.pages,
                params.page_size, params.flags);
    params.offset = 0xdeadbeef; // TODO(ogniK): Actually allocate space and give a real offset
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_as_gpu::MapBufferEx(const std::vector<u8>& input, std::vector<u8>& output) {
    map_buffer_ex params{};
    std::memcpy(&params, input.data(), input.size());

    LOG_WARNING(Service_NVDRV,
                "(STUBBED) called, flags=%x, nvmap_handle=%x, buffer_offset=%lx, mapping_size=%lx, "
                "offset=%lx",
                params.flags, params.nvmap_handle, params.buffer_offset, params.mapping_size,
                params.offset);
    params.offset = 0x0; // TODO(ogniK): Actually map and give a real offset
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_as_gpu::BindChannel(const std::vector<u8>& input, std::vector<u8>& output) {
    bind_channel params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_DEBUG(Service_NVDRV, "called, fd=%x", params.fd);
    channel = params.fd;
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_as_gpu::GetVARegions(const std::vector<u8>& input, std::vector<u8>& output) {
    get_va_regions params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_WARNING(Service, "(STUBBED) called, buf_addr=%lx, buf_size=%x", params.buf_addr,
                params.buf_size);

    params.buf_size = 0x30;
    params.regions[0].offset = 0x04000000;
    params.regions[0].page_size = 0x1000;
    params.regions[0].pages = 0x3fbfff;

    params.regions[1].offset = 0x04000000;
    params.regions[1].page_size = 0x10000;
    params.regions[1].pages = 0x1bffff;
    // TODO(ogniK): This probably can stay stubbed but should add support way way later
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

} // namespace Devices
} // namespace Nvidia
} // namespace Service
