#ifndef CORE_BASE_LOG_H
#define CORE_BASE_LOG_H

#include <string>

namespace links {
namespace core {

enum class LogLevel { Debug, Info, Warning, Error };

/**
 * Receives log records. Implementations must be thread-safe: core logs from
 * LiveKit SDK threads, media reader threads and the main thread.
 */
class LogSink {
public:
    virtual ~LogSink() = default;
    virtual void write(LogLevel level, const std::string& message) = 0;
};

/**
 * Install the process-wide sink. Pass nullptr to fall back to stderr.
 * Call once from main() before anything logs.
 *
 * This is deliberately ambient rather than injected: there are ~200 call sites
 * in core/, including free functions in anonymous namespaces, and threading a
 * logger reference through all of them would distort every signature for a
 * genuinely cross-cutting concern. Tests install a capturing sink.
 */
void setLogSink(LogSink* sink);

void log(LogLevel level, const std::string& message);

inline void logDebug(const std::string& m)   { log(LogLevel::Debug, m); }
inline void logInfo(const std::string& m)    { log(LogLevel::Info, m); }
inline void logWarning(const std::string& m) { log(LogLevel::Warning, m); }
inline void logError(const std::string& m)   { log(LogLevel::Error, m); }

}  // namespace core
}  // namespace links

#endif  // CORE_BASE_LOG_H
