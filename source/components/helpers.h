#ifndef HELPERS_H
#define HELPERS_H

#include <QString>
#include "mediafile.h"

inline QString fileFilters() {
    auto buildFilter = [](const QString &description, const QSet<QString> &extensions) {
        QStringList patterns;
        for (const QString &extension : extensions) {
            patterns << "*" + extension;
        }
        return QString("%1 (%2)").arg(description, patterns.join(' '));
    };

    QString imageFilter = buildFilter("Image Files", imageExtensions);
    QString videoFilter = buildFilter("Video Files", videoExtensions);

    QSet<QString> allExtensions = imageExtensions + videoExtensions;
    QString allFilter = buildFilter("All Supported Files", allExtensions);

    return allFilter + ";;" + imageFilter + ";;" + videoFilter;
}

inline cv::Mat getMatAtFrame(std::vector<MediaFile> &files, int frame) {
    int currentFrame = 0;
    for (int i = 0; i < files.size(); ++i) {
        if (frame < currentFrame + files[i].frames()) {
            return files[i].matAtFrame(frame - currentFrame);
        }
        currentFrame += files[i].frames();
    }
    return cv::Mat();
}

#endif // HELPERS_H
