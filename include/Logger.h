#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <string>
#include <log4cpp/Category.hh>
#include <log4cpp/PropertyConfigurator.hh>

using std::string;

// 日志管理器：单例模式封装 log4cpp
class Logger {
public:
    static Logger* getInstance();

    // 初始化日志系统（从配置文件加载）
    void init(const string& configPath = "conf/log4cpp.properties");

    // 日志输出接口
    void debug(const string& msg);
    void info(const string& msg);
    void warn(const string& msg);
    void error(const string& msg);
    void fatal(const string& msg);

    // 获取底层 Category（高级用法）
    log4cpp::Category& getCategory() { return *_category; }

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    static Logger* _pInstance;
    log4cpp::Category* _category;
    bool _initialized;
};

// 便捷宏定义：简化日志调用
#define LOG_DEBUG(msg) Logger::getInstance()->debug(msg)
#define LOG_INFO(msg)  Logger::getInstance()->info(msg)
#define LOG_WARN(msg)  Logger::getInstance()->warn(msg)
#define LOG_ERROR(msg) Logger::getInstance()->error(msg)
#define LOG_FATAL(msg) Logger::getInstance()->fatal(msg)

#endif // __LOGGER_H__
