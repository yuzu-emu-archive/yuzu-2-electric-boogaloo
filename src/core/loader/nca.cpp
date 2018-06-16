// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <vector>

#include "common/common_funcs.h"
#include "common/common_paths.h"
#include "common/file_util.h"
#include "common/logging/log.h"
#include "common/swap.h"
#include "core/core.h"
#include "core/file_sys/disk_filesystem.h"
#include "core/file_sys/program_metadata.h"
#include "core/file_sys/romfs_factory.h"
#include "core/hle/kernel/process.h"
#include "core/hle/kernel/resource_limit.h"
#include "core/loader/nro.h"
#include "core/memory.h"
#include "nca.h"
#include "nso.h"

namespace Loader {

enum NcaContentType {
    NCT_PROGRAM = 0,
    NCT_META = 1,
    NCT_CONTROL = 2,
    NCT_MANUAL = 3,
    NCT_DATA = 4
};

enum NcaSectionFilesystemType { NSFS_PFS0 = 0x2, NSFS_ROMFS = 0x3 };

struct NcaSectionTableEntry {
    u32 media_offset;
    u32 media_end_offset;
    INSERT_PADDING_BYTES(0x8);
};
static_assert(sizeof(NcaSectionTableEntry) == 0x10, "NcaSectionTableEntry has incorrect size.");

struct NcaHeader {
    u8 rsa_signature_1[0x100];
    u8 rsa_signature_2[0x100];
    u32 magic;
    u8 is_system;
    u8 content_type;
    u8 crypto_type;
    u8 key_index;
    u64 size;
    u64 title_id;
    INSERT_PADDING_BYTES(0x4);
    u32 sdk_version;
    u8 crypto_type_2;
    INSERT_PADDING_BYTES(15);
    u8 rights_id[0x10];
    NcaSectionTableEntry section_tables[0x4];
    u8 hash_tables[0x4][0x20];
    u8 key_area[0x4][0x10];
    INSERT_PADDING_BYTES(0xC0);
};
static_assert(sizeof(NcaHeader) == 0x400, "NcaHeader has incorrect size.");

struct NcaSectionHeaderBlock {
    INSERT_PADDING_BYTES(3);
    u8 filesystem_type;
    u8 crypto_type;
    INSERT_PADDING_BYTES(3);
};
static_assert(sizeof(NcaSectionHeaderBlock) == 0x8, "NcaSectionHeaderBlock has incorrect size.");

struct Pfs0Superblock {
    NcaSectionHeaderBlock header_block;
    std::array<u8, 0x20> hash;
    u32 size;
    INSERT_PADDING_BYTES(4);
    u64 hash_table_offset;
    u64 hash_table_size;
    u64 pfs0_header_offset;
    u64 pfs0_size;
    INSERT_PADDING_BYTES(432);
};
static_assert(sizeof(Pfs0Superblock) == 0x200, "Pfs0Superblock has incorrect size.");

///**
// * Adapted from hactool/aes.c, void aes_xts_decrypt(aes_ctx_t *ctx, void *dst,
// *      const void *src, size_t l, size_t sector, size_t sector_size)
// * and from various other functions in aes.c (get_tweak, aes_setiv, aes_decrypt)
// *
// * Copyright (c) 2018, SciresM
// *
// * Permission to use, copy, modify, and/or distribute this software for any
// * purpose with or without fee is hereby granted, provided that the above
// * copyright notice and this permission notice appear in all copies.
// */
// template <size_t size>
// static std::array<u8, size> AesXtsDecrypt(std::array<u8, size> in, size_t sector_size) {
//    std::array<u8, 0x10> tweak;
//    ASSERT(size % sector_size == 0);
//
//    u8 sector = 0;
//
//    for (size_t i = 0; i < size; i += sector_size) {
//
//        // Get tweak
//        size_t sector_temp = sector++;
//        for (int i = 0xF; i >= 0; --i) {
//            tweak[i] = static_cast<u8>(sector_temp);
//            sector_temp >>= 8;
//        }
//
//
//    }
//}

Nca::Nca(FileUtil::IOFile&& in_file, std::string path) : file(std::move(in_file)), path(path) {
    file.Seek(0, SEEK_SET);
    std::array<u8, 0x400> header_array{};
    if (0x400 != file.ReadBytes(header_array.data(), 0x400))
        NGLOG_CRITICAL(Loader, "File reader errored out during header read.");

    NcaHeader header = *reinterpret_cast<NcaHeader*>(header_array.data());

    int number_sections =
        std::count_if(std::begin(header.section_tables), std::end(header.section_tables),
                      [](NcaSectionTableEntry entry) { return entry.media_offset > 0; });

    for (int i = 0; i < number_sections; ++i) {
        file.Seek(0x400 + (i << 9), SEEK_SET);
        std::array<u8, 0x10> array{};
        if (0x10 != file.ReadBytes(array.data(), 0x10))
            NGLOG_CRITICAL(Loader, "File reader errored out during header read.");

        NcaSectionHeaderBlock block = *reinterpret_cast<NcaSectionHeaderBlock*>(array.data());

        if (block.filesystem_type == NSFS_ROMFS) {
            romfs_offset = (header.section_tables[i].media_offset << 9);
            romfs_size = (header.section_tables[i].media_end_offset << 9) - romfs_offset;
        } else if (block.filesystem_type == NSFS_PFS0) {
            Pfs0Superblock sb;
            file.Seek(0x400 + (i << 9), SEEK_SET);
            if (0x200 != file.ReadBytes(&sb, 0x200))
                NGLOG_CRITICAL(Loader, "File reader errored out during header read.");

            pfs.emplace_back();
            pfs[pfs.size() - 1].Load(path, (header.section_tables[i].media_offset << 9) +
                                               sb.pfs0_header_offset);
            pfs_offset.emplace_back((header.section_tables[i].media_offset << 9) +
                                    sb.pfs0_header_offset);
        }
    }
}

FileSys::PartitionFilesystem Nca::GetPfs(u8 id) {
    return pfs[id];
}

// TODO(DarkLordZach): INLINE THIS
static bool contains(const std::vector<std::string>& vec, std::string str) {
    return std::find(vec.begin(), vec.end(), str) != vec.end();
}

static bool IsPfsExeFs(const FileSys::PartitionFilesystem& pfs) {
    // According to switchbrew, an exefs must only contain these two files:
    return pfs.GetFileSize("main") > 0 && pfs.GetFileSize("main.npdm") > 0;
}

u8 Nca::GetExeFsPfsId() {
    for (int i = 0; i < pfs.size(); ++i) {
        if (IsPfsExeFs(pfs[i]))
            return i;
    }

    return -1;
}

u64 Nca::GetExeFsFileOffset(const std::string& file_name) {
    return pfs[GetExeFsPfsId()].GetFileOffset(file_name) + pfs_offset[GetExeFsPfsId()];
}

u64 Nca::GetExeFsFileSize(const std::string& file_name) {
    return pfs[GetExeFsPfsId()].GetFileSize(file_name);
}

u64 Nca::GetRomFsOffset() {
    return romfs_offset;
}

u64 Nca::GetRomFsSize() {
    return romfs_size;
}

std::vector<u8> Nca::GetExeFsFile(const std::string& file_name) {
    std::vector<u8> out(GetExeFsFileSize(file_name));
    file.Seek(GetExeFsFileOffset(file_name), SEEK_SET);
    file.ReadBytes(out.data(), GetExeFsFileSize(file_name));
    return out;
}

static bool IsValidNca(const NcaHeader& header) {
    return header.magic == Common::MakeMagic('N', 'C', 'A', '2') ||
           header.magic == Common::MakeMagic('N', 'C', 'A', '3');
}

///**
// * Adapted from hactool/nca.c, int nca_decrypt_header(nca_ctx_t *ctx)
// *
// * Copyright (c) 2018, SciresM
// *
// * Permission to use, copy, modify, and/or distribute this software for any
// * purpose with or without fee is hereby granted, provided that the above
// * copyright notice and this permission notice appear in all copies.
// */
// static NcaHeader DecryptHeader(std::array<u8, 0xC00> enc_data) {
//    // Just in case its decrypted.
//    NcaHeader header = *reinterpret_cast<NcaHeader*>(enc_data.data());
//    if (IsValidNca(header))
//        return header;
//
//    std::array<u8, 0xC00> dec = AesXtsDecrypt(enc_data, 0x200);
//
//    header = *reinterpret_cast<NcaHeader*>(dec.data());
//    return header;
//}

AppLoader_NCA::AppLoader_NCA(FileUtil::IOFile&& file, std::string filepath)
    : AppLoader(std::move(file)), filepath(std::move(filepath)) {}

FileType AppLoader_NCA::IdentifyType(FileUtil::IOFile& file, const std::string&) {
    file.Seek(0, SEEK_SET);
    std::array<u8, 0x400> header_enc_array{};
    if (0x400 != file.ReadBytes(header_enc_array.data(), 0x400))
        return FileType::Error;

    // NcaHeader header = DecryptHeader(header_enc_array);
    // TODO(DarkLordZach): Assuming everything is decrypted. Add crypto support.
    NcaHeader header = *reinterpret_cast<NcaHeader*>(header_enc_array.data());

    if (IsValidNca(header) && header.content_type == NCT_PROGRAM)
        return FileType::NCA;

    return FileType::Error;
}

ResultStatus AppLoader_NCA::Load(Kernel::SharedPtr<Kernel::Process>& process) {
    if (is_loaded) {
        return ResultStatus::ErrorAlreadyLoaded;
    }
    if (!file.IsOpen()) {
        return ResultStatus::Error;
    }

    Nca nca{std::move(file), filepath};

    ResultStatus result = metadata.Load(nca.GetExeFsFile("main.npdm"));
    if (result != ResultStatus::Success) {
        return result;
    }
    metadata.Print();

    const FileSys::ProgramAddressSpaceType arch_bits{metadata.GetAddressSpaceType()};
    if (arch_bits == FileSys::ProgramAddressSpaceType::Is32Bit) {
        return ResultStatus::ErrorUnsupportedArch;
    }

    VAddr next_load_addr{Memory::PROCESS_IMAGE_VADDR};
    for (const auto& module : {"rtld", "main", "subsdk0", "subsdk1", "subsdk2", "subsdk3",
                               "subsdk4", "subsdk5", "subsdk6", "subsdk7", "sdk"}) {
        const VAddr load_addr = next_load_addr;
        next_load_addr = AppLoader_NSO::LoadModule(module, nca.GetExeFsFile(module), load_addr);
        if (next_load_addr) {
            NGLOG_DEBUG(Loader, "loaded module {} @ 0x{:X}", module, load_addr);
        } else {
            next_load_addr = load_addr;
        }
    }

    process->program_id = metadata.GetTitleID();
    process->svc_access_mask.set();
    process->address_mappings = default_address_mappings;
    process->resource_limit =
        Kernel::ResourceLimit::GetForCategory(Kernel::ResourceLimitCategory::APPLICATION);
    process->Run(Memory::PROCESS_IMAGE_VADDR, metadata.GetMainThreadPriority(),
                 metadata.GetMainThreadStackSize());

    // Load the RomFS

    is_loaded = true;
    return ResultStatus::Success;
}

} // namespace Loader
