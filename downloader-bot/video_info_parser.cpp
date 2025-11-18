#include "pch.h"
#include "video_info_parser.h"

bool VideoInfoParser::isValid() const {
    return tableValues.size();
}

QVector<VideoStreamsInfo> VideoInfoParser::readVideos(const QStringList& lines)
{
    QVector<VideoStreamsInfo> foundVideos;

    QStringList lines_Fixed = fixLines(lines);

    VideoStreamsInfo currentVideo;

    for (const QString& line : lines_Fixed)
    {
        if (line.startsWith("ID "))
        {
            initWithHeader(line);

            if (currentVideo.size())
                foundVideos.push_back(currentVideo);

            currentVideo = VideoStreamsInfo(); // reset

            continue;
        }
        else if (isValid() && !line.startsWith("---") && !line.startsWith("[")) // '[' is from [download] Downloading item 3 of 3
        {
            Q_ASSERT(line.size() > 0);

            QString id = getRowValue(line, "ID");
            QString ext = getRowValue(line, "EXT");
            QString resolution = getRowValue(line, "RESOLUTION");
            QString fps = getRowValue(line, "FPS");
            QString ch = getRowValue(line, "CH");
            QString filesize = getRowValue(line, "FILESIZE");
            QString tbr = getRowValue(line, "TBR");
            QString proto = getRowValue(line, "PROTO");
            QString vcodec = getRowValue(line, "VCODEC");
            QString vbr = getRowValue(line, "VBR");
            QString acodec = getRowValue(line, "ACODEC");
            QString abr = getRowValue(line, "ABR");
            QString asr = getRowValue(line, "ASR");
            QString more_info = getRowValue(line, "MORE INFO");

            Q_ASSERT(id.size());

            currentVideo.push_back(StreamInfo(id, ext, resolution, fps, ch, filesize, tbr, proto, vcodec, vbr, acodec, abr, asr, more_info));
        }
    }

    if (currentVideo.size())
        foundVideos.push_back(currentVideo);

    Q_ASSERT(foundVideos.size());

    return foundVideos;
}

void VideoInfoParser::initWithHeader(const QString& line)
{
    tableValues.clear();

    const static int nFileSizeLen = strlen("FILESIZE");
    static int nVCodecColumnWidth = strlen("vp09.00.51.08");

    tableValues.push_back({ "ID" , 0 });
    tableValues.push_back({ "EXT" , line.indexOf("EXT ") });
    tableValues.push_back({ "RESOLUTION" , line.indexOf("RESOLUTION ") });
    
    if (line.indexOf("FPS ") != -1)
        tableValues.push_back({ "FPS" , line.indexOf("FPS ") });

    if (line.indexOf("CH ") != -1)
        tableValues.push_back({ "CH" , line.indexOf("CH ") });

    if (line.indexOf("FILESIZE") != -1)
    {
        int nFileSizePos = line.indexOf("|") + 1;

        tableValues.push_back({ "FILESIZE" , nFileSizePos }); // ! first position after the |

        int nFileSize2Pos = line.indexOf("FILESIZE");
        int tbrPos = nFileSize2Pos + nFileSizeLen + 1;

        tableValues.push_back({ "TBR" , tbrPos });
    }
    
    tableValues.push_back({ "PROTO" , line.indexOf("PROTO ") });

    int nPosVCodec = line.indexOf("VCODEC ");

    tableValues.push_back({ "VCODEC" , nPosVCodec });
    
    if (line.indexOf("VBR ") != -1)
    {
        int nVbrPos = nPosVCodec + nVCodecColumnWidth; // vcodec column width

        tableValues.push_back({ "VBR" , nVbrPos });
    }
    
    if (line.indexOf("ACODEC") != -1)
        tableValues.push_back({ "ACODEC" , line.indexOf("ACODEC") });

    if (line.indexOf("ABR ") != -1)
        tableValues.push_back({ "ABR" , line.indexOf("ABR ") });

    if (line.indexOf("ASR ") != -1)
        tableValues.push_back({ "ASR" , line.indexOf("ASR ") });

    if (line.indexOf("MORE INFO") != -1)
        tableValues.push_back({ "MORE INFO" , line.indexOf("MORE INFO") });
}

QString VideoInfoParser::getRowValue(const QString& line, const char* row) const
{
    std::optional<int> nPos;
    int nWidth = 0;

    for (int i = 0, c = tableValues.size(); i < c; i++)
    {
        if (tableValues[i].first == row)
        {
            nPos = tableValues[i].second;

            if ((i + 1) < c)
                nWidth = tableValues[i + 1].second - nPos.value();
            else
                nWidth = line.length() - nPos.value();

            break;
        }
    }

    QString strRow = (nPos.has_value() && nWidth > 0) ? line.mid(nPos.value(), nWidth) : QString();

    strRow.replace("|", "");
    strRow = strRow.trimmed();

    return strRow;
}

// sometimes output is misplaced like this
// see stream 22, one space is missing:
// example: https://www.youtube.com/watch?v=DSACezi2WPQ
// example: https://youtube.com/shorts/nGIiMH6aU4I?si=G8T30Ax9QdAXHP_z

// 244     webm  854x480     30    |  118.66MiB   277k https | vp09.00.30.08   277k video only          480p, webm_dash
// 22      mp4   1280x720    30  2 | 461.01MiB  1049k https | avc1.64001F          mp4a.40.2       44k [en] 720p
// 136     mp4   1280x720    30    |  395.15MiB   921k https | avc1.64001f     921k video only          720p, mp4_dash

void fixLine(QString& line) // fixes output
{
    static int nPosProto = -1;

    int nPosTest = line.indexOf("PROTO");

    if (nPosTest != -1)
    {
        nPosProto = nPosTest;
    }
    else if (nPosProto != -1 && nPosProto < line.size() && !line.startsWith("---"))
    {
        QChar zeroTest = line[nPosProto - 1];

        if (zeroTest != QChar(' '))
        {
            int indexofFirstVline = line.indexOf("|");

            if (indexofFirstVline != -1)
                line.insert(indexofFirstVline + 1, " ");
        }
    }
}

QStringList VideoInfoParser::fixLines(const QStringList& lines)
{
    QStringList lines_fixed = lines;

    for (QString& line : lines_fixed)
        fixLine(line);

    return lines_fixed;
}