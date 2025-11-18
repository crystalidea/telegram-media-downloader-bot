#include "pch.h"
#include "stream_info.h"

double parseSize(const QString& size_str) {

    if (size_str.size())
    {
        QString size_str_adjusted = size_str;
        size_str_adjusted.replace("~", "");
        size_str_adjusted.replace("≈", ""); // only in Linux, means contains both audio & video
        
        size_str_adjusted = size_str_adjusted.trimmed();

        if (size_str_adjusted.contains("KiB"))
        {
            bool ok = false;
            double ft = size_str_adjusted.left(size_str_adjusted.indexOf("KiB")).toDouble(&ok);

            if (ok)
                return ft / 1024.0;
        }
        else if (size_str_adjusted.contains("MiB"))
        {
            bool ok = false;
            double ft = size_str_adjusted.left(size_str_adjusted.indexOf("MiB")).toDouble(&ok);

            if (ok)
                return ft;
        }
        else if (size_str_adjusted.contains("GiB"))
        {
            bool ok = false;
            double ft = size_str_adjusted.left(size_str_adjusted.indexOf("GiB")).toDouble(&ok);

            if (ok)
                return ft * 1024;
        }

        Q_ASSERT(0);
    }

    return 0.0;
}

StreamInfo::StreamInfo(const QString& id, const QString& ext, const QString& resolution, const QString& fps, const QString& ch, 
    const QString& filesize, const QString& tbr, const QString& proto, const QString& vcodec, const QString& vbr, const QString& acodec, 
        const QString& abr, const QString& asr, const QString& more_info)
{
    _id = id;
    _ext = ext;
    _vcodec = vcodec;
    _acodec = acodec;

    _filesize_mb = parseSize(filesize);

    if (ext != "mhtml" && _filesize_mb)
    {
        if (vcodec == "audio only")
            _type = StreamType::Audio;
        else if (acodec == "video only")
            _type = StreamType::Video;
        else if (resolution.size())
            _type = StreamType::AudioVideo;
    }

    if (resolution.size())
    {
        static const QRegularExpression rx("(\\d+)x(\\d+)");

        QRegularExpressionMatch match = rx.match(resolution);

        if (match.hasMatch())
        {
            _nVideoWidth = match.captured(1).toInt();
            _nVideoHeight = match.captured(2).toInt();
        }
    }
}
