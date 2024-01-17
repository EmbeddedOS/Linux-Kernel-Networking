#pragma once
#include <assert.h>
#include <string>
#include <iostream>
#include <sstream>

namespace larva
{
    class logger
    {
    public:
        enum class level
        {
            debug = 0,
            info,
            warn,
            error,
            fatal
        };

    private:
        std::ostringstream _str;
        logger::level _level;

    public:
        logger(logger::level level,
               const char *file,
               int line,
               const char *function) : _level{level}
        {
            this->_str << "[" << level_to_string(level) << "] "
                       << file << ":" << line << " " << function << "(): ";
        }

        ~logger()
        {
            std::cerr << std::boolalpha << this->_str.str() << std::endl;
            if (this->_level == logger::level::fatal)
            {
                abort();
            }
        }

        template <typename T>
        logger &operator<<(T data)
        {
            this->_str << data;
            return *this;
        }

    private:
        std::string level_to_string(logger::level level)
        {
            switch (level)
            {
            case logger::level::debug:
                return "DEBUG";
            case logger::level::info:
                return "INFO";
            case logger::level::warn:
                return "WARN";
            case logger::level::error:
                return "ERROR";
            case logger::level::fatal:
                return "FATAL";
            }

            return "";
        }
    };
}

#define debug() larva::logger(larva::logger::level::debug, \
                              __FILE__,                    \
                              __LINE__,                    \
                              __func__)

#define info() larva::logger(larva::logger::level::info, \
                             __FILE__,                   \
                             __LINE__,                   \
                             __func__)

#define warn() larva::logger(larva::logger::level::warn, \
                             __FILE__,                   \
                             __LINE__,                   \
                             __func__)

#define error() larva::logger(larva::logger::level::error, \
                              __FILE__,                    \
                              __LINE__,                    \
                              __func__)

#define fatal() larva::logger(larva::logger::level::fatal, \
                              __FILE__,                    \
                              __LINE__,                    \
                              __func__)
