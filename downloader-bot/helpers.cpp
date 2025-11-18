#include "pch.h"
#include "helpers.h"

#ifdef Q_OS_WIN
    #include <windows.h>
    #include <shlwapi.h>
    #pragma comment(lib, "Shlwapi.lib")
#endif

QString Helpers::formatSize(const quint64& bytes)
{
    QString ret;

#if defined(Q_OS_WIN)

    WCHAR size_str[10];
    ::StrFormatByteSizeW(bytes, size_str, 10);

    ret = QString::fromUtf16(reinterpret_cast<const char16_t*>(size_str));

#else

    const static qint64 B = 1;
    const static qint64 KB = 1024 * B;
    const static qint64 MB = 1024 * KB;
    const static qint64 GB = 1024 * MB;

    static QString b = "B";
    static QString kb = "KB";
    static QString mb = "MB";
    static QString gb = "GB";

    if (bytes > GB)
    {
        ret = QString("%1 %2").arg((double)bytes / (double)GB, 0, 'f', 2).arg(gb);
    }
    else if (bytes > MB)
    {
        ret = QString("%1 %2").arg((double)bytes / (double)MB, 0, 'f', 1).arg(mb);
    }
    else if (bytes > KB)
    {
        ret = QString("%1 %2").arg((double)bytes / (double)KB, 0, 'f', 0).arg(kb);
    }
    else
    {
        ret = QString("%1 %2").arg(bytes).arg(b);
    }

    ret = ret.replace(".00", "");

#endif

    return ret;
}

UrlType Helpers::getUrlType(const QString& url)
{
    if (url.contains("instagram.com/p/"))
        return UrlType::InstagramPost;
    else if (url.contains("instagram.com/reel"))
        return UrlType::InstagramReel;
    else if (url.contains("youtube.com") || url.contains("youtu.be"))
        return UrlType::Youtube;
    else if (url.contains("x.com") || url.contains("twitter.com"))
        return UrlType::Twitter;
    else if (url.contains("threads.net") || url.contains("threads.com"))
        return UrlType::Threads;

    return UrlType::Unknown;
}

// Helper function that parses a single CSV line into fields.
static QStringList parseCsvLine(const QString& line, QChar separator)
{
    QStringList fields;
    QString currentField;
    bool inQuotedField = false;
    QChar quoteChar = QChar(); // Will be '\'' or '\"' once in quoted mode.
    bool escaped = false;      // Tracks if the last character was a backslash.

    for (int i = 0; i < line.size(); ++i) {
        QChar c = line.at(i);

        if (inQuotedField) {
            // We are *inside* a quoted field.
            if (escaped) {
                // The previous character was '\', so interpret this character literally.
                // We only do a few "standard" un-escapes here: \" -> "
                // (or \' -> ') and \\ -> \. Everything else is appended as-is.
                if ((c == quoteChar) || (c == '\\')) {
                    currentField.append(c);
                }
                else {
                    // Not a recognized escape -> add '\' + c literally
                    currentField.append('\\');
                    currentField.append(c);
                }
                escaped = false;
            }
            else {
                // Not escaped
                if (c == '\\') {
                    // Might escape the next character
                    escaped = true;
                }
                else if (c == quoteChar) {
                    // End of quoted field
                    inQuotedField = false;
                    quoteChar = QChar();
                }
                else {
                    // Just a normal character inside a quoted field
                    currentField.append(c);
                }
            }
        }
        else {
            // We are *not* inside a quoted field.
            if (c == separator) {
                // This ends the current field
                fields.append(currentField.trimmed());
                currentField.clear();
            }
            else if (c == '"' || c == '\'') {
                // Start of a quoted field
                inQuotedField = true;
                quoteChar = c;
            }
            else {
                // Normal character in an unquoted field
                currentField.append(c);
            }
        }
    }

    // Add the final field in the line
    fields.append(currentField.trimmed());

    return fields;
}

/*!
 * \brief parseCSV
 * Reads all lines from \a file, parses each line as CSV with the given
 * \a separator, and returns a \c QList<QStringList> of rows. If
 * \a withHeader is \c true, the first line is read and discarded.
 *
 * \param file        The QIODevice to read from (e.g. a QFile opened for reading)
 * \param withHeader  Whether to skip the first line (e.g. if it contains column headers)
 * \param separator   The field separator character (default: comma)
 * \return            A list of rows, where each row is a list of fields (QString)
 */
QList<QStringList> Helpers::parseCSV(QIODevice* file, bool withHeader, QChar separator /*= QChar(',')*/)
{
    QList<QStringList> rows;

    // If the file is not yet open, try to open it as text read-only.
    if (!file->isOpen()) {
        if (!file->open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Could not open file for reading.";
            return rows;
        }
    }

    QTextStream in(file);
    bool skipFirstLine = withHeader;

    while (!in.atEnd()) {
        QString line = in.readLine();

        // Skip the very first line if it's a header
        if (skipFirstLine) {
            skipFirstLine = false;
            continue;
        }

        // Parse the line into fields
        QStringList fields = parseCsvLine(line, separator);
        rows.append(fields);
    }

    return rows;
}

QString Helpers::escapeMarkdownV2(const QString& text) {
    static const QMap<QChar, QString> markdownSpecialChars = {
        {'_', "\\_"},
        {'*', "\\*"},
        {'[', "\\["},
        {']', "\\]"},
        {'(', "\\("},
        {')', "\\)"},
        {'~', "\\~"},
        {'`', "\\`"},
        {'>', "\\>"},
        {'#', "\\#"},
        {'+', "\\+"},
        {'-', "\\-"},
        {'=', "\\="},
        {'|', "\\|"},
        {'{', "\\{"},
        {'}', "\\}"},
        {'.', "\\."},
        {'!', "\\!"}
    };

    QString escapedText;
    for (QChar c : text) {
        if (markdownSpecialChars.contains(c)) {
            escapedText.append(markdownSpecialChars.value(c));
        }
        else {
            escapedText.append(c);
        }
    }

    return escapedText;
}