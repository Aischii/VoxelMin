#pragma once
#include <cstdarg>
#include <cstdio>

// Tiny printf-style logging helpers. No external dependency and no log levels
// to configure -- good enough for an alpha-stage learning project.
namespace vox::log {

inline void info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::printf("[INFO ] ");
    std::vprintf(fmt, args);
    std::printf("\n");
    va_end(args);
}

inline void warn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::printf("[WARN ] ");
    std::vprintf(fmt, args);
    std::printf("\n");
    va_end(args);
}

inline void error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::printf("[ERROR] ");
    std::vprintf(fmt, args);
    std::printf("\n");
    va_end(args);
}

} // namespace vox::log
