#include "pch.h"
#include "bot.h"
#include "video_downloader.h"
#include "gallery_downloader.h"
#include "helpers.h"
#include "bot_logger.h"
#include "ffmpeg.h"

#include <iostream>

using namespace Telegram;

QTelegramDownloaderBot::QTelegramDownloaderBot(int& argc, char** argv)
    : QCoreApplication(argc, argv)
{
    
}

QTelegramDownloaderBot::~QTelegramDownloaderBot()
{
    getLogger()->trace("QTelegramDownloaderBot destroyed");
}

std::shared_ptr<spdlog::logger> QTelegramDownloaderBot::getLogger() const
{
    return _logger;
}

QString getMessageAuthor(const Telegram::Message &msg)
{
    QString from;

    if (msg.from.has_value())
    {
        Telegram::User u = msg.from.value();

        std::string f = fmt::format("'{}{}' [id={}{})", u.first_name,
            u.last_name.has_value() ? fmt::format(" {}", u.last_name.value()) : "",
            u.id,
            u.username.has_value() ? fmt::format(", username={}", u.username.value()) : "");

        from = QString::fromUtf8(f);
    }

    return from;
}

bool isMessageFromGroupChat(const Telegram::Message& msg)
{
    return (msg.chat->type == Chat::Type::GROUP);
}

static const qint64 maxTelegramBotFileSizeBytes = 50 * 1024 * 1024; // 50 MB

void QTelegramDownloaderBot::setAvailableCommands()
{
    //  List of bot commands 
    QVector<BotCommand> bot_commands = {
        {"status", "View bot status"},
        {"log", "Send log file"}
    };

    _bot->setMyCommands(bot_commands);
}

QString QTelegramDownloaderBot::getStartText() const
{
    return "This bot can be used to download Instagram posts/reels, Youtube videos/shorts, Facebook share links or Twitter videos. "
        "Send me a link to test but keep in mind I'm currently in a very early beta.";
}

void QTelegramDownloaderBot::download_thread(const QString &url, qint64 chat_id, bool isGroupChat, qint32 originalMessageID)
{
    const bool sendChatMessage = (isGroupChat == false);

    // TODO: we could queue only upload tasks, dowload can be simultaneous

    {
        QMutexLocker locker(&_queuedUrlsMutex);

        _queuedUrls[chat_id].push_back(url);

        if (_queuedUrls[chat_id].size() > 1) // there's at least one more
        {
            return; // download will start in the previous thread
        }
    }

    QString currentUrl = url;

    while (true) // for multiple urls of single user
    {
        UrlType type = Helpers::getUrlType(currentUrl);

        if (type == UrlType::InstagramPost)
        {
            QGalleryDownloader downloader;
            downloader.setCookies(_config.getCookiesFile());

            bool downloadOK = false; 
            getLogger()->info("Downloading...");

            GalleryInfo gi;
            
            if (sendChatMessage)
            {
                auto downloadingMessage = sendMessage(chat_id, "Downloading...", true);
                downloadingMessage.wait();

                downloadOK = downloader.downloadUrl(currentUrl, gi, getLogger());

                deleteMessage(chat_id, downloadingMessage.get().message_id); // delete Downloading...
            }
            else
                downloadOK = downloader.downloadUrl(currentUrl, gi, getLogger());

            if (_quitEvent)
                break;

            if (downloadOK)
            {
                QList<GalleryItem> galleryFiles = gi.getFiles();

                getLogger()->info("Done ({} files)", galleryFiles.size());

                quint64 totalSize = gi.getTotalFileSize();
                
                _status.statsUpdateFileDownloaded(totalSize);

                getLogger()->info("Uploading...");

                bool bUploaded = false;

                if (galleryFiles.size() > 1)
                {
                    bUploaded = sendGallery(chat_id, galleryFiles, currentUrl);
                }
                else if (galleryFiles.size() == 1)
                {
                    bUploaded = sendPhoto(chat_id, galleryFiles[0]);
                }
                else
                {
                    getLogger()->warn("No files downloaded");
                }

                if (bUploaded)
                {
                    getLogger()->info("Done");

                    // delete the original message
                    // we'll save the url in the description

                    deleteMessage(chat_id, originalMessageID);
                }
            }
            else
            {
                sendMessage(chat_id, "Error: no files downloaded");

                getLogger()->warn("Failed to download");
            }
        }
        else
        {
            QMediaDownloader downloader;

            downloader.setCookies(_config.getCookiesFile());

            DownloadInfo di;
            QString outputLog;

            bool downloadOK = false;
            getLogger()->info("Downloading...");

            if (sendChatMessage)
            {
                auto downloadingMessage = sendMessage(chat_id, "Downloading...", true);
                downloadingMessage.wait();

                downloadOK = downloader.downloadUrl(currentUrl, di, getLogger());

                deleteMessage(chat_id, downloadingMessage.get().message_id); // delete Downloading...
            }
            else
                downloadOK = downloader.downloadUrl(currentUrl, di, getLogger());
            
            if (_quitEvent)
                break;

            if (downloadOK && di.size())
            {
                // TODO: send as gallery?

                for (int i = 0; i < di.size(); i++)
                {
                    DownloadInfoItem downloadedItem = di[i];

                    downloadedItem.videoTitle += QString::fromStdString(fmt::format("\n\n[Original URL]({})", currentUrl));

                    qint64 fileSize = QFile(downloadedItem.videoFilePath).size();
                    QString fileSizeFormatted = Helpers::formatSize(fileSize);

                    _status.statsUpdateFileDownloaded(fileSize);

                    QString resolution = QString("%1x%2").arg(downloadedItem.videoWidth).arg(downloadedItem.videoHeight);

                    getLogger()->info("Done ({}, {})", resolution, fileSizeFormatted);

                    if (fileSize > maxTelegramBotFileSizeBytes)
                    {
                        auto msgReencode = sendMessage(chat_id, fmt::format("The file is over 50MB ({}). Re-encoding to fit the limit...", fileSizeFormatted));

                        getLogger()->warn("File is over 50MB ({}). Attempting to transcode.", fileSizeFormatted);

                        auto compressedItem = ffmpeg::transcodeVideoToLimit(downloadedItem, maxTelegramBotFileSizeBytes, getLogger());

                        if (compressedItem.has_value())
                        {
                            downloadedItem = compressedItem.value();
                            fileSize = QFile(downloadedItem.videoFilePath).size();
                            fileSizeFormatted = Helpers::formatSize(fileSize);

                            getLogger()->info("Compressed file size: {}", fileSizeFormatted);

                            deleteMessage(chat_id, msgReencode.get().message_id);
                        }
                        else
                        {
                            sendMessage(chat_id, fmt::format("Error: failed to re-encode the video under 50MB ({}).", fileSizeFormatted));

                            getLogger()->warn("Failed to transcode video to fit 50MB limit ({}).", fileSizeFormatted);

                            continue;
                        }

                        if (fileSize > maxTelegramBotFileSizeBytes)
                        {
                            sendMessage(chat_id, fmt::format("Error: the file is over 50MB even after compression ({})", fileSizeFormatted));

                            getLogger()->warn(fmt::format("Error: the file is over 50MB even after compression ({})", fileSizeFormatted));

                            continue;
                        }
                    }
                    
                    getLogger()->info("Uploading...");

                    if (sendVideo(chat_id, downloadedItem))
                    {
                        getLogger()->info("Done");
                    }

                    // delete the original message
                    // we'll save the url in the description

                    deleteMessage(chat_id, originalMessageID);
                }
            }
            else
            {
                sendMessage(chat_id, "Error: failed to download");

                getLogger()->warn("Failed to download");
                getLogger()->warn(outputLog);
            }
        }

        // sync

        if (_quitEvent)
            break;

        {
            QMutexLocker locker(&_queuedUrlsMutex);

            _queuedUrls[chat_id].removeAll(currentUrl);

            if (_queuedUrls[chat_id].size()) // there's at least one more
            {
                currentUrl = _queuedUrls[chat_id].first();

                continue;
            }

            break;
        }
    }
}

void QTelegramDownloaderBot::pollingThread()
{
    _logger->info("pollingThread started");

    qint32 lastUpdateID = 0;

    while (_quitEvent == false)
    {
        QVector<QString> allowedUpdates = {"message"};
        auto updates_future = _bot->getUpdates(lastUpdateID + 1, std::nullopt, std::nullopt, allowedUpdates);

        updates_future.wait();

        // if you start a second instance, quit event will be triggered, but this thread can be still waiting 
        // we need to check it here once more and break

        if (_quitEvent)
            break;

        QVector<Telegram::Update> updates = updates_future.get();

        if (!updates.size())
            continue;

        for (auto update : updates)
        {
            if (update.update_id > lastUpdateID)
                lastUpdateID = update.update_id;

            if (update.message.has_value())
            {
                const QString from = getMessageAuthor(update.message.value());
                const bool isGroupChat = isMessageFromGroupChat(update.message.value());
                const bool loggerEnabled = (isGroupChat == false);
                const bool sendChatMessage = (isGroupChat == false);

                getLogger()->info("Message received from {}", from);

                if (update.message->text.has_value())
                {
                    qint32 originalMessageID = update.message->message_id;

                    const QString text = update.message->text.value();
                    const qint64 chat_id = update.message->chat->id;
                    
                    const bool supportedWebsite = isSupportedWebsite(text);
                    const bool validURL = QUrl(text).isValid();
                    
                    if (loggerEnabled)
                        getLogger()->info(text);

                    if (text == "/status")
                    {
                        if (sendChatMessage)
                            sendMessage(chat_id, _status.print(), false, true);
                    }
                    else if (text == "/start")
                    {
                        if (sendChatMessage)
                            sendMessage(chat_id, getStartText());
                    }
                    else if (text == "/log")
                    {
                        getLogger()->info("Sending log file");

                        if (!sendLogFile(chat_id) && sendChatMessage)
                        {
                            sendMessage(chat_id, "Failed to send the log file");
                        }
                    }
                    else if (validURL)
                    {
                        if (supportedWebsite)
                        {
                            auto downloadTask = pool.submit_task( [this, text, chat_id, isGroupChat, originalMessageID]{ download_thread(text, chat_id, isGroupChat, originalMessageID); });
                        }
                        else
                        {
                            if (loggerEnabled)
                                getLogger()->warn("This website is not supported");

                            if (sendChatMessage)
                                sendMessage(chat_id, "This website is not supported");
                        }
                    }
                    else
                    {
                        if (sendChatMessage)
                            sendMessage(chat_id, "Invalid URL specified.");

                        if (loggerEnabled)
                            getLogger()->warn("Invalid URL");
                    }
                }
                else
                {
                    if (loggerEnabled)
                        getLogger()->info("Ignore non-text message");
                }
            }
            else
            {
                getLogger()->info("Ignore event without message");
            }
        }

        if (_quitEvent)
            break;
    }

    _logger->info("pollingThread stopped");

    _pollThreadFinished = true;
    _pollThreadFinished.notify_all();
}

bool QTelegramDownloaderBot::isSupportedWebsite(const QString& url) const 
{
    UrlType t = Helpers::getUrlType(url);

    return (t != UrlType::Unknown);
}

bool QTelegramDownloaderBot::start()
{
    // make the executable path as current path

#ifdef Q_OS_WIN
    QDir::setCurrent(QCoreApplication::applicationDirPath()); // ?
#endif

    _logger = BotLogger::create();

    if (!_logger)
    {
        std::cerr << "Unable to proceed without logger.";

        return false;
    }

    if (!_config.read(_logger))
    {
        _logger->critical("Failed to read bot config");

        return false;
    }

    _logger->info("Starting with bot token {}...", _config.getToken().left(8) + "...");
    _logger->info("Current path: {}", QDir::currentPath());
    _logger->info("Config path: {}", _config.path());

    if (!_status.statsLoadFromFile(_logger))
    {
        _logger->warn("Failed to load stats from file");
    }

#if 0
    const QStringList args = arguments();

    for (const QString& arg : args) {
        //_logger->info("arg: {}", arg);
    }
#endif

    QString v1 = QMediaDownloader::ytdlpVersion();
    QString v2 = QMediaDownloader::ffmpegVersion();
    QString v3 = QGalleryDownloader::galleryVersion();

    if (v1.size())
        _logger->info("yt-dlp version: {}", v1);
    else
    {
        _logger->critical("Failed to get yt-dlp version");

        return false;
    }

    if (v2.size())
        _logger->info("ffmpeg version: {}", v2);
    else
    {
        _logger->critical("Failed to get ffmpeg version");

        return false;
    }

    if (v3.size())
        _logger->info("galery-dl version: {}", v3);
    else
    {
        _logger->critical("Failed to get galery-dl version");

        return false;
    }

    std::shared_ptr< BotSettings> settings = std::make_shared<BotSettings>(_config.getToken());

    _bot = std::make_unique<Telegram::Bot>(settings);

    QObject::connect(_bot.get(), &Bot::errorOccured, this, &QTelegramDownloaderBot::errorOccured);
    
    //	Telegram::Bot::networkErrorOccured is emitted every time when there is an error while receiving Updates from the Telegram. 
    // Error class contains the occurred error description and code. See Error.h for details

    QObject::connect(_bot.get(), &Bot::networkErrorOccured, [this](Error error) {
        _logger->critical("Network error {} occured: {}", error.error_code, error.description);
    });

    setAvailableCommands();

    _pollThread = std::async(std::launch::async, [this] { pollingThread(); });

    return true;
}

void QTelegramDownloaderBot::errorOccured(Telegram::Error error) // happens in the main thread
{
    static const QVector<qint32> fatalErrorCodes = { 0, 401, 409 }; // I checked those

    const bool fatalErrorCode = (fatalErrorCodes.contains(error.error_code));

    if (fatalErrorCode)
        stop();

    if (error.error_code == 0) // Internet is not available
    {
        _logger->critical("Error 0, trying to restart the bot, retrying in {} seconds", RESTART_WAIT_SECONDS);
    }
    else if (error.error_code == 401) // auth problem
    {
        _logger->critical("Please check if bot token is valid", error.error_code);
    }
    else if (error.error_code == 409) // conflict
    {
        _logger->critical("Terminated by another bot instance");
    }
    else
        _logger->warn("Error {} ({})", error.error_code, error.description);

    if (fatalErrorCode)
        emit fatalError(error.error_code);
}

void QTelegramDownloaderBot::stop()
{
    _quitEvent = true;

    if (!_pollThreadFinished && _pollThread.valid())
        _pollThreadFinished.wait(false); // blocks while _pollThreadFinished continues to be false [C++ 20]

    pool.wait();
}

std::future<Telegram::Message> QTelegramDownloaderBot::sendMessage(const qint64& chat_id, const QString &msg, bool silent, bool markdown)
{
    _bot->sendChatAction(chat_id, Bot::ChatActionType::TYPING);

    bool disable_web_page_preview = true;

    return _bot->sendMessage(chat_id, msg, markdown ? Message::FormattingType::MarkdownV2 : Message::FormattingType::HTML, std::nullopt, disable_web_page_preview, silent);
}

bool QTelegramDownloaderBot::sendVideo(const qint64& chat_id, const DownloadInfoItem& di)
{
    bool bSuccess = false;

    // https://core.telegram.org/bots/api#sendvideo
    // 50 MB limit

    auto video_file = std::make_shared<QFile>(di.videoFilePath);

    if (video_file->open(QIODevice::ReadOnly))
    {
        QString video_caption = di.videoTitle;

        _bot->sendChatAction(chat_id, Bot::ChatActionType::UPLOAD_VIDEO);

        if (di.videoWidth && di.videoHeight)
        {

        }
        else
        {
            _logger->warn("No video dimensions!");

            // TODO: use ffpeg to get video size
        }

        Telegram::Message v_msg = _bot->sendVideo({
            .chat_id = chat_id,
            .video = video_file.get(),
            .width = di.videoWidth,
            .height = di.videoHeight,
            .caption = video_caption,
            .parse_mode = Message::FormattingType::MarkdownV2 }
        ).get();

        bSuccess = (v_msg.isEmpty() == false);
    }

    return bSuccess;
}

void QTelegramDownloaderBot::deleteMessage(const qint64& chat_id, const qint32& message_id)
{
    _bot->deleteMessage(chat_id, message_id);
}

void QTelegramDownloaderBot::editMessage(const qint64& chat_id, const qint32& message_id, const QString& msg)
{
    _bot->editMessageText(msg, chat_id, message_id);
}

bool QTelegramDownloaderBot::sendPhoto(const qint64& chat_id, const QString& file, const QString& description)
{
    bool bSuccess = false;

    // TODO: 10 MB limit

    QString fixedPath = QDir::toNativeSeparators(file);

    auto img_file = std::make_shared<QFile>(fixedPath);

    if (img_file->open(QIODevice::ReadOnly))
    {
        QString img_caption;

        _bot->sendChatAction(chat_id, Bot::ChatActionType::UPLOAD_PHOTO);

        Telegram::Message v_msg = _bot->sendPhoto({
            .chat_id = chat_id,
            .photo = img_file.get(),
            .caption = description.left(1024), // 1024 limit
            .parse_mode = Message::FormattingType::MarkdownV2 }
        ).get();

        bSuccess = (v_msg.isEmpty() == false);
    }

    return bSuccess;
}

bool QTelegramDownloaderBot::sendPhoto(const qint64& chat_id, const GalleryItem& file)
{
    return sendPhoto(chat_id, file.getFile(), file.getDescription());
}

bool QTelegramDownloaderBot::sendGallery(const qint64& chat_id, const QList< GalleryItem>& files, const QString &url)
{
    QString description = files[0].getDescription();

    description += QString::fromStdString(fmt::format("\n\n[Original URL]({})", url));

    // photos and videos are send without description, 
    // but the next message does contain a common description for all of them

    bool bSuccess = false;

    // TODO: 10 MB limit (?)

    QVector<QFile*> inputFiles;

    QVector<InputMediaPhoto> photos;
    QVector<InputMediaVideo> videos;

    int nNumberOfFiles = 0;

    for (const GalleryItem &oneFile : files)
    {
        nNumberOfFiles++;

        QFile* myFile = new QFile(oneFile.getFile());

        if (myFile->open(QIODevice::ReadOnly))
        {
            inputFiles.push_back(myFile);

            if (oneFile.isVideo())
            {
                std::optional<int> width = oneFile.getWidth() ? std::optional<int>(oneFile.getWidth()) : std::nullopt;
                std::optional<int> height = oneFile.getHeight() ? std::optional<int>(oneFile.getHeight()) : std::nullopt;

                InputMediaVideo video(myFile, std::nullopt, std::nullopt, std::nullopt, std::nullopt, width, height);

                videos.emplace_back(video);
            }
            else
            {
                photos.emplace_back(InputMediaPhoto(myFile));
            }
        }
        else
            delete myFile;

        if (nNumberOfFiles >= 10)
        {
            _logger->warn("Limit number of messages to 10, total {}", files.size());

            break;
        }
    }

    _bot->sendChatAction(chat_id, Bot::ChatActionType::UPLOAD_DOCUMENT);

    if (photos.size() || videos.size())
    {
        QVector<Telegram::Message> v_msg = _bot->sendMediaGroup({
            .chat_id = chat_id,
            .photos = photos,
            .videos = videos,
            }
        ).get();

        bSuccess = (v_msg.isEmpty() == false);
    }

    for (QFile* myFile : inputFiles)
        delete myFile;

    if (bSuccess)
    {
        sendMessage(chat_id, description, false, true); // WITH MARKDOWN SUPPORT
    }

    return bSuccess;
}

bool QTelegramDownloaderBot::sendLogFile(const qint64& chat_id)
{
    const QString logFilePath = BotLogger::getCurrentLogFilePath();

    auto logFile = std::make_shared<QFile>(logFilePath);

    if (!logFile->exists())
    {
        getLogger()->warn("Log file doesn't exist: {}", logFilePath);
        sendMessage(chat_id, QString("Log file doesn't exist: %1").arg(QFileInfo(logFilePath).fileName()));

        return false;
    }

    if (!logFile->open(QIODevice::ReadOnly))
    {
        getLogger()->warn("Failed to open log file: {}", logFilePath);
        sendMessage(chat_id, "Failed to open log file.");

        return false;
    }

    _bot->sendChatAction(chat_id, Bot::ChatActionType::UPLOAD_DOCUMENT);

    Telegram::Message logMessage = _bot->sendDocument({
        .chat_id = chat_id,
        .document = logFile.get(),
        .caption = QString("Current log file")
        }).get();

    const bool success = (logMessage.isEmpty() == false);

    if (!success)
    {
        getLogger()->warn("Failed to send log file");
    }

    return success;
}
