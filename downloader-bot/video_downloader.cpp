#include "pch.h"
#include "video_downloader.h"
#include "video_info_parser.h"
#include "helpers.h"

QString QMediaDownloader::yt_dlp_path = "yt-dlp";
QString QMediaDownloader::ffmpeg_path = "ffmpeg";

QString QMediaDownloader::yt_dlp_version;
QString QMediaDownloader::ffmpeg_version;

QMediaDownloader::QMediaDownloader(QObject* parent) 
    : QObject(parent)
{
    
}

void QMediaDownloader::setCookies(const QString& file)
{
    _cookiesFile = file;
}

bool QMediaDownloader::downloadUrl(const QString& url, DownloadInfo& info, spdlogger logger)
{
    QString tempPath = tempDir.path();

    // improvement: instagram reels don't always require cookies
    // don't compromise the instagram account security,
    // use cookies only when necessary

    VideoInfo vi;

    logger->info("Querying media info...");

    bool bInstagramReel = (Helpers::getUrlType(url) == UrlType::InstagramReel);
    bool bUseCookies = false;

    if (bInstagramReel)
    {
        logger->info("No cookies first");
    }

    bool bGetVideoOK = getVideoInfo(url, vi, logger, bUseCookies);

    if (!bGetVideoOK && bInstagramReel)
    {
        bUseCookies = true;

        logger->warn("Failed, trying with cookies");

        bGetVideoOK = getVideoInfo(url, vi, logger, bUseCookies);
    }

    // by default yt-dlp downloads best quality which is often more than 50MB allowed

    if (bGetVideoOK)
    {
        for (int i = 0; i < vi.getCount(); i++)
        {
            // lets dump all formats first

            QStringList args;
            args.push_back(url);
            args.push_back("-o");
            args.push_back(QString("%1/video%2").arg(tempPath).arg(i));

            QString download_info_txt = QString("%1/download_info%2.txt").arg(tempPath).arg(i);

            args.push_back("--remux-video");
            args.push_back("mp4");

            args.push_back("--print-to-file");
            args.push_back("after_move:filepath");
            args.push_back(download_info_txt);

            args.push_back("--print-to-file");
            args.push_back("after_move:title");
            args.push_back(download_info_txt);

            if (vi.getCount() > 1) // specify playlist index
            {
                args.push_back("-I");
                args.push_back(QString::number(i + 1));
            }

            if (bInstagramReel)
            {
                if (_cookiesFile.size() && bUseCookies)
                {
                    args.push_back("--cookies");
                    args.push_back(_cookiesFile);
                }
            }

            DownloadInfoItem infoItem;

            const VideoStreamsInfo& v_streams = vi.getItem(i);

            QString bestStreamID = vi.getBestStreamID(v_streams, url, infoItem.videoWidth, infoItem.videoHeight, logger);

            if (bestStreamID.size())
            {
                logger->info("Best stream: {}", bestStreamID);

                args.push_back("-f");
                args.push_back(bestStreamID);

                /*
                // https://github.com/yt-dlp/yt-dlp/issues/7607
                args.push_back("--use-postprocessor");
                args.push_back("FFmpegCopyStream");
                args.push_back("--ppa");
                args.push_back("CopyStream:' - c:v libx264 - c : a aac - f mp4'");
                */
            }
            else
            {
                if (v_streams.size() == 1)
                {
                    infoItem.videoWidth = v_streams[0].getWidth();
                    infoItem.videoHeight = v_streams[0].getHeight();

                    bestStreamID = v_streams[0].getID();
                }
                else
                {
                    logger->info("Use default stream selection");

                    // from the manual
                    // # Download largest video(that also has audio) but no bigger than 50 MB,
                    // # or the smallest video(that also has audio) if there is no video under 50 MB

                    args.push_back("-f");
                    args.push_back("b");

                    args.push_back("-S");
                    args.push_back("filesize:50M");
                }
            }

            logger->info("Downloading...");

            QProcess process;

            process.setWorkingDirectory(tempPath);
            process.start(yt_dlp_path, args);
            process.waitForFinished();

            QFile file(download_info_txt);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                logger->critical(QString::fromUtf8(process.readAllStandardError()));

                return false;
            }

            infoItem.videoFilePath = QString::fromUtf8(file.readLine()).trimmed();

            infoItem.videoTitle = QString::fromUtf8(file.readLine()).trimmed();
            infoItem.videoTitle = Helpers::escapeMarkdownV2(infoItem.videoTitle);

            info.push_back(infoItem);
        }
    }
    else
    {
        logger->warn("Failed");
    }

    return info.size() > 0;
}

QString QMediaDownloader::ytdlpVersion()
{
    if (!yt_dlp_version.size())
    {
        QProcess process;

        //process.setWorkingDirectory(tempDir.path());
        process.start(yt_dlp_path, { "--version" });
        process.waitForFinished();

        yt_dlp_version = QString(process.readAllStandardOutput()).trimmed();
    }

    return yt_dlp_version;
}

QString QMediaDownloader::ffmpegVersion()
{
    if (!ffmpeg_version.size())
    {
        QProcess process;

        //process.setWorkingDirectory(tempDir.path());
        process.start(ffmpeg_path, { "-version" });
        process.waitForFinished();

        auto lines = QString(process.readAllStandardOutput()).split("\n", Qt::SkipEmptyParts);

        QString v = lines.size() ? lines[0].trimmed() : QString();

        QRegularExpression rx("ffmpeg version (.+?)\\s");

        QRegularExpressionMatch match = rx.match(v);

        if (match.hasMatch())
            v = match.captured(1);

        ffmpeg_version = v;
    }

    return ffmpeg_version;
}

bool QMediaDownloader::getVideoInfo(const QString &url, VideoInfo& info, spdlogger logger, bool useCookies)
{
    QStringList args;
    
    args.push_back("--list-formats");

    if (Helpers::getUrlType(url) == UrlType::InstagramPost || Helpers::getUrlType(url) == UrlType::InstagramReel)
    {
        if (_cookiesFile.size() && useCookies)
        {
            args.push_back("--cookies");
            args.push_back(_cookiesFile);
        }
    }

    args.push_back(url);

    QString cmdLine;
    for (auto a : args)
    {
        cmdLine += a;
        cmdLine += " ";
    }

    QProcess process;

    process.setWorkingDirectory(tempDir.path());
    process.start(yt_dlp_path, args);   
    process.waitForFinished();

    logger->info("Arguments: {}", args.join(" "));

    QStringList stdOutLines = QString(process.readAllStandardOutput()).split("\n", Qt::SkipEmptyParts);

    for (const QString& line : stdOutLines)
        logger->info(line);

    return info.readlistFormatsOutput(stdOutLines);
}
