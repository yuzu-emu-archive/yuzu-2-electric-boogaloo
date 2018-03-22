// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <array>
#include <cstdio>
#include "common/assert.h"
#include "common/common_funcs.h" // snprintf compatibility define
#include "common/logging/backend.h"
#include "common/logging/filter.h"
#include "common/logging/log.h"
#include "common/logging/text_formatter.h"
#include "common/string_util.h"

namespace Log {

/// Macro listing all log classes. Code should define CLS and SUB as desired before invoking this.
#define ALL_LOG_CLASSES()                                                                          \
    CLS(Log)                                                                                       \
    CLS(Common)                                                                                    \
    SUB(Common, Filesystem)                                                                        \
    SUB(Common, Memory)                                                                            \
    CLS(Core)                                                                                      \
    SUB(Core, ARM)                                                                                 \
    SUB(Core, Timing)                                                                              \
    CLS(Config)                                                                                    \
    CLS(Debug)                                                                                     \
    SUB(Debug, Emulated)                                                                           \
    SUB(Debug, GPU)                                                                                \
    SUB(Debug, Breakpoint)                                                                         \
    SUB(Debug, GDBStub)                                                                            \
    CLS(Kernel)                                                                                    \
    SUB(Kernel, SVC)                                                                               \
    CLS(Service)                                                                                   \
    SUB(Service, ACC)                                                                              \
    SUB(Service, Audio)                                                                            \
    SUB(Service, AM)                                                                               \
    SUB(Service, AOC)                                                                              \
    SUB(Service, APM)                                                                              \
    SUB(Service, Friend)                                                                           \
    SUB(Service, FS)                                                                               \
    SUB(Service, HID)                                                                              \
    SUB(Service, LM)                                                                               \
    SUB(Service, NIFM)                                                                             \
    SUB(Service, NS)                                                                               \
    SUB(Service, NVDRV)                                                                            \
    SUB(Service, PCTL)                                                                             \
    SUB(Service, SET)                                                                              \
    SUB(Service, SM)                                                                               \
    SUB(Service, Time)                                                                             \
    SUB(Service, VI)                                                                               \
    CLS(HW)                                                                                        \
    SUB(HW, Memory)                                                                                \
    SUB(HW, LCD)                                                                                   \
    SUB(HW, GPU)                                                                                   \
    SUB(HW, AES)                                                                                   \
    CLS(IPC)                                                                                       \
    CLS(Frontend)                                                                                  \
    CLS(Render)                                                                                    \
    SUB(Render, Software)                                                                          \
    SUB(Render, OpenGL)                                                                            \
    CLS(Audio)                                                                                     \
    SUB(Audio, DSP)                                                                                \
    SUB(Audio, Sink)                                                                               \
    CLS(Input)                                                                                     \
    CLS(Network)                                                                                   \
    CLS(Loader)                                                                                    \
    CLS(WebService)

// GetClassName is a macro defined by Windows.h, grrr...
const char* GetLogClassName(Class log_class) {
    switch (log_class) {
#define CLS(x)                                                                                     \
    case Class::x:                                                                                 \
        return #x;
#define SUB(x, y)                                                                                  \
    case Class::x##_##y:                                                                           \
        return #x "." #y;
        ALL_LOG_CLASSES()
#undef CLS
#undef SUB
    case Class::Count:
        UNREACHABLE();
    }
}

const char* GetLevelName(Level log_level) {
#define LVL(x)                                                                                     \
    case Level::x:                                                                                 \
        return #x
    switch (log_level) {
        LVL(Trace);
        LVL(Debug);
        LVL(Info);
        LVL(Warning);
        LVL(Error);
        LVL(Critical);
    case Level::Count:
        UNREACHABLE();
    }
#undef LVL
}

Entry CreateEntry(Class log_class, Level log_level, const char* filename, unsigned int line_nr,
                  const char* function, std::string message) {
    using std::chrono::duration_cast;
    using std::chrono::steady_clock;

    static steady_clock::time_point time_origin = steady_clock::now();

    Entry entry;
    entry.timestamp = duration_cast<std::chrono::microseconds>(steady_clock::now() - time_origin);
    entry.log_class = log_class;
    entry.log_level = log_level;
    entry.filename = Common::TrimSourcePath(filename);
    entry.line_num = line_nr;
    entry.function = function;
    entry.message = std::move(message);

    return entry;
}

static Filter* filter = nullptr;

void SetFilter(Filter* new_filter) {
    filter = new_filter;
}

void LogMessage(Class log_class, Level log_level, const char* filename, unsigned int line_num,
                const char* function, const char* format, ...) {
    if (filter && !filter->CheckMessage(log_class, log_level))
        return;
    std::array<char, 4 * 1024> formatting_buffer;
    va_list args;
    va_start(args, format);
    vsnprintf(formatting_buffer.data(), formatting_buffer.size(), format, args);
    va_end(args);
    Entry entry = CreateEntry(log_class, log_level, filename, line_num, function,
                              std::string(formatting_buffer.data()));

    PrintColoredMessage(entry);
}

void FmtLogMessage(Class log_class, Level log_level, const char* filename, unsigned int line_num,
                   const char* function, const char* format, const fmt::ArgList& args) {
    if (filter && !filter->CheckMessage(log_class, log_level))
        return;
    Entry entry =
        CreateEntry(log_class, log_level, filename, line_num, function, fmt::format(format, args));

    PrintColoredMessage(entry);
}
} // namespace Log
