// Copyright 2013 Dolphin Emulator Project / 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <cstdio>
#include <fstream>
#include <functional>
#include <string>
#include <type_traits>
#include <vector>
#include "common/common_types.h"
#ifdef _MSC_VER
#include "common/string_util.h"
#endif

// User directory indices for GetUserPath
enum {
    D_USER_IDX,
    D_ROOT_IDX,
    D_CONFIG_IDX,
    D_CACHE_IDX,
    D_SDMC_IDX,
    D_NAND_IDX,
    D_SYSDATA_IDX,
    D_LOGS_IDX,
    NUM_PATH_INDICES
};

namespace FileUtil {

// FileSystem tree node/
struct FSTEntry {
    bool isDirectory;
    u64 size;                 // file length or number of entries from children
    std::string physicalName; // name on disk
    std::string virtualName;  // name in FST names table
    std::vector<FSTEntry> children;
};

// Returns true if file filename exists
bool Exists(const std::string& filename);

// Returns true if filename is a directory
bool IsDirectory(const std::string& filename);

// Returns the size of filename (64bit)
u64 GetSize(const std::string& filename);

// Overloaded GetSize, accepts file descriptor
u64 GetSize(const int fd);

// Overloaded GetSize, accepts FILE*
u64 GetSize(FILE* f);

// Returns true if successful, or path already exists.
bool CreateDir(const std::string& filename);

// Creates the full path of fullPath returns true on success
bool CreateFullPath(const std::string& fullPath);

// Deletes a given filename, return true on success
// Doesn't supports deleting a directory
bool Delete(const std::string& filename);

// Deletes a directory filename, returns true on success
bool DeleteDir(const std::string& filename);

// renames file srcFilename to destFilename, returns true on success
bool Rename(const std::string& srcFilename, const std::string& destFilename);

// copies file srcFilename to destFilename, returns true on success
bool Copy(const std::string& srcFilename, const std::string& destFilename);

// creates an empty file filename, returns true on success
bool CreateEmptyFile(const std::string& filename);

/**
 * @param num_entries_out to be assigned by the callable with the number of iterated directory
 * entries, never null
 * @param directory the path to the enclosing directory
 * @param virtual_name the entry name, without any preceding directory info
 * @return whether handling the entry succeeded
 */
using DirectoryEntryCallable = std::function<bool(
    unsigned* num_entries_out, const std::string& directory, const std::string& virtual_name)>;

/**
 * Scans a directory, calling the callback for each file/directory contained within.
 * If the callback returns failure, scanning halts and this function returns failure as well
 * @param num_entries_out assigned by the function with the number of iterated directory entries,
 * can be null
 * @param directory the directory to scan
 * @param callback The callback which will be called for each entry
 * @return whether scanning the directory succeeded
 */
bool ForeachDirectoryEntry(unsigned* num_entries_out, const std::string& directory,
                           DirectoryEntryCallable callback);

/**
 * Scans the directory tree, storing the results.
 * @param directory the parent directory to start scanning from
 * @param parent_entry FSTEntry where the filesystem tree results will be stored.
 * @param recursion Number of children directories to read before giving up.
 * @return the total number of files/directories found
 */
unsigned ScanDirectoryTree(const std::string& directory, FSTEntry& parent_entry,
                           unsigned int recursion = 0);

// deletes the given directory and anything under it. Returns true on success.
bool DeleteDirRecursively(const std::string& directory, unsigned int recursion = 256);

// Returns the current directory
std::string GetCurrentDir();

// Create directory and copy contents (does not overwrite existing files)
void CopyDir(const std::string& source_path, const std::string& dest_path);

// Set the current directory to given directory
bool SetCurrentDir(const std::string& directory);

// Returns a pointer to a string with a yuzu data dir in the user's home
// directory. To be used in "multi-user" mode (that is, installed).
const std::string& GetUserPath(const unsigned int DirIDX, const std::string& newPath = "");

// Returns the path to where the sys file are
std::string GetSysDirectory();

#ifdef __APPLE__
std::string GetBundleDirectory();
#endif

#ifdef _WIN32
std::string& GetExeDirectory();
std::string AppDataRoamingDirectory();
#endif

size_t WriteStringToFile(bool text_file, const std::string& str, const char* filename);
size_t ReadFileToString(bool text_file, const char* filename, std::string& str);

/**
 * Splits the filename into 8.3 format
 * Loosely implemented following https://en.wikipedia.org/wiki/8.3_filename
 * @param filename The normal filename to use
 * @param short_name A 9-char array in which the short name will be written
 * @param extension A 4-char array in which the extension will be written
 */
void SplitFilename83(const std::string& filename, std::array<char, 9>& short_name,
                     std::array<char, 4>& extension);

// simple wrapper for cstdlib file functions to
// hopefully will make error checking easier
// and make forgetting an fclose() harder
class IOFile : public NonCopyable {
public:
    IOFile();
    IOFile(const std::string& filename, const char openmode[]);

    ~IOFile();

    IOFile(IOFile&& other) noexcept;
    IOFile& operator=(IOFile&& other) noexcept;

    void Swap(IOFile& other) noexcept;

    bool Open(const std::string& filename, const char openmode[]);
    bool Close();

    template <typename T>
    size_t ReadArray(T* data, size_t length) const {
        static_assert(std::is_trivially_copyable<T>(),
                      "Given array does not consist of trivially copyable objects");

        if (!IsOpen())
            return -1;

        return std::fread(data, sizeof(T), length, m_file);
    }

    template <typename T>
    size_t WriteArray(const T* data, size_t length) {
        static_assert(std::is_trivially_copyable<T>(),
                      "Given array does not consist of trivially copyable objects");
        if (!IsOpen())
            return -1;
        return std::fwrite(data, sizeof(T), length, m_file);
    }

    template <typename T>
    size_t ReadBytes(T* data, size_t length) const {
        static_assert(std::is_trivially_copyable<T>(), "T must be trivially copyable");
        return ReadArray(reinterpret_cast<char*>(data), length);
    }

    template <typename T>
    size_t WriteBytes(const T* data, size_t length) {
        static_assert(std::is_trivially_copyable<T>(), "T must be trivially copyable");
        return WriteArray(reinterpret_cast<const char*>(data), length);
    }

    template <typename T>
    size_t WriteObject(const T& object) {
        static_assert(!std::is_pointer<T>::value, "Given object is a pointer");
        return WriteArray(&object, 1);
    }

    bool IsOpen() const {
        return nullptr != m_file;
    }

    bool Seek(s64 off, int origin) const;
    u64 Tell() const;
    u64 GetSize() const;
    bool Resize(u64 size);
    bool Flush();

    // clear error state
    void Clear() {
        std::clearerr(m_file);
    }

private:
    std::FILE* m_file = nullptr;
};

} // namespace FileUtil

// To deal with Windows being dumb at unicode:
template <typename T>
void OpenFStream(T& fstream, const std::string& filename, std::ios_base::openmode openmode) {
#ifdef _MSC_VER
    fstream.open(Common::UTF8ToTStr(filename).c_str(), openmode);
#else
    fstream.open(filename.c_str(), openmode);
#endif
}
