#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <exception>
#define LOG Logger::instance().log
#define ERR Logger::instance().error
#define FORMAT fmt::format
class Logger
{
private:
    Logger();
    std::shared_ptr<spdlog::logger> spdLogger;

public:
    static Logger &instance();
    void log(const std::string &msg);
    void error(const std::string &msg, bool fatal = true);
};