#ifndef QT_LOG_SINK_H
#define QT_LOG_SINK_H

#include "core/base/log.h"

namespace links {
namespace qt_adapter {

/**
 * Forwards core log records into the existing Qt Logger, so file location,
 * format and QMutex serialisation are unchanged.
 */
class QtLogSink : public core::LogSink {
public:
    void write(core::LogLevel level, const std::string& message) override;
};

}  // namespace qt_adapter
}  // namespace links

#endif  // QT_LOG_SINK_H
