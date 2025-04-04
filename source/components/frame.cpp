#include "frame.h"

cv::Rect Frame::findObject(int minSize) {
    cv::Mat proccessed = image.clone();

    cv::cvtColor(proccessed, proccessed, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(proccessed, proccessed, cv::Size(5, 5), 0);
    cv::threshold(proccessed, proccessed, 0, 255, cv::THRESH_OTSU | cv::THRESH_BINARY);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(proccessed, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        return cv::Rect();
    }

    std::vector<cv::Point> allPoints;
    for (const auto &contour : contours) {
        allPoints.insert(allPoints.end(), contour.begin(), contour.end());
    }

    cv::Rect boundingRect;
    if (!allPoints.empty()) {
        boundingRect = cv::boundingRect(allPoints);
        if (std::min(boundingRect.width, boundingRect.height) < minSize) {
            boundingRect = cv::Rect();
        }
    }

    return boundingRect;
}

cv::Rect Frame::getObjectCrop(cv::Rect object, int width, int height) {
    cv::Point center = (object.br() + object.tl()) / 2;
    cv::Point tl(center.x - width / 2, center.y - height / 2);
    cv::Point br(center.x + width / 2, center.y + height / 2);
    return cv::Rect(tl, br);
}

cv::Mat Frame::crop(cv::Rect rect) {
    cv::Rect alignedCrop = rect & cv::Rect(0, 0, image.cols, image.rows);
    if (alignedCrop.width != rect.width || alignedCrop.height != rect.height) {
        cv::Mat black = cv::Mat::zeros(cv::Size(rect.width, rect.height), image.type());
        auto imageRect = cv::Rect({}, image.size());
        auto intersection = imageRect & rect;
        auto interROI = intersection - rect.tl();
        image(intersection).copyTo(black(interROI));
        return black;
    }
    else {
        return image(rect);
    }
}

void Frame::convertColor(int code) {
    cv::cvtColor(image, image, code);
}
