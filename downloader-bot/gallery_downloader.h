#pragma once

class GalleryItem
{
public:

    GalleryItem(const QString& name, const QString &descr, int width, int height, bool isVideo)
        : file(name), description(descr), width(width), height(height), _isVideo(isVideo)
    {

    }

    QString getFile() const {
        return file;
    }

    QString getDescription() const {
        return description;
    }

    void setDescription(const QString& desc) {
        description = desc;
    }

    bool isVideo() const {
        return _isVideo;
    }

    int getWidth() const {
        return width;
    }

    int getHeight() const {
        return height;
    }

private:

    QString file;
    
    int width = 0;
    int height = 0;

    bool _isVideo = false;

    QString description;
};

class GalleryInfo
{
public:

    bool isEmpty() const {
        return files.isEmpty();
    }

    QList< GalleryItem> getFiles() const {
        return files;
    }

    void addFile(const QString& name, const QString &description, int width, int height, bool isVideo) {
        files.append(GalleryItem(name, description, width, height, isVideo));
    }

    uint32_t getTotalFileSize() const {
        uint32_t totalSize = 0;
        for (const GalleryItem& f : files)
        {
            totalSize += QFileInfo(f.getFile()).size();
        }
        return totalSize;
    }

private:

    QList< GalleryItem> files;
};

class QGalleryDownloader : public QObject
{
public:

    QGalleryDownloader(QObject* parent = nullptr);
    ~QGalleryDownloader();

    void setCookies(const QString& file) {
        _cookiesFile = file;
    }

    bool downloadUrl(const QString& url, GalleryInfo &gi, spdlogger logger);

    static QString galleryVersion();

protected:

private:

    static QString _version;

    QString _cookiesFile; // required for instagram
};