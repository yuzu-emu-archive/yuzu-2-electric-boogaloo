// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include "common/common_types.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/file_sys/disk_filesystem.h"
#include "core/file_sys/savedata_factory.h"
#include "core/hle/kernel/process.h"

namespace FileSys {

std::string SaveStructDebugInfo(SaveStruct save_struct) {
    return fmt::format("[type={:02X}, title_id={:016X}, user_id={:016X}{:016X}, save_id={:016X}]",
                       static_cast<u8>(save_struct.type), save_struct.title_id,
                       save_struct.user_id[1], save_struct.user_id[0], save_struct.save_id);
}

SaveDataFactory::SaveDataFactory(std::string nand_directory)
    : nand_directory(std::move(nand_directory)) {}

ResultVal<std::unique_ptr<FileSystemBackend>> SaveDataFactory::Open(SaveDataSpaceId space,
                                                                    SaveStruct meta) {
    if ((meta.type == SaveDataType::SystemSaveData || meta.type == SaveDataType::SaveData)) {
        if (meta.zero_1 != 0) {
            LOG_WARNING(Service_FS,
                        "Possibly incorrect SaveStruct, type is "
                        "SystemSaveData||SaveData but offset 0x28 is non-zero ({:016X}).",
                        meta.zero_1);
        }
        if (meta.zero_2 != 0) {
            LOG_WARNING(Service_FS,
                        "Possibly incorrect SaveStruct, type is "
                        "SystemSaveData||SaveData but offset 0x30 is non-zero ({:016X}).",
                        meta.zero_2);
        }
        if (meta.zero_3 != 0) {
            LOG_WARNING(Service_FS,
                        "Possibly incorrect SaveStruct, type is "
                        "SystemSaveData||SaveData but offset 0x38 is non-zero ({:016X}).",
                        meta.zero_3);
        }
    }

    if (meta.type == SaveDataType::SystemSaveData && meta.title_id != 0) {
        LOG_WARNING(Service_FS,
                    "Possibly incorrect SaveStruct, type is SystemSaveData but title_id is "
                    "non-zero ({:016X}).",
                    meta.title_id);
    }

    std::string save_directory =
        GetFullPath(space, meta.type, meta.title_id, meta.user_id, meta.save_id);

    // TODO(DarkLordZach): Try to not create when opening, there are dedicated create save methods.
    // But, user_ids don't match so this works for now.

    if (!FileUtil::Exists(save_directory)) {
        // TODO(bunnei): This is a work-around to always create a save data directory if it does not
        // already exist. This is a hack, as we do not understand yet how this works on hardware.
        // Without a save data directory, many games will assert on boot. This should not have any
        // bad side-effects.
        FileUtil::CreateFullPath(save_directory);
    }

    // TODO(DarkLordZach): For some reason, CreateFullPath dosen't create the last bit. Should be
    // fixed with VFS.
    if (!FileUtil::IsDirectory(save_directory)) {
        FileUtil::CreateDir(save_directory);
    }

    // Return an error if the save data doesn't actually exist.
    if (!FileUtil::IsDirectory(save_directory)) {
        // TODO(Subv): Find out correct error code.
        return ResultCode(-1);
    }

    auto archive = std::make_unique<Disk_FileSystem>(save_directory);
    return MakeResult<std::unique_ptr<FileSystemBackend>>(std::move(archive));
}

ResultCode SaveDataFactory::Format(SaveDataSpaceId space, SaveStruct meta) {
    LOG_WARNING(Service_FS, "Formatting save data of space={:01X}, meta={}", static_cast<u8>(space),
                SaveStructDebugInfo(meta));
    std::string save_directory =
        GetFullPath(space, meta.type, meta.title_id, meta.user_id, meta.save_id);

    // Create the save data directory.
    if (!FileUtil::Exists(save_directory)) {
        // TODO(bunnei): This is a work-around to always create a save data directory if it does not
        // already exist. This is a hack, as we do not understand yet how this works on hardware.
        // Without a save data directory, many games will assert on boot. This should not have any
        // bad side-effects.
        FileUtil::CreateFullPath(save_directory);
    }

    // TODO(DarkLordZach): For some reason, CreateFullPath dosen't create the last bit. Should be
    // fixed with VFS.
    if (!FileUtil::IsDirectory(save_directory)) {
        FileUtil::CreateDir(save_directory);
    }

    // Return an error if the save data doesn't actually exist.
    if (!FileUtil::IsDirectory(save_directory)) {
        // TODO(Subv): Find out correct error code.
        return ResultCode(-1);
    }

    return RESULT_SUCCESS;
}

std::string SaveDataFactory::GetFullPath(SaveDataSpaceId space, SaveDataType type, u64 title_id,
                                         u128 user_id, u64 save_id) const {
    if (type == SaveDataType::SaveData && title_id == 0)
        title_id = Core::CurrentProcess()->program_id;

    std::string prefix;

    switch (space) {
    case SaveDataSpaceId::NandSystem:
        prefix = nand_directory + "system/save/";
    case SaveDataSpaceId::NandUser:
        prefix = nand_directory + "user/save/";
    default:
        ASSERT_MSG(true, "Unrecognized SaveDataSpaceId: {:02X}", static_cast<u8>(space));
    }

    switch (type) {
    case SaveDataType::SystemSaveData:
        return fmt::format("{}{:016X}/{:016X}{:016X}", prefix, save_id, user_id[1], user_id[0]);
    case SaveDataType::SaveData:
        return fmt::format("{}{:08X}/{:016X}{:016X}/{:016X}", prefix, 0, user_id[1], user_id[0],
                           title_id);
    default:
        ASSERT_MSG(true, "Unrecognized SaveDataType: {:02X}", static_cast<u8>(type));
    }

    return "";
}

} // namespace FileSys
