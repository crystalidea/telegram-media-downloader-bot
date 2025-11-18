#pragma once

#include "video_info.h"

using TableValues = QVector< QPair<QString, int> >;

class VideoInfoParser
{
public:

    bool isValid() const;

    QVector<VideoStreamsInfo> readVideos(const QStringList& lines);

protected:

    void initWithHeader(const QString& line);

    QString getRowValue(const QString& line, const char* row) const;

    static QStringList fixLines(const QStringList& lines);

private:

    TableValues tableValues;
};