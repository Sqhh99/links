#include "qt_log_sink.h"

#include <QString>

#include "../../../utils/logger.h"

namespace links {
namespace qt_adapter {

void QtLogSink::write(core::LogLevel level, const std::string& message)
{
    const QString text = QString::fromStdString(message);
    switch (level) {
    case core::LogLevel::Debug:
        Logger::instance().debug(text);
        break;
    case core::LogLevel::Info:
        Logger::instance().info(text);
        break;
    case core::LogLevel::Warning:
        Logger::instance().warning(text);
        break;
    case core::LogLevel::Error:
        Logger::instance().error(text);
        break;
    }
}

}  // namespace qt_adapter
}  // namespace links
