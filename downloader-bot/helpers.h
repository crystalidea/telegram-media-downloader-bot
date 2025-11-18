#pragma once

enum class UrlType
{
    InstagramReel,
    InstagramPost,
    Youtube,
    Twitter,
    Threads,

    Unknown
};

namespace Helpers {
    
    QString formatSize(const quint64& bytes);

    UrlType getUrlType(const QString& url);
    
    QList<QStringList> parseCSV(QIODevice* file, bool withHeader, QChar separator = QChar(','));

    QString escapeMarkdownV2(const QString& text);
}
