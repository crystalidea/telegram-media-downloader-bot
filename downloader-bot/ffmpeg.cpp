#include "pch.h"
#include "ffmpeg.h"

std::optional<double> probeDurationSeconds(const QString& filePath, const spdlogger& logger)
{
    QProcess process;

    QStringList args = {
        "-v", "error",
        "-show_entries", "format=duration",
        "-of", "default=noprint_wrappers=1:nokey=1",
        filePath };

    process.start("ffprobe", args);
    process.waitForFinished();

    if (process.exitStatus() != QProcess::NormalExit)
    {
        logger->warn("ffprobe did not exit normally for {}", filePath);
        return std::nullopt;
    }

    bool ok = false;
    double duration = QString::fromUtf8(process.readAllStandardOutput()).trimmed().toDouble(&ok);

    if (process.exitCode() != 0 || ok == false || duration <= 0)
    {
        logger->warn("Failed to get duration for {}: {}", filePath, QString::fromUtf8(process.readAllStandardError()));
        return std::nullopt;
    }

    return duration;
}

std::optional<QSize> probeVideoDimensions(const QString& filePath, const spdlogger& logger)
{
    QProcess process;

    QStringList args = {
        "-v", "error",
        "-select_streams", "v:0",
        "-show_entries", "stream=width,height",
        "-of", "csv=p=0:s=x",
        filePath };

    process.start("ffprobe", args);
    process.waitForFinished();

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
    {
        logger->warn("Failed to read video dimensions for {}: {}", filePath, QString::fromUtf8(process.readAllStandardError()));
        return std::nullopt;
    }

    const QString output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    const QStringList parts = output.split("x");

    if (parts.size() != 2)
    {
        logger->warn("Unexpected ffprobe dimensions output for {}: {}", filePath, output);
        return std::nullopt;
    }

    bool widthOk = false;
    bool heightOk = false;
    int width = parts[0].toInt(&widthOk);
    int height = parts[1].toInt(&heightOk);

    if (!widthOk || !heightOk)
    {
        logger->warn("Failed to parse video dimensions for {}: {}", filePath, output);
        return std::nullopt;
    }

    return QSize(width, height);
}

std::optional<DownloadInfoItem> ffmpeg::transcodeVideoToLimit(const DownloadInfoItem& original, qint64 maxSizeBytes, const spdlogger& logger)
{
    QFileInfo fileInfo(original.videoFilePath);
    const QString outputPath = fileInfo.path() + "/" + fileInfo.completeBaseName() + "_tg.mp4";

    QFile::remove(outputPath);

    const auto durationSeconds = probeDurationSeconds(original.videoFilePath, logger);

    if (!durationSeconds.has_value())
    {
        return std::nullopt;
    }

    // Leave a small margin to ensure the resulting file fits within Telegram limits.
    const qint64 safetyMarginBytes = 256 * 1024;
    const qint64 targetSizeBytes = std::max<qint64>(1024 * 1024, maxSizeBytes - safetyMarginBytes);
    const double totalBitrateBps = (static_cast<double>(targetSizeBytes) * 8.0) / durationSeconds.value();

    const qint64 audioBitrate = 64 * 1024; // 64 kbps audio
    qint64 videoBitrate = static_cast<qint64>(totalBitrateBps) - audioBitrate;

    // Ensure we don't go below a reasonable lower bound.
    videoBitrate = std::max<qint64>(videoBitrate, 250 * 1024);

    QString videoBitrateStr = QString::number(videoBitrate / 1000) + "k";
    QString audioBitrateStr = QString::number(audioBitrate / 1000) + "k";

    QStringList args = {
        "-y",
        "-i", original.videoFilePath,
        "-c:v", "libx264",
        "-preset", "veryfast",
        "-b:v", videoBitrateStr,
        "-maxrate", videoBitrateStr,
        "-bufsize", videoBitrateStr,
        "-c:a", "aac",
        "-b:a", audioBitrateStr,
        "-movflags", "+faststart"
    };

    if (original.videoWidth > 1280 || original.videoHeight > 720)
    {
        args << "-vf" << "scale='min(1280,iw)':-2";
    }

    args << outputPath;

    QProcess process;
    process.start("ffmpeg", args);
    process.waitForFinished(-1);

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
    {
        logger->warn("ffmpeg failed to transcode {}: {}", original.videoFilePath, QString::fromUtf8(process.readAllStandardError()));
        return std::nullopt;
    }

    QFile outputFile(outputPath);

    if (!outputFile.exists())
    {
        logger->warn("Transcoded file {} does not exist", outputPath);
        return std::nullopt;
    }

    DownloadInfoItem compressedItem = original;
    compressedItem.videoFilePath = outputPath;

    const auto newDimensions = probeVideoDimensions(outputPath, logger);

    if (newDimensions.has_value())
    {
        compressedItem.videoWidth = newDimensions.value().width();
        compressedItem.videoHeight = newDimensions.value().height();
    }

    return compressedItem;
}