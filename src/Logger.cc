#include "Logger.h"
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/RollingFileAppender.hh>
#include <log4cpp/PatternLayout.hh>
#include <log4cpp/Priority.hh>
#include <iostream>

Logger* Logger::_pInstance = nullptr;

Logger::Logger()
    : _category(nullptr)
    , _initialized(false) {
}

Logger::~Logger() {
    if (_initialized) {
        log4cpp::Category::shutdown();
    }
}

Logger* Logger::getInstance() {
    if (_pInstance == nullptr) {
        _pInstance = new Logger();
    }
    return _pInstance;
}

void Logger::init(const string& configPath) {
    if (_initialized) {
        return;
    }

    try {
        // 尝试从配置文件加载
        log4cpp::PropertyConfigurator::configure(configPath);
        _category = &log4cpp::Category::getRoot();
        _initialized = true;
    } catch (log4cpp::ConfigureFailure& e) {
        // 配置文件加载失败，使用默认配置
        std::cerr << "Warning: Failed to load log config: " << e.what() << "\n";
        std::cerr << "Using default log configuration...\n";

        // 创建控制台输出
        log4cpp::PatternLayout* consoleLayout = new log4cpp::PatternLayout();
        consoleLayout->setConversionPattern("%d{%Y-%m-%d %H:%M:%S} [%p] %m%n");

        log4cpp::OstreamAppender* consoleAppender =
            new log4cpp::OstreamAppender("console", &std::cout);
        consoleAppender->setLayout(consoleLayout);

        // 创建文件输出（滚动日志，最大10MB，保留5个备份）
        log4cpp::PatternLayout* fileLayout = new log4cpp::PatternLayout();
        fileLayout->setConversionPattern("%d{%Y-%m-%d %H:%M:%S} [%p] %m%n");

        log4cpp::RollingFileAppender* fileAppender =
            new log4cpp::RollingFileAppender("file", "logs/search_engine.log",
                                              10 * 1024 * 1024, 5);
        fileAppender->setLayout(fileLayout);

        // 配置根日志器
        _category = &log4cpp::Category::getRoot();
        _category->setPriority(log4cpp::Priority::DEBUG);
        _category->addAppender(consoleAppender);
        _category->addAppender(fileAppender);

        _initialized = true;
    }
}

void Logger::debug(const string& msg) {
    if (!_initialized) {
        init();
    }
    _category->debug(msg);
}

void Logger::info(const string& msg) {
    if (!_initialized) {
        init();
    }
    _category->info(msg);
}

void Logger::warn(const string& msg) {
    if (!_initialized) {
        init();
    }
    _category->warn(msg);
}

void Logger::error(const string& msg) {
    if (!_initialized) {
        init();
    }
    _category->error(msg);
}

void Logger::fatal(const string& msg) {
    if (!_initialized) {
        init();
    }
    _category->fatal(msg);
}
