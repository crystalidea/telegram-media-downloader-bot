#pragma once

#include "download_info.h"

namespace ffmpeg
{
    std::optional<DownloadInfoItem> transcodeVideoToLimit(const DownloadInfoItem& original, qint64 maxSizeBytes, const spdlogger& logger);
}