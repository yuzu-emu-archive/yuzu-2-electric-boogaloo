// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <array>
#include <cstddef>
#include <type_traits>

#include "common/common_types.h"
#include "core/core.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/gpu.h"
#include "video_core/renderer_opengl/gl_state_tracker.h"

#define OFF(field_name) MAXWELL3D_REG_INDEX(field_name)
#define NUM(field_name) (sizeof(Maxwell3D::Regs::field_name) / sizeof(u32))

namespace OpenGL {

namespace {

using namespace Dirty;
using namespace VideoCommon::Dirty;
using Tegra::Engines::Maxwell3D;
using Regs = Maxwell3D::Regs;
using Dirty = std::remove_reference_t<decltype(Maxwell3D::dirty)>;
using Tables = std::remove_reference_t<decltype(Maxwell3D::dirty.tables)>;
using Table = std::remove_reference_t<decltype(Maxwell3D::dirty.tables[0])>;

template <typename Integer>
void FillBlock(Table& table, std::size_t begin, std::size_t num, Integer dirty_index) {
    const auto it = std::begin(table) + begin;
    std::fill(it, it + num, static_cast<u8>(dirty_index));
}

template <typename Integer1, typename Integer2>
void FillBlock(Tables& tables, std::size_t begin, std::size_t num, Integer1 index_a,
               Integer2 index_b) {
    FillBlock(tables[0], begin, num, index_a);
    FillBlock(tables[1], begin, num, index_b);
}

void SetupDirtyRenderTargets(Tables& tables) {
    static constexpr std::size_t num_per_rt = NUM(rt[0]);
    static constexpr std::size_t begin = OFF(rt);
    static constexpr std::size_t num = num_per_rt * Regs::NumRenderTargets;
    for (std::size_t rt = 0; rt < Regs::NumRenderTargets; ++rt) {
        FillBlock(tables[0], begin + rt * num_per_rt, num_per_rt, ColorBuffer0 + rt);
    }
    FillBlock(tables[1], begin, num, RenderTargets);

    static constexpr std::array zeta_flags{ZetaBuffer, RenderTargets};
    for (std::size_t i = 0; i < std::size(zeta_flags); ++i) {
        const u8 flag = zeta_flags[i];
        auto& table = tables[i];
        table[OFF(zeta_enable)] = flag;
        table[OFF(zeta_width)] = flag;
        table[OFF(zeta_height)] = flag;
        FillBlock(table, OFF(zeta), NUM(zeta), flag);
    }
}

void SetupDirtyColorMasks(Tables& tables) {
    tables[0][OFF(color_mask_common)] = ColorMaskCommon;
    for (std::size_t rt = 0; rt < Regs::NumRenderTargets; ++rt) {
        const std::size_t offset = OFF(color_mask) + rt * NUM(color_mask[0]);
        FillBlock(tables[0], offset, NUM(color_mask[0]), ColorMask0 + rt);
    }

    FillBlock(tables[1], OFF(color_mask), NUM(color_mask), ColorMasks);
}

void SetupDirtyVertexArrays(Tables& tables) {
    static constexpr std::size_t num_array = 3;
    static constexpr std::size_t instance_base_offset = 3;
    for (std::size_t i = 0; i < Regs::NumVertexArrays; ++i) {
        const std::size_t array_offset = OFF(vertex_array) + i * NUM(vertex_array[0]);
        const std::size_t limit_offset = OFF(vertex_array_limit) + i * NUM(vertex_array_limit[0]);

        FillBlock(tables, array_offset, num_array, VertexBuffer0 + i, VertexBuffers);
        FillBlock(tables, limit_offset, NUM(vertex_array_limit), VertexBuffer0 + i, VertexBuffers);

        const std::size_t instance_array_offset = array_offset + instance_base_offset;
        tables[0][instance_array_offset] = static_cast<u8>(VertexInstance0 + i);
        tables[1][instance_array_offset] = VertexInstances;

        const std::size_t instance_offset = OFF(instanced_arrays) + i;
        tables[0][instance_offset] = static_cast<u8>(VertexInstance0 + i);
        tables[1][instance_offset] = VertexInstances;
    }
}

void SetupDirtyVertexFormat(Tables& tables) {
    for (std::size_t i = 0; i < Regs::NumVertexAttributes; ++i) {
        const std::size_t offset = OFF(vertex_attrib_format) + i * NUM(vertex_attrib_format[0]);
        FillBlock(tables[0], offset, NUM(vertex_attrib_format[0]), VertexFormat0 + i);
    }

    FillBlock(tables[1], OFF(vertex_attrib_format), Regs::NumVertexAttributes, VertexFormats);
}

void SetupDirtyViewports(Tables& tables) {
    for (std::size_t i = 0; i < Regs::NumViewports; ++i) {
        const std::size_t transf_offset = OFF(viewport_transform) + i * NUM(viewport_transform[0]);
        const std::size_t viewport_offset = OFF(viewports) + i * NUM(viewports[0]);

        FillBlock(tables[0], transf_offset, NUM(viewport_transform[0]), Viewport0 + i);
        FillBlock(tables[0], viewport_offset, NUM(viewports[0]), Viewport0 + i);
    }

    FillBlock(tables[1], OFF(viewport_transform), NUM(viewport_transform), Viewports);
    FillBlock(tables[1], OFF(viewports), NUM(viewports), Viewports);

    tables[0][OFF(viewport_transform_enabled)] = ViewportTransform;
    tables[1][OFF(viewport_transform_enabled)] = Viewports;
}

void SetupDirtyScissors(Tables& tables) {
    for (std::size_t i = 0; i < Regs::NumViewports; ++i) {
        const std::size_t offset = OFF(scissor_test) + i * NUM(scissor_test[0]);
        FillBlock(tables[0], offset, NUM(scissor_test[0]), Scissor0 + i);
    }
    FillBlock(tables[1], OFF(scissor_test), NUM(scissor_test), Scissors);
}

void SetupDirtyShaders(Tables& tables) {
    FillBlock(tables[0], OFF(shader_config[0]), NUM(shader_config[0]) * Regs::MaxShaderProgram,
              Shaders);
}

void SetupDirtyDepthTest(Tables& tables) {
    auto& table = tables[0];
    table[OFF(depth_test_enable)] = DepthTest;
    table[OFF(depth_write_enabled)] = DepthMask;
    table[OFF(depth_test_func)] = DepthTest;
}

void SetupDirtyStencilTest(Tables& tables) {
    static constexpr std::array offsets = {
        OFF(stencil_enable),          OFF(stencil_front_func_func), OFF(stencil_front_func_ref),
        OFF(stencil_front_func_mask), OFF(stencil_front_op_fail),   OFF(stencil_front_op_zfail),
        OFF(stencil_front_op_zpass),  OFF(stencil_front_mask),      OFF(stencil_two_side_enable),
        OFF(stencil_back_func_func),  OFF(stencil_back_func_ref),   OFF(stencil_back_func_mask),
        OFF(stencil_back_op_fail),    OFF(stencil_back_op_zfail),   OFF(stencil_back_op_zpass),
        OFF(stencil_back_mask)};
    for (const auto offset : offsets) {
        tables[0][offset] = StencilTest;
    }
}

void SetupDirtyAlphaTest(Tables& tables) {
    auto& table = tables[0];
    table[OFF(alpha_test_ref)] = AlphaTest;
    table[OFF(alpha_test_func)] = AlphaTest;
    table[OFF(alpha_test_enabled)] = AlphaTest;
}

void SetupDirtyBlend(Tables& tables) {
    FillBlock(tables[0], OFF(blend_color), NUM(blend_color), BlendColor);

    tables[0][OFF(independent_blend_enable)] = BlendIndependentEnabled;

    for (std::size_t i = 0; i < Regs::NumRenderTargets; ++i) {
        const std::size_t offset = OFF(independent_blend) + i * NUM(independent_blend[0]);
        FillBlock(tables[0], offset, NUM(independent_blend[0]), BlendState0 + i);

        tables[0][OFF(blend.enable) + i] = static_cast<u8>(BlendState0 + i);
    }
    FillBlock(tables[1], OFF(independent_blend), NUM(independent_blend), BlendStates);
    FillBlock(tables[1], OFF(blend), NUM(blend), BlendStates);
}

void SetupDirtyPrimitiveRestart(Tables& tables) {
    FillBlock(tables[0], OFF(primitive_restart), NUM(primitive_restart), PrimitiveRestart);
}

void SetupDirtyPolygonOffset(Tables& tables) {
    auto& table = tables[0];
    table[OFF(polygon_offset_fill_enable)] = PolygonOffset;
    table[OFF(polygon_offset_line_enable)] = PolygonOffset;
    table[OFF(polygon_offset_point_enable)] = PolygonOffset;
    table[OFF(polygon_offset_factor)] = PolygonOffset;
    table[OFF(polygon_offset_units)] = PolygonOffset;
    table[OFF(polygon_offset_clamp)] = PolygonOffset;
}

void SetupDirtyMultisampleControl(Tables& tables) {
    FillBlock(tables[0], OFF(multisample_control), NUM(multisample_control), MultisampleControl);
}

void SetupDirtyRasterizeEnable(Tables& tables) {
    tables[0][OFF(rasterize_enable)] = RasterizeEnable;
}

void SetupDirtyFramebufferSRGB(Tables& tables) {
    tables[0][OFF(framebuffer_srgb)] = FramebufferSRGB;
}

void SetupDirtyLogicOp(Tables& tables) {
    FillBlock(tables[0], OFF(logic_op), NUM(logic_op), LogicOp);
}

void SetupDirtyFragmentClampColor(Tables& tables) {
    tables[0][OFF(frag_color_clamp)] = FragmentClampColor;
}

void SetupDirtyPointSize(Tables& tables) {
    tables[0][OFF(vp_point_size)] = PointSize;
    tables[0][OFF(point_size)] = PointSize;
    tables[0][OFF(point_sprite_enable)] = PointSize;
}

void SetupDirtyMisc(Tables& tables) {
    auto& table = tables[0];

    table[OFF(clip_distance_enabled)] = ClipDistances;

    table[OFF(front_face)] = FrontFace;

    table[OFF(cull_test_enabled)] = CullTest;
    table[OFF(cull_face)] = CullTest;
}

} // Anonymous namespace

StateTracker::StateTracker(Core::System& system) : system{system} {}

void StateTracker::Initialize() {
    auto& dirty = system.GPU().Maxwell3D().dirty;
    auto& tables = dirty.tables;
    SetupDirtyRenderTargets(tables);
    SetupDirtyColorMasks(tables);
    SetupDirtyViewports(tables);
    SetupDirtyScissors(tables);
    SetupDirtyVertexArrays(tables);
    SetupDirtyVertexFormat(tables);
    SetupDirtyShaders(tables);
    SetupDirtyDepthTest(tables);
    SetupDirtyStencilTest(tables);
    SetupDirtyAlphaTest(tables);
    SetupDirtyBlend(tables);
    SetupDirtyPrimitiveRestart(tables);
    SetupDirtyPolygonOffset(tables);
    SetupDirtyMultisampleControl(tables);
    SetupDirtyRasterizeEnable(tables);
    SetupDirtyFramebufferSRGB(tables);
    SetupDirtyLogicOp(tables);
    SetupDirtyFragmentClampColor(tables);
    SetupDirtyPointSize(tables);
    SetupDirtyMisc(tables);

    auto& store = dirty.on_write_stores;
    store[RenderTargets] = true;
    store[ZetaBuffer] = true;
    for (std::size_t i = 0; i < Regs::NumRenderTargets; ++i) {
        store[ColorBuffer0 + i] = true;
    }
    store[VertexBuffers] = true;
    for (std::size_t i = 0; i < Regs::NumVertexArrays; ++i) {
        store[VertexBuffer0 + i] = true;
    }
}

} // namespace OpenGL
