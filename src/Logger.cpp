#include <simple-animation-blender/Logger.h>
using namespace spdlog;
Logger::Logger()
{
    spdLogger = basic_logger_st("Logger", "log.txt", true);
}
Logger &Logger::instance()
{
    static Logger logger;
    return logger;
}
void Logger::log(const std::string &msg)
{
    spdLogger->info(msg);
}
void Logger::error(const std::string &msg, bool fatal)
{
    spdLogger->error(msg);
    if (fatal)
    {
        spdLogger->flush();
        throw std::exception();
    }
}