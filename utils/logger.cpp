#include "logger.h"
#include <QStandardPaths>
#include <QDir>
#include <iostream>

Logger& Logger::instance()
{
    static Logger instance;
    return instance;
}

Logger::Logger() : initialized_(false)
{
}

Logger::~Logger()
{
    if (logFile_.isOpen()) {
        logFile_.close();
    }
}

void Logger::init()
{
    if (initialized_) {
        return;
    }
    
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(logDir);
    
    QString logPath = logDir + "/conference.log";
    logFile_.setFileName(logPath);
    
    if (logFile_.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        logStream_.setDevice(&logFile_);
        initialized_ = true;
        log("Logger initialized");
    }
}

void Logger::log(const QString& message, const QString& level)
{
    writeLog(level, message);
}

void Logger::debug(const QString& message)
{
    writeLog("DEBUG", message);
}

void Logger::info(const QString& message)
{
    writeLog("INFO", message);
}

void Logger::warning(const QString& message)
{
    writeLog("WARNING", message);
}

void Logger::error(const QString& message)
{
    writeLog("ERROR", message);
}

void Logger::writeLog(const QString& level, const QString& message)
{
    QMutexLocker locker(&mutex_);
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString logMessage = QString("[%1] [%2] %3").arg(timestamp, level, message);
    
    // Write to console
    std::cout << logMessage.toStdString() << std::endl;
    
    // Write to file if initialized
    if (initialized_ && logFile_.isOpen()) {
        logStream_ << logMessage << "\n";
        logStream_.flush();
    }
}
