#pragma once

#include <future>
#include <memory>

#include "download_info.h"
#include "bot_status.h"
#include "bot_config.h"

#include <TelegramBotAPI.h>

#include <BS_thread_pool.hpp>

#include "gallery_downloader.h"

const static int RESTART_WAIT_SECONDS = 10;
const static int RESTART_MAX_COUNT = 100; // which means 1000 seconds or ~17 minutes of trying

class QTelegramDownloaderBot : public QCoreApplication
{
    Q_OBJECT

public:

    QTelegramDownloaderBot(int& argc, char** argv);
    ~QTelegramDownloaderBot();

    bool start();

    void stop();

    std::shared_ptr<spdlog::logger> getLogger() const;

signals:

    void fatalError(qint32 errorCode);

protected slots:

    void errorOccured(Telegram::Error error);

protected:

    void setAvailableCommands();

    QString getStartText() const;

    void download_thread(const QString& url, qint64 chat_id, bool group_message, qint32 originalMessageID);

    bool isSupportedWebsite(const QString& url) const;

    void pollingThread();

    std::future<Telegram::Message> sendMessage(const qint64& chat_id, const QString& msg, bool silent = false, bool markdown = false);
    std::future<Telegram::Message> sendMessage(const qint64& chat_id, const std::string& msg, bool silent = false, bool markdown = false) {
        return sendMessage(chat_id, QString::fromUtf8(msg), silent, markdown);
    }
    std::future<Telegram::Message> sendMessage(const qint64& chat_id, const char * msg, bool silent = false, bool markdown = false) {
        return sendMessage(chat_id, QString::fromUtf8(msg), silent, markdown);
    }

    bool sendVideo(const qint64& chat_id, const DownloadInfoItem&di);
    
    bool sendPhoto(const qint64& chat_id, const QString& file, const QString &description);
    bool sendPhoto(const qint64& chat_id, const GalleryItem& file);

    bool sendGallery(const qint64& chat_id, const QList< GalleryItem>& files, const QString& url);

    void deleteMessage(const qint64& chat_id, const qint32& message_id);

    void editMessage(const qint64& chat_id, const qint32& message_id, const QString &msg);
    void editMessage(const qint64& chat_id, const qint32& message_id, const std::string& msg) {
        return editMessage(chat_id, message_id, QString::fromUtf8(msg));
    }

private:

    std::future<void> _pollThread;

    std::unique_ptr<Telegram::Bot> _bot;

    std::atomic<bool> _quitEvent = false;
    std::atomic<bool> _pollThreadFinished = false;

    std::shared_ptr<spdlog::logger> _logger;

    BS::thread_pool pool;

    QHash<qint64, QStringList > _queuedUrls;
    QMutex _queuedUrlsMutex;

    BotConfig _config;
    BotStatus _status;
};