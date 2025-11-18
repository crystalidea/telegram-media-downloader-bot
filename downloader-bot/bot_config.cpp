#include "pch.h"
#include "bot_config.h"

#define SHARED_FOLDER_NAME "bot_shared"
#define COOKIES_FILE    "www.instagram.com_cookies.txt"
#define TOKEN_FILE      "token.txt"

BotConfig::BotConfig()
{
    
}

bool BotConfig::read(std::shared_ptr<spdlog::logger> logger)
{
    QString cookieTest = path() + QDir::separator() + COOKIES_FILE;
    QString tokenTest = path() + QDir::separator() + TOKEN_FILE;

    if (QFile(cookieTest).exists())
        _cookiesFile = cookieTest;
    else
    {
        logger->error("Cannot proceed without a cookies file '{}' doesn't exist", cookieTest);

        return false;
    }

    QFile file(tokenTest);

    if (file.open(QIODevice::ReadOnly)) 
        _token = file.readLine().trimmed();
    else
    {
        logger->critical("Failed to read {}", tokenTest);
    }
    
    return _token.size();
}

QString BotConfig::path() // static
{
    // on Windows we place logs in the same folder as executable
    // on Linux we use ~/logs since the app runs in Docker

    QString logsPath;

#ifdef Q_OS_WIN

    logsPath = QDir::toNativeSeparators(QCoreApplication::applicationDirPath());

#else

    logsPath = QDir::homePath();
    
#endif

    logsPath += QDir::separator();
    logsPath += SHARED_FOLDER_NAME;

    return logsPath;
}
