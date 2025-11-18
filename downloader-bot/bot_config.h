#pragma once

class BotConfig
{
public:

    BotConfig();

    bool read(std::shared_ptr<spdlog::logger>);

    QString getToken() const {
        return _token;
    }

    QString getCookiesFile() const {
        return _cookiesFile;
    }

    static QString path();

private:

    QString _cookiesFile;
    QString _token;
};