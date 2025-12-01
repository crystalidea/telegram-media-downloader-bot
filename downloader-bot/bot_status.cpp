#include "pch.h"
#include "bot_status.h"
#include "helpers.h"
#include "video_downloader.h"
#include "bot_config.h"
#include "gallery_downloader.h"
#include "bot_version.h"

#include <chrono>

BotStatus::BotStatus()
{
    _startTime = QDateTime::currentDateTime();

    _totalDownloadedSize = 0;
    _totalDownloads = 0;
}

QString BotStatus::getUptimeString() const
{
    using namespace std::chrono;

    const qint64 millisecondsDiff = _startTime.msecsTo(QDateTime::currentDateTime());

    std::chrono::milliseconds ms = std::chrono::milliseconds(millisecondsDiff);

    auto elapsed_secs = duration_cast<seconds>(ms);
    ms -= duration_cast<milliseconds>(elapsed_secs);

    auto elapsed_minutes = duration_cast<minutes>(elapsed_secs);
    elapsed_secs -= duration_cast<seconds>(elapsed_minutes);

    auto elapsed_hours = duration_cast<hours>(elapsed_minutes);
    elapsed_minutes -= duration_cast<minutes>(elapsed_hours);

    auto elapsed_days = duration_cast<days>(elapsed_hours);
    elapsed_hours -= duration_cast<hours>(elapsed_days);

    return QString::fromStdString(
        fmt::format("Uptime: {} days, {} hours, {} minutes and {} seconds", elapsed_days.count(), elapsed_hours.count(),
            elapsed_minutes.count(), elapsed_secs.count())
    );
}

QString BotStatus::getDownloadsString() const
{
    return QString::fromStdString(
        fmt::format("Downloaded: {} links of {} in total", _totalDownloads.load(), Helpers::formatSize(_totalDownloadedSize.load()))
    );
}

QString BotStatus::print() const
{
    QString status = "```Status\n";

    status += QString("Version: ") + BOT_VERSION_STR;
    status += "\n";
    status += getUptimeString();
    status += "\n";
    status += getDownloadsString();
    status += "\n";
    status += QString("yt-dlp: ") + QMediaDownloader::ytdlpVersion();
    status += "\n";
    status += QString("ffmpeg: ") + QMediaDownloader::ffmpegVersion();
    status += "\n";
    status += QString("gallery-dl: ") + QGalleryDownloader::galleryVersion();

    status += "```";
    
    return status;
}

void BotStatus::statsUpdateFileDownloaded(uint64_t file_size)
{
    _totalDownloads++;

    _totalDownloadedSize += file_size;

    QMutexLocker locker(&_csvMutex);

    QString csvContents;

    csvContents += "total_downloads,total_downloaded,total_downloaded_fmt\n";
    csvContents += QString("%1,%2,\"%3\"")
        .arg(QString::number(_totalDownloads))
        .arg(QString::number(_totalDownloadedSize))
        .arg(Helpers::formatSize(_totalDownloadedSize));

    QFile file(getStatsFileName());

    if (file.open(QIODevice::ReadWrite | QIODevice::Truncate))
    {
        file.write(csvContents.toUtf8());
    }
}

bool BotStatus::statsLoadFromFile(std::shared_ptr<spdlog::logger> logger)
{
    bool bSuccess = false;

    QFile file(getStatsFileName());

    if (file.open(QIODevice::ReadOnly))
    {
        auto parts = Helpers::parseCSV(&file, true);

        Q_ASSERT(parts.size() == 1 && parts[0].size() == 3); // one line

        if (parts.size() == 1 && parts[0].size() == 3)
        {
            bool ok1 = true, ok2 = true;

            uint32_t d1 = parts[0][0].toUInt(&ok1);
            uint64_t d2 = parts[0][1].toULongLong(&ok2);

            if (ok1 && ok2)
            {
                _totalDownloads = d1;
                _totalDownloadedSize = d2;

                logger->info("Loaded stats: {} downloads of {} size in total", _totalDownloads.load(), parts[0][2]);

                bSuccess = true;
            }
        }
        else
        {
            logger->warn("Failed to parse stats.csv");
        }
    }
    else
    {
        logger->warn("Failed to open stats.csv");
    }

    return bSuccess;
}

QString BotStatus::getStatsFileName() const
{
    return BotConfig::path() + QDir::separator() + "stats.csv";
}