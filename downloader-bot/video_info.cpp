#include "pch.h"
#include "video_info.h"
#include "video_info_parser.h"
#include "helpers.h"

static const double maxSizeLimit = 50.0;

bool VideoInfo::readlistFormatsOutput(const QStringList& lines)
{
    VideoInfoParser parser;

    _videos = parser.readVideos(lines);

    return (_videos.size() > 0);
}

QString VideoInfo::getBestStreamID(const VideoStreamsInfo& video, const QString& url, int &videoWidth, int &videoHeight, spdlogger logger) // static
{
    QString bestStreamID;

    const UrlType type = Helpers::getUrlType(url);

    logger->info("Streams count: {}", video.size());

    if (video.size() > 1)
    {
        if (type == UrlType::InstagramReel)
        {
            // instagram reels may have no size and format, 
            // select based on resolution

            QVector<StreamInfo> streamsWithNoSize;

            for (const StreamInfo& si : video)
            {
                const int streamWidth = si.getWidth();

                if (streamWidth && si.getType() == StreamType::UNKNOWN && si.getExtension() == "mp4")
                    streamsWithNoSize.push_back(si);
            }

            // sort descending width
            std::sort(streamsWithNoSize.begin(), streamsWithNoSize.end(), 
                [](const StreamInfo& a, const StreamInfo& b) { return b.getWidth() < a.getWidth(); });

            if (streamsWithNoSize.size())
            {
                const StreamInfo* si = nullptr;

                if (streamsWithNoSize.size() > 1)
                    si = &streamsWithNoSize[1]; // take not the biggest one
                else
                    si = &streamsWithNoSize[0]; // take the only one

                videoWidth = si->getWidth();
                videoHeight = si->getHeight();

                bestStreamID = si->getID();
            }
        }
        else
        {
            // select based on size

            double bestSize = 0;

            for (const StreamInfo& si : video)
            {
                const double streamSize = si.getSizeInMB();

                if (si.getType() == StreamType::AudioVideo &&
                    streamSize < maxSizeLimit && streamSize > bestSize)
                {
                    logger->info("matching stream {}x{} id={} size={}", si.getWidth(), si.getHeight(), si.getID(), Helpers::formatSize(si.getSizeInBytes()));

                    videoWidth = si.getWidth();
                    videoHeight = si.getHeight();

                    bestSize = streamSize;
                    bestStreamID = si.getID();
                }
            }
        }
        

        if (bestStreamID.isEmpty())
        {
            QVector<StreamInfo> audioStreams;
            QVector<StreamInfo> videoStreams;

            for (const StreamInfo& si : video)
            {
                if (si.getSizeInMB() < maxSizeLimit)
                {
                    if (si.getType() == StreamType::Audio)
                        audioStreams.push_back(si);
                    else if (si.getType() == StreamType::Video)
                        videoStreams.push_back(si);
                }
            }

            std::sort(audioStreams.begin(), audioStreams.end(), [](const StreamInfo& a, const StreamInfo& b) { return b.getSizeInMB() < a.getSizeInMB(); });
            std::sort(videoStreams.begin(), videoStreams.end(), [](const StreamInfo& a, const StreamInfo& b) { return b.getSizeInMB() < a.getSizeInMB(); });

            int nVideoStreamStartWith = 0; // biggest

            // if the size of the first is more than 20 MB, choose somewhat medium
            if (videoStreams.size() >= 2 && videoStreams[0].getSizeInMB() > 20.0) 
            {
                nVideoStreamStartWith = videoStreams.size() / 2;
            }
            else
            {
                // otherwise prefer biggest size
            }

            for (int i=nVideoStreamStartWith; i<videoStreams.size(); i++)
            {
                const StreamInfo& vs = videoStreams[i];

                if (bestStreamID.size())
                    break;

                if (vs.getVCodec().startsWith("av01")) // not supported
                    continue;
                
                for (const StreamInfo& as : audioStreams)
                {
                    if (as.getSizeInMB() > vs.getSizeInMB()) // if audio is bigger than video, it's too much
                        continue;

                    double totalSize = vs.getSizeInMB() + as.getSizeInMB() + 1.0; // extra 1MB for remux, just in case - may happen; 

                    if (totalSize < maxSizeLimit)
                    {
                        videoWidth = vs.getWidth();
                        videoHeight = vs.getHeight();

                        bestStreamID = QString("%1+%2").arg(vs.getID(), as.getID());

                        break;
                    }
                }
            }
        }
    }

    return bestStreamID;
}
