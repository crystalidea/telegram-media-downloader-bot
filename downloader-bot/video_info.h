#pragma once

#include "stream_info.h"

using VideoStreamsInfo = QVector<StreamInfo>;

class VideoInfo
{
public:

    bool readlistFormatsOutput(const QStringList& lines);

    static QString getBestStreamID(const VideoStreamsInfo &video, const QString &url, int& videoWidth, int& videoHeight, spdlogger logger);

    size_t getCount() const {
        return _videos.size();
    }

    const VideoStreamsInfo &getItem(size_t nItem) const {
        return _videos[nItem];
    }

private:

    QVector<VideoStreamsInfo> _videos;
};