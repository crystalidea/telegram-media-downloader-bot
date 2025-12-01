#include "pch.h"
#include "bot_logger.h"
#include "bot_config.h"

#include <iostream>

#define LOG_FILENAME_FORMAT     "logs/%Y-%m-%d.txt"
#define LOG_PATTERN             "[%H:%M:%S:%e] [thread %t] [%^%l%$] %v"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>

#ifdef Q_OS_WIN
#include <windows.h>
static inline LPCWSTR logPathFromQString(const QString& str) { return reinterpret_cast<LPCWSTR>(str.utf16()); }
#else
#define logPathFromQString(s) s.toUtf8().constData()
#endif

std::shared_ptr<spdlog::logger> BotLogger::create()
{
    std::shared_ptr<spdlog::logger> _logger;

    try
    {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);
        console_sink->set_pattern(LOG_PATTERN);

#ifdef Q_OS_WIN
        ::SetConsoleOutputCP(CP_UTF8);
#endif

        try
        {
            QString logsFile = BotConfig::path();
            logsFile += QDir::separator();
            logsFile += LOG_FILENAME_FORMAT;

            auto file_sink = std::make_shared<spdlog::sinks::daily_file_format_sink_mt>(logPathFromQString(logsFile), 0, 0);

            auto file_name = file_sink->filename();

            file_sink->set_level(spdlog::level::trace);
            file_sink->set_pattern(LOG_PATTERN);

            _logger.reset(new spdlog::logger("downloader-bot", { console_sink, file_sink }));
        }
        catch (const std::exception& ec)
        {
            _logger.reset(new spdlog::logger("downloader-bot", { console_sink }));

            _logger->warn("Unable to create file logger, {}", ec.what());
        }

        _logger->set_level(spdlog::level::trace);

        _logger->flush_on(spdlog::level::trace);
    }
    catch (const std::exception& ec)
    {
        std::cerr << ec.what();
    }

    return _logger;
}

QString BotLogger::getCurrentLogFilePath()
{
    QString logFilePath = BotConfig::path();
    logFilePath += QDir::separator();
    logFilePath += "logs";
    logFilePath += QDir::separator();
    logFilePath += QDate::currentDate().toString("yyyy-MM-dd");
    logFilePath += ".txt";

    return logFilePath;
}