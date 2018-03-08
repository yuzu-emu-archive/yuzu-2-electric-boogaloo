// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/kernel/errors.h"
#include "core/memory.h"

namespace IPC {

/// Size of the command buffer area, in 32-bit words.
constexpr size_t COMMAND_BUFFER_LENGTH = 0x100 / sizeof(u32);

/// Maximum number of static buffers per thread
constexpr size_t MAX_STATIC_BUFFERS = 16;
// These errors are commonly returned by invalid IPC translations, so alias them here for
// convenience.
// TODO(yuriks): These will probably go away once translation is implemented inside the kernel.
constexpr auto ERR_INVALID_HANDLE = Kernel::ERR_INVALID_HANDLE_OS;

enum class ControlCommand : u32 {
    ConvertSessionToDomain = 0,
    ConvertDomainToSession = 1,
    DuplicateSession = 2,
    QueryPointerBufferSize = 3,
    DuplicateSessionEx = 4,
    Unspecified,
};

enum class CommandType : u32 {
    Close = 2,
    Request = 4,
    Control = 5,
    Unspecified,
};

struct CommandHeader {
    union {
        u32_le raw_low;
        BitField<0, 16, CommandType> type;
        BitField<16, 4, u32_le> num_buf_x_descriptors;
        BitField<20, 4, u32_le> num_buf_a_descriptors;
        BitField<24, 4, u32_le> num_buf_b_descriptors;
        BitField<28, 4, u32_le> num_buf_w_descriptors;
    };

    enum class BufferDescriptorCFlag : u32 {
        Disabled = 0,
        InlineDescriptor = 1,
        OneDescriptor = 2,
    };

    union {
        u32_le raw_high;
        BitField<0, 10, u32_le> data_size;
        BitField<10, 4, BufferDescriptorCFlag> buf_c_descriptor_flags;
        BitField<31, 1, u32_le> enable_handle_descriptor;
    };
};
static_assert(sizeof(CommandHeader) == 8, "CommandHeader size is incorrect");

union HandleDescriptorHeader {
    u32_le raw_high;
    BitField<0, 1, u32_le> send_current_pid;
    BitField<1, 4, u32_le> num_handles_to_copy;
    BitField<5, 4, u32_le> num_handles_to_move;
};
static_assert(sizeof(HandleDescriptorHeader) == 4, "HandleDescriptorHeader size is incorrect");

struct BufferDescriptorX {
    union {
        BitField<0, 6, u32_le> counter_bits_0_5;
        BitField<6, 3, u32_le> address_bits_36_38;
        BitField<9, 3, u32_le> counter_bits_9_11;
        BitField<12, 4, u32_le> address_bits_32_35;
        BitField<16, 16, u32_le> size;
    };

    u32_le address_bits_0_31;

    u32_le Counter() const {
        u32_le counter{counter_bits_0_5};
        counter |= counter_bits_9_11 << 9;
        return counter;
    }

    VAddr Address() const {
        VAddr address{address_bits_0_31};
        address |= static_cast<VAddr>(address_bits_32_35) << 32;
        address |= static_cast<VAddr>(address_bits_36_38) << 36;
        return address;
    }

    u64 Size() const {
        return static_cast<u64>(size);
    }
};
static_assert(sizeof(BufferDescriptorX) == 8, "BufferDescriptorX size is incorrect");

struct BufferDescriptorABW {
    u32_le size_bits_0_31;
    u32_le address_bits_0_31;

    union {
        BitField<0, 2, u32_le> flags;
        BitField<2, 3, u32_le> address_bits_36_38;
        BitField<24, 4, u32_le> size_bits_32_35;
        BitField<28, 4, u32_le> address_bits_32_35;
    };

    VAddr Address() const {
        VAddr address{address_bits_0_31};
        address |= static_cast<VAddr>(address_bits_32_35) << 32;
        address |= static_cast<VAddr>(address_bits_36_38) << 36;
        return address;
    }

    u64 Size() const {
        u64 size{size_bits_0_31};
        size |= static_cast<u64>(size_bits_32_35) << 32;
        return size;
    }
};
static_assert(sizeof(BufferDescriptorABW) == 12, "BufferDescriptorABW size is incorrect");

struct BufferDescriptorC {
    u32_le address_bits_0_31;

    union {
        BitField<0, 16, u32_le> address_bits_32_47;
        BitField<16, 16, u32_le> size;
    };

    VAddr Address() const {
        VAddr address{address_bits_0_31};
        address |= static_cast<VAddr>(address_bits_32_47) << 32;
        return address;
    }

    u64 Size() const {
        return static_cast<u64>(size);
    }
};
static_assert(sizeof(BufferDescriptorC) == 8, "BufferDescriptorC size is incorrect");

struct DataPayloadHeader {
    u32_le magic;
    INSERT_PADDING_WORDS(1);
};
static_assert(sizeof(DataPayloadHeader) == 8, "DataPayloadRequest size is incorrect");

struct DomainMessageHeader {
    enum class CommandType : u32_le {
        SendMessage = 1,
        CloseVirtualHandle = 2,
    };

    union {
        // Used when responding to an IPC request, Server -> Client.
        struct {
            u32_le num_objects;
            INSERT_PADDING_WORDS(3);
        };

        // Used when performing an IPC request, Client -> Server.
        struct {
            union {
                BitField<0, 8, CommandType> command;
                BitField<16, 16, u32_le> size;
            };
            u32_le object_id;
            INSERT_PADDING_WORDS(2);
        };
    };
};
static_assert(sizeof(DomainMessageHeader) == 16, "DomainMessageHeader size is incorrect");

} // namespace IPC
