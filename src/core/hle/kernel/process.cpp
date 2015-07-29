// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/common_funcs.h"
#include "common/logging/log.h"
#include "common/make_unique.h"

#include "core/hle/kernel/memory.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/resource_limit.h"
#include "core/hle/kernel/thread.h"
#include "core/hle/kernel/vm_manager.h"
#include "core/memory.h"

namespace Kernel {

SharedPtr<CodeSet> CodeSet::Create(std::string name, u64 program_id) {
    SharedPtr<CodeSet> codeset(new CodeSet);

    codeset->name = std::move(name);
    codeset->program_id = program_id;

    return codeset;
}

CodeSet::CodeSet() {}
CodeSet::~CodeSet() {}

u32 Process::next_process_id;

SharedPtr<Process> Process::Create(SharedPtr<CodeSet> code_set) {
    SharedPtr<Process> process(new Process);

    process->codeset = std::move(code_set);
    process->flags.raw = 0;
    process->flags.memory_region = MemoryRegion::APPLICATION;
    Memory::InitLegacyAddressSpace(process->vm_manager);

    return process;
}

void Process::ParseKernelCaps(const u32* kernel_caps, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        u32 descriptor = kernel_caps[i];
        u32 type = descriptor >> 20;

        if (descriptor == 0xFFFFFFFF) {
            // Unused descriptor entry
            continue;
        } else if ((type & 0xF00) == 0xE00) { // 0x0FFF
            // Allowed interrupts list
            LOG_WARNING(Loader, "ExHeader allowed interrupts list ignored");
        } else if ((type & 0xF80) == 0xF00) { // 0x07FF
            // Allowed syscalls mask
            unsigned int index = ((descriptor >> 24) & 7) * 24;
            u32 bits = descriptor & 0xFFFFFF;

            while (bits && index < svc_access_mask.size()) {
                svc_access_mask.set(index, bits & 1);
                ++index; bits >>= 1;
            }
        } else if ((type & 0xFF0) == 0xFE0) { // 0x00FF
            // Handle table size
            handle_table_size = descriptor & 0x3FF;
        } else if ((type & 0xFF8) == 0xFF0) { // 0x007F
            // Misc. flags
            flags.raw = descriptor & 0xFFFF;
        } else if ((type & 0xFFE) == 0xFF8) { // 0x001F
            // Mapped memory range
            if (i+1 >= len || ((kernel_caps[i+1] >> 20) & 0xFFE) != 0xFF8) {
                LOG_WARNING(Loader, "Incomplete exheader memory range descriptor ignored.");
                continue;
            }
            u32 end_desc = kernel_caps[i+1];
            ++i; // Skip over the second descriptor on the next iteration

            AddressMapping mapping;
            mapping.address = descriptor << 12;
            mapping.size = (end_desc << 12) - mapping.address;
            mapping.writable = (descriptor & (1 << 20)) != 0;
            mapping.unk_flag = (end_desc & (1 << 20)) != 0;

            address_mappings.push_back(mapping);
        } else if ((type & 0xFFF) == 0xFFE) { // 0x000F
            // Mapped memory page
            AddressMapping mapping;
            mapping.address = descriptor << 12;
            mapping.size = Memory::PAGE_SIZE;
            mapping.writable = true; // TODO: Not sure if correct
            mapping.unk_flag = false;
        } else if ((type & 0xFE0) == 0xFC0) { // 0x01FF
            // Kernel version
            kernel_version = descriptor & 0xFFFF;

            int minor = kernel_version & 0xFF;
            int major = (kernel_version >> 8) & 0xFF;
            LOG_DEBUG(Loader, "ExHeader kernel version: %d.%d", major, minor);
        } else {
            LOG_ERROR(Loader, "Unhandled kernel caps descriptor: 0x%08X", descriptor);
        }
    }
}

void Process::Run(s32 main_thread_priority, u32 stack_size) {
    auto MapSegment = [&](CodeSet::Segment& segment, VMAPermission permissions, MemoryState memory_state) {
        auto vma = vm_manager.MapMemoryBlock(segment.addr, codeset->memory,
                segment.offset, segment.size, memory_state).Unwrap();
        vm_manager.Reprotect(vma, permissions);
    };

    // Map CodeSet segments
    MapSegment(codeset->code,   VMAPermission::ReadExecute, MemoryState::Code);
    MapSegment(codeset->rodata, VMAPermission::Read,        MemoryState::Code);
    MapSegment(codeset->data,   VMAPermission::ReadWrite,   MemoryState::Private);

    // Allocate and map stack
    vm_manager.MapMemoryBlock(Memory::HEAP_VADDR_END - stack_size,
            std::make_shared<std::vector<u8>>(stack_size, 0), 0, stack_size, MemoryState::Locked
            ).Unwrap();

    vm_manager.LogLayout(Log::Level::Debug);
    Kernel::SetupMainThread(codeset->entrypoint, main_thread_priority);
}

ResultVal<VAddr> Process::HeapAllocate(VAddr target, u32 size, VMAPermission perms) {
    if (target < Memory::HEAP_VADDR || target + size > Memory::HEAP_VADDR_END || target + size < target) {
        return ERR_INVALID_ADDRESS;
    }

    if (heap_memory == nullptr) {
        // Initialize heap
        heap_memory = std::make_shared<std::vector<u8>>();
        heap_start = heap_end = target;
    }

    // If necessary, expand backing vector to cover new heap extents.
    if (target < heap_start) {
        heap_memory->insert(begin(*heap_memory), heap_start - target, 0);
        heap_start = target;
        vm_manager.RefreshMemoryBlockMappings(heap_memory.get());
    }
    if (target + size > heap_end) {
        heap_memory->insert(end(*heap_memory), (target + size) - heap_end, 0);
        heap_end = target + size;
        vm_manager.RefreshMemoryBlockMappings(heap_memory.get());
    }
    ASSERT(heap_end - heap_start == heap_memory->size());

    CASCADE_RESULT(auto vma, vm_manager.MapMemoryBlock(target, heap_memory, target - heap_start, size, MemoryState::Private));
    vm_manager.Reprotect(vma, perms);

    return MakeResult<VAddr>(heap_end - size);
}

ResultCode Process::HeapFree(VAddr target, u32 size) {
    if (target < Memory::HEAP_VADDR || target + size > Memory::HEAP_VADDR_END || target + size < target) {
        return ERR_INVALID_ADDRESS;
    }

    ResultCode result = vm_manager.UnmapRange(target, size);
    if (result.IsError()) return result;

    return RESULT_SUCCESS;
}

ResultVal<VAddr> Process::LinearAllocate(VAddr target, u32 size, VMAPermission perms) {
    if (linear_heap_memory == nullptr) {
        // Initialize heap
        linear_heap_memory = std::make_shared<std::vector<u8>>();
    }

    VAddr heap_end = Memory::LINEAR_HEAP_VADDR + (u32)linear_heap_memory->size();
    // Games and homebrew only ever seem to pass 0 here (which lets the kernel decide the address),
    // but explicit addresses are also accepted and respected.
    if (target == 0) {
        target = heap_end;
    }

    if (target < Memory::LINEAR_HEAP_VADDR || target + size > Memory::LINEAR_HEAP_VADDR_END ||
        target > heap_end || target + size < target) {

        return ERR_INVALID_ADDRESS;
    }

    // Expansion of the linear heap is only allowed if you do an allocation immediatelly at its
    // end. It's possible to free gaps in the middle of the heap and then reallocate them later,
    // but expansions are only allowed at the end.
    if (target == heap_end) {
        linear_heap_memory->insert(linear_heap_memory->end(), size, 0);
        vm_manager.RefreshMemoryBlockMappings(linear_heap_memory.get());
    }

    size_t offset = target - Memory::LINEAR_HEAP_VADDR;
    CASCADE_RESULT(auto vma, vm_manager.MapMemoryBlock(target, linear_heap_memory, offset, size, MemoryState::Continuous));
    vm_manager.Reprotect(vma, perms);

    return MakeResult<VAddr>(target);
}

ResultCode Process::LinearFree(VAddr target, u32 size) {
    if (linear_heap_memory == nullptr || target < Memory::LINEAR_HEAP_VADDR ||
        target + size > Memory::LINEAR_HEAP_VADDR_END || target + size < target) {

        return ERR_INVALID_ADDRESS;
    }

    VAddr heap_end = Memory::LINEAR_HEAP_VADDR + (u32)linear_heap_memory->size();
    if (target + size > heap_end) {
        return ERR_INVALID_ADDRESS_STATE;
    }

    ResultCode result = vm_manager.UnmapRange(target, size);
    if (result.IsError()) return result;

    if (target + size == heap_end) {
        // End of linear heap has been freed, so check what's the last allocated block in it and
        // reduce the size.
        auto vma = vm_manager.FindVMA(target);
        ASSERT(vma != vm_manager.vma_map.end());
        ASSERT(vma->second.type == VMAType::Free);
        VAddr new_end = vma->second.base;
        if (new_end >= Memory::LINEAR_HEAP_VADDR) {
            linear_heap_memory->resize(new_end - Memory::LINEAR_HEAP_VADDR);
        }
    }

    return RESULT_SUCCESS;
}

Kernel::Process::Process() {}
Kernel::Process::~Process() {}

SharedPtr<Process> g_current_process;

}
