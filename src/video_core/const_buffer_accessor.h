#pragma once

#include <cstring>

#include "common/common_types.h"
#include "core/core.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/gpu.h"
#include "video_core/memory_manager.h"

namespace Tegra {

namespace ConstBufferAccessor {

template <typename T>
T access(Tegra::Engines::Maxwell3D::Regs::ShaderStage stage, u64 const_buffer, u64 offset) {
    auto& gpu = Core::System::GetInstance().GPU();
    auto& memory_manager = gpu.MemoryManager();
    auto& maxwell3d = gpu.Maxwell3D();
    const auto& shader_stage = maxwell3d.state.shader_stages[static_cast<std::size_t>(stage)];
    const auto& buffer = shader_stage.const_buffers[const_buffer];
    T result;
    std::memcpy(&result, memory_manager.GetPointer(buffer.address + offset), sizeof(T));
    return result;
}

} // namespace ConstBufferAccessor
} // namespace Tegra
