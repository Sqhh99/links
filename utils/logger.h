#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>

class Logger
{
public:
    static Logger& instance();
    
    void init();
    void log(const QString& message, const QString& level = "INFO");
    void debug(const QString& message);
    void info(const QString& message);
    void warning(const QString& message);
    void error(const QString& message);
    
private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void writeLog(const QString& level, const QString& message);
    
    QFile logFile_;
    QTextStream logStream_;
    QMutex mutex_;
    bool initialized_;
};

#endif // LOGGER_H
