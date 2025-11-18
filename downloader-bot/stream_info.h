#pragma once

enum StreamType
{
    AudioVideo, Audio, Video, UNKNOWN
};

struct StreamInfo
{
public:

    StreamInfo(const QString& id, const QString& ext, const QString& resolution, const QString& fps, const QString& ch,
        const QString& filesize, const QString& tbr, const QString& proto, const QString& vcodec, const QString& vbr,
        const QString& acodec, const QString& abr, const QString& asr, const QString& more_info);

    double getSizeInMB() const {
        return _filesize_mb;
    }

    uint32_t getSizeInBytes() const {
        return _filesize_mb * 1024 * 1024;
    }

    StreamType getType() const {
        return _type;
    }

    QString getID() const {
        return _id;
    }

    QString getExtension() const {
        return _ext;
    }

    QString getVCodec() const {
        return _vcodec;
    }

    QString getACodec() const {
        return _acodec;
    }

    int getWidth() const {
        return _nVideoWidth;
    }

    int getHeight() const {
        return _nVideoHeight;
    }

private:

    QString _id;
    QString _ext;
    double _filesize_mb = 0;
    StreamType _type = StreamType::UNKNOWN;
    QString _vcodec;
    QString _acodec;
    
    int _nVideoWidth = 0;
    int _nVideoHeight = 0;
};