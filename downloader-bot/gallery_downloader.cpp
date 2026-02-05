#include "pch.h"
#include "gallery_downloader.h"

#include "helpers.h"

#include <json.hpp>
#include <fstream>

using json = nlohmann::json;

QString QGalleryDownloader::_version;

QGalleryDownloader::QGalleryDownloader(QObject* parent /*= nullptr*/)
    : QObject(parent)
{

}

QGalleryDownloader::~QGalleryDownloader()
{
    qDeleteAll(tempDirs);

    tempDirs.clear();
}

bool QGalleryDownloader::downloadUrl(const QString& url, GalleryInfo& gi, spdlogger logger)
{
    tempDirs.append(new QTemporaryDir());

    QTemporaryDir *dir = tempDirs.back();

    QString tempPath = QDir::toNativeSeparators(dir->path());

    QStringList args;

    if (_cookiesFile.size())
    {
        if (QFile::exists(_cookiesFile))
        {
            args.push_back("--cookies");
            args.push_back(_cookiesFile);
        }
        else
        {
            logger->warn("Cookies file '{}' does not exist, skipping.", _cookiesFile);
        }
    }

    args.push_back("-D");
    args.push_back(tempPath);

    args.push_back("-f");
    args.push_back("{num}.{extension}");

    args.push_back("--write-metadata");

    args.push_back(url);

    QProcess process;

    process.setWorkingDirectory(tempPath);
    process.start("gallery-dl", args);
    process.waitForFinished();

    QStringList jsonFiles = QDir(tempPath).entryList(QStringList { "*.json"}, QDir::Files);

    if (jsonFiles.size())
    {
        for (auto jsonFileNoPath : jsonFiles)
        {
            QString jsonFile = tempPath + QDir::separator() + jsonFileNoPath;

            std::ifstream f(jsonFile.toUtf8().constData());
            json data = json::parse(f);
            f.close();

            QString description = QString::fromUtf8(data.value("description", "").c_str());
            description = Helpers::escapeMarkdownV2(description);

            std::string ext = data.value("extension", "");

            int nWidth = data.value("width", 0);
            int nHeight = data.value("height", 0);

            bool isVideo = (ext == "mp4");
            
            QString realFile = jsonFile;
            realFile.replace(".json", QString());
            
            gi.addFile(realFile, description, nWidth, nHeight, isVideo);

            QFile(jsonFile).remove();
        }
    }

    return (gi.isEmpty() == false);
}

QString QGalleryDownloader::galleryVersion()
{
    if (!_version.size())
    {
        QProcess process;

        process.start("gallery-dl", {"--version"});
        process.waitForFinished();

        _version = QString(process.readAllStandardOutput()).trimmed();
    }

    return _version;
}
