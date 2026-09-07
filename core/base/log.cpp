#include "log.h"

#include <atomic>
#include <cstdio>

namespace links {
namespace core {
namespace {

std::atomic<LogSink*> g_sink{nullptr};

const char* levelName(LogLevel level)
{
    switch (level) {
    case LogLevel::Debug:   return "DEBUG";
    case LogLevel::Info:    return "INFO";
    case LogLevel::Warning: return "WARNING";
    case LogLevel::Error:   return "ERROR";
    }
    return "INFO";
}

}  // namespace

void setLogSink(LogSink* sink)
{
    g_sink.store(sink, std::memory_order_release);
}

void log(LogLevel level, const std::string& message)
{
    if (LogSink* sink = g_sink.load(std::memory_order_acquire)) {
        sink->write(level, message);
        return;
    }
    // No sink installed yet (early startup, or a unit test). Matches what
    // Logger does before init(): write to the console and carry on.
    std::fprintf(stderr, "[%s] %s\n", levelName(level), message.c_str());
}

}  // namespace core
}  // namespace links
