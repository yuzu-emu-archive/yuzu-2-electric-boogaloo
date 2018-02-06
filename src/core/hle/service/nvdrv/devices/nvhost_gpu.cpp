// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <map>
#include "common/assert.h"
#include "common/logging/log.h"
#include "core/hle/service/nvdrv/devices/nvhost_gpu.h"

namespace Service {
namespace Nvidia {
namespace Devices {

u32 nvhost_gpu::ioctl(Ioctl command, const std::vector<u8>& input, std::vector<u8>& output) {
    LOG_DEBUG(Service_NVDRV, "called, command=0x%08x, input_size=0x%llx, output_size=0x%llx",
              command, input.size(), output.size());

    switch (static_cast<IoctlCommand>(command.raw)) {
    case IoctlCommand::IocSetNVMAPfdCommand:
        return SetNVMAPfd(input, output);
    case IoctlCommand::IocSetClientDataCommand:
        return SetClientData(input, output);
    case IoctlCommand::IocGetClientDataCommand:
        return GetClientData(input, output);
    case IoctlCommand::IocZCullBind:
        return ZCullBind(input, output);
    case IoctlCommand::IocSetErrorNotifierCommand:
        return SetErrorNotifier(input, output);
    case IoctlCommand::IocChannelSetPriorityCommand:
        return SetChannelPriority(input, output);
    case IoctlCommand::IocAllocGPFIFOEx2Command:
        return AllocGPFIFOEx2(input, output);
    case IoctlCommand::IocAllocObjCtxCommand:
        return AllocateObjectContext(input, output);
    }

    if (command.group == NVGPU_IOCTL_MAGIC) {
        if (command.cmd == NVGPU_IOCTL_CHANNEL_SUBMIT_GPFIFO) {
            return SubmitGPFIFO(input, output);
        }
    }

    UNIMPLEMENTED();
    return 0;
};

u32 nvhost_gpu::SetNVMAPfd(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlSetNvmapFD params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_DEBUG(Service_NVDRV, "called, fd=%x", params.nvmap_fd);
    nvmap_fd = params.nvmap_fd;
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_gpu::SetClientData(const std::vector<u8>& input, std::vector<u8>& output) {
    LOG_DEBUG(Service_NVDRV, "called");
    IoctlClientData params{};
    std::memcpy(&params, input.data(), input.size());
    user_data = params.data;
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_gpu::GetClientData(const std::vector<u8>& input, std::vector<u8>& output) {
    LOG_DEBUG(Service_NVDRV, "called");
    IoctlClientData params{};
    std::memcpy(&params, input.data(), input.size());
    params.data = user_data;
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_gpu::ZCullBind(const std::vector<u8>& input, std::vector<u8>& output) {
    std::memcpy(&zcull_params, input.data(), input.size());
    LOG_DEBUG(Service_NVDRV, "called, gpu_va=%lx, mode=%x", zcull_params.gpu_va, zcull_params.mode);
    std::memcpy(output.data(), &zcull_params, output.size());
    return 0;
}

u32 nvhost_gpu::SetErrorNotifier(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlSetErrorNotifier params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_WARNING(Service_NVDRV, "(STUBBED) called, offset=%lx, size=%lx, mem=%x", params.offset,
                params.size, params.mem);
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_gpu::SetChannelPriority(const std::vector<u8>& input, std::vector<u8>& output) {
    std::memcpy(&channel_priority, input.data(), input.size());
    LOG_DEBUG(Service_NVDRV, "(STUBBED) called, priority=%x", channel_priority);
    std::memcpy(output.data(), &channel_priority, output.size());
    return 0;
}

u32 nvhost_gpu::AllocGPFIFOEx2(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlAllocGpfifoEx2 params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_WARNING(Service_NVDRV,
                "(STUBBED) called, num_entries=%x, flags=%x, unk0=%x, unk1=%x, unk2=%x, unk3=%x",
                params.num_entries, params.flags, params.unk0, params.unk1, params.unk2,
                params.unk3);
    params.fence_out.id = 0;
    params.fence_out.value = 0;
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_gpu::AllocateObjectContext(const std::vector<u8>& input, std::vector<u8>& output) {
    IoctlAllocObjCtx params{};
    std::memcpy(&params, input.data(), input.size());
    LOG_WARNING(Service_NVDRV, "(STUBBED) called, class_num=%x, flags=%x", params.class_num,
                params.flags);
    params.obj_id = 0x0;
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

u32 nvhost_gpu::SubmitGPFIFO(const std::vector<u8>& input, std::vector<u8>& output) {
    if (input.size() < sizeof(IoctlSubmitGpfifo))
        UNIMPLEMENTED();
    IoctlSubmitGpfifo params{};
    std::memcpy(&params, input.data(), 24);
    LOG_WARNING(Service_NVDRV, "(STUBBED) called, gpfifo=%lx, num_entries=%x, flags=%x",
                params.gpfifo, params.num_entries, params.flags);

    auto entries = std::vector<IoctlGpfifoEntry>();
    entries.resize(params.num_entries);
    std::memcpy(&entries[0], &input.data()[24], params.num_entries * 8);
    for (auto entry : entries) {
        u64 va_addr = (entry.gpu_va_hi << 32) | entry.entry0;
        // TODO(ogniK): Process these
    }
    params.fence_out.id = 0;
    params.fence_out.value = 0;
    std::memcpy(output.data(), &params, output.size());
    return 0;
}

} // namespace Devices
} // namespace Nvidia
} // namespace Service
