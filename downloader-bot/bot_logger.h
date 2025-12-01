#pragma once

class BotLogger
{
public:

    static std::shared_ptr<spdlog::logger> create();

    static QString getCurrentLogFilePath();
};