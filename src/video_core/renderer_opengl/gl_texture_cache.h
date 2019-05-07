// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <functional>
#include <utility>
#include <vector>

#include <glad/glad.h>

#include "common/common_types.h"
#include "video_core/engines/shader_bytecode.h"
#include "video_core/renderer_opengl/gl_resource_manager.h"
#include "video_core/texture_cache/texture_cache.h"

namespace OpenGL {

using VideoCommon::SurfaceParams;
using VideoCommon::ViewParams;

class CachedSurfaceView;
class CachedSurface;
class TextureCacheOpenGL;

using Surface = std::shared_ptr<CachedSurface>;
using View = std::shared_ptr<CachedSurfaceView>;
using TextureCacheBase = VideoCommon::TextureCache<Surface, View>;

class CachedSurface final : public VideoCommon::SurfaceBase<View> {
    friend CachedSurfaceView;

public:
    explicit CachedSurface(const GPUVAddr gpu_addr, const SurfaceParams& params);
    ~CachedSurface();

    void UploadTexture(std::vector<u8>& staging_buffer) override;
    void DownloadTexture(std::vector<u8>& staging_buffer) override;

    GLenum GetTarget() const {
        return target;
    }

    GLuint GetTexture() const {
        return texture.handle;
    }

protected:
    void DecorateSurfaceName();

    View CreateView(const ViewParams& view_key) override;

private:
    void UploadTextureMipmap(u32 level, std::vector<u8>& staging_buffer);

    GLenum internal_format{};
    GLenum format{};
    GLenum type{};
    bool is_compressed{};
    GLenum target{};
    u32 view_count{};

    OGLTexture texture;
};

class CachedSurfaceView final : public VideoCommon::ViewBase {
public:
    explicit CachedSurfaceView(CachedSurface& surface, const ViewParams& params);
    ~CachedSurfaceView();

    /// Attaches this texture view to the current bound GL_DRAW_FRAMEBUFFER
    void Attach(GLenum attachment) const;

    GLuint GetTexture() {
        return texture_view.handle;
    }

    const SurfaceParams& GetSurfaceParams() const {
        return surface.GetSurfaceParams();
    }

    u32 GetWidth() const {
        const auto owner_params = GetSurfaceParams();
        return owner_params.GetMipWidth(params.base_level);
    }

    u32 GetHeight() const {
        const auto owner_params = GetSurfaceParams();
        return owner_params.GetMipHeight(params.base_level);
    }

    u32 GetDepth() const {
        const auto owner_params = GetSurfaceParams();
        return owner_params.GetMipDepth(params.base_level);
    }

    void ApplySwizzle(Tegra::Texture::SwizzleSource x_source,
                      Tegra::Texture::SwizzleSource y_source,
                      Tegra::Texture::SwizzleSource z_source,
                      Tegra::Texture::SwizzleSource w_source);

    void DecorateViewName(GPUVAddr gpu_addr, std::string prefix);

private:
    u32 EncodeSwizzle(Tegra::Texture::SwizzleSource x_source,
                      Tegra::Texture::SwizzleSource y_source,
                      Tegra::Texture::SwizzleSource z_source,
                      Tegra::Texture::SwizzleSource w_source) const {
        return (static_cast<u32>(x_source) << 24) | (static_cast<u32>(y_source) << 16) |
               (static_cast<u32>(z_source) << 8) | static_cast<u32>(w_source);
    }

    OGLTextureView CreateTextureView() const;

    CachedSurface& surface;
    GLenum target{};

    OGLTextureView texture_view;
    u32 swizzle;
};

class TextureCacheOpenGL final : public TextureCacheBase {
public:
    explicit TextureCacheOpenGL(Core::System& system, VideoCore::RasterizerInterface& rasterizer);
    ~TextureCacheOpenGL();

protected:
    Surface CreateSurface(GPUVAddr gpu_addr, const SurfaceParams& params) override;
    void ImageCopy(Surface src_surface, Surface dst_surface,
                   const VideoCommon::CopyParams& copy_params) override;
};

} // namespace OpenGL
