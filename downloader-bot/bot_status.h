#pragma once

class QMediaDownloader;

class BotStatus
{
public:

    BotStatus();

    QString print() const;

    void statsUpdateFileDownloaded(uint64_t file_size);
    bool statsLoadFromFile(std::shared_ptr<spdlog::logger> logger);

protected:

    QString getUptimeString() const;
    QString getDownloadsString() const;

    QString getStatsFileName() const;

private:

    QDateTime _startTime;

    std::atomic<uint32_t> _totalDownloads;
    std::atomic<uint64_t> _totalDownloadedSize;

    QMutex _csvMutex;
};