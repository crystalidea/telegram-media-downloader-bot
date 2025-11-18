#pragma once

#include "download_info.h"
#include "video_info.h"

class QMediaDownloader : public QObject
{
public:

    QMediaDownloader(QObject *parent = nullptr);

    void setCookies(const QString& file);

    bool downloadUrl(const QString& url, DownloadInfo&info, spdlogger logger);

    static QString ytdlpVersion();
    static QString ffmpegVersion();
    
protected:

    bool getVideoInfo(const QString& url, VideoInfo& info, spdlogger logger, bool useCookies);

private:

    static QString yt_dlp_path;
    static QString ffmpeg_path;

    static QString yt_dlp_version;
    static QString ffmpeg_version;

    QTemporaryDir tempDir;

    QString _cookiesFile;
};