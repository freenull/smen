#ifndef SMEN_LOGGER_HPP
#define SMEN_LOGGER_HPP

#include <string>
#include <iostream>

namespace smen {
    class Logger {
    private:
        std::ostream & _ostream;

    public:
        enum class Level {
            DEBUG,
            INFO,
            WARN,
            ERROR
        };

        const std::string name;
        const Level min_level;

        Logger(
            std::ostream & ostream,
            const std::string name, const Level min_level = Level::DEBUG
        )
        : _ostream(ostream)
        , name(name)
        , min_level(min_level)
        {}

        inline bool can_print(Level level) const {
            return static_cast<int>(level) >= static_cast<int>(min_level);
        }

        inline const std::string level_name(Level level) const {
            switch(level) {
            case Level::DEBUG: return std::string("DEBUG");
            case Level::INFO: return std::string("INFO");
            case Level::WARN: return std::string("WARN");
            case Level::ERROR: return std::string("ERROR");
            default: return std::string("?");
            }
        }

        inline void write_prefix(Level level) const {
            _ostream << "[" << name << " " << level_name(level) << "]";
        }

        template <typename... Args>
        void print(Level level, Args &&... args) const {
            if (!can_print(level)) return;

            write_prefix(level);
            _ostream << " ";
            (_ostream << ... << std::forward<Args>(args));
            _ostream << "\n";
        }

        template <typename... T>
        void debug(T &&... args) const {
            print(Level::DEBUG, args...);
        }

        template <typename... T>
        void info(T &&... args) const {
            print(Level::INFO, args...);
        }

        template <typename... T>
        void warn(T &&... args) const {
            print(Level::WARN, args...);
        }

        template <typename... T>
        void error(T &&... args) const {
            print(Level::WARN, args...);
        }
    };

    inline const Logger make_logger(std::string name) {
        return Logger(std::cerr, name, Logger::Level::DEBUG);
    }
};

#endif//SMEN_LOGGER_HPP
