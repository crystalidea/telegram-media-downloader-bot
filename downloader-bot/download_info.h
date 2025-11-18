#pragma once

struct DownloadInfoItem
{
    QString videoFilePath;
    QString videoTitle;

    qint32 videoWidth = 0;
    qint32 videoHeight = 0;
};

using DownloadInfo = QVector<DownloadInfoItem>;