#include "frame.h"

cv::Mat Frame::centerObject(cv::Mat frame, int width, int height) {
    cv::Mat processed;
    cv::cvtColor(frame, processed, cv::COLOR_BGR2GRAY);

    // Downscale for faster object detection
    const double scale = 0.25;
    cv::resize(processed, processed, {}, scale, scale, cv::INTER_AREA);

    cv::blur(processed, processed, cv::Size(3, 3));
    cv::threshold(processed, processed, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(processed, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    if (contours.empty()) {
        return frame;
    }

    // Combine all contours
    std::vector<cv::Point> allPoints;
    for (const auto &contour : contours) {
        allPoints.insert(allPoints.end(), contour.begin(), contour.end());
    }

    // Rectangle for downscaled image
    cv::Rect rect = cv::boundingRect(allPoints);
    // Back to original size
    rect = cv::Rect{rect.tl() * (1.0 / scale), rect.br() * (1.0 / scale)};

    // Center of the rectangle
    cv::Point center = (rect.tl() + rect.br()) / 2;
    // Calculate a rectangle with 'width' and 'height' around center
    rect = cv::Rect{center.x - width / 2, center.y - height / 2, width, height};

    // Check for out-of-bounds
    cv::Rect validRect = rect & cv::Rect{0, 0, frame.cols, frame.rows};
    if (validRect.width == rect.width && validRect.height == rect.height) {
        return frame(validRect).clone();
    }

    // Fill out-of-bounds area with black
    cv::Mat black = cv::Mat::zeros(rect.size(), frame.type());
    frame(validRect).copyTo(black(cv::Rect{validRect.tl() - rect.tl(), validRect.size()}));

    return black;
}

double Frame::estimateQuality(cv::Mat frame) {
    cv::Mat gray;
    if (frame.channels() == 3) {
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    }
    else {
        gray = frame;
    }

    cv::GaussianBlur(gray, gray, cv::Size(3, 3), 0.5);

    cv::Mat gradX, gradY;
    cv::Sobel(gray, gradX, CV_64F, 1, 0, 3);
    cv::Sobel(gray, gradY, CV_64F, 0, 1, 3);

    cv::Mat magnitude;
    cv::magnitude(gradX, gradY, magnitude);

    cv::Mat capped;
    cv::threshold(magnitude, capped, 200.0, 200.0, cv::THRESH_TRUNC);

    return cv::mean(capped)[0];
}

std::vector<AlignmentPoint> Frame::getAps(cv::Mat frame, int apSize) {
    std::vector<AlignmentPoint> aps;

    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    gray.convertTo(gray, CV_32FC1);

    cv::blur(gray, gray, {3, 3});

    std::vector<cv::Point> cvAps;
    cv::goodFeaturesToTrack(gray, cvAps, 150, 0.15, apSize / 2);

    for (const auto &cvAp : cvAps) {
        aps.emplace_back(cvAp.x, cvAp.y, apSize);
    }

    return aps;

    // cv::Mat processed = frame.clone();
    // cv::cvtColor(processed, processed, cv::COLOR_BGR2GRAY);
    // cv::threshold(processed, processed, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    // cv::blur(processed, processed, {5, 5});

    // std::vector<std::vector<cv::Point>> contours;
    // cv::findContours(processed, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    // if (contours.empty()) {
    //     return {};
    // }

    // auto largestContour = [](std::vector<std::vector<cv::Point>> &contours) -> int {
    //     int largestContour = 0;
    //     double maxArea = 0.0;
    //     for (int i = 0; i < contours.size(); ++i) {
    //         double area = cv::contourArea(contours[i]);
    //         if (area > maxArea) {
    //             maxArea = area;
    //             largestContour = i;
    //         }
    //     }
    //     return largestContour;
    // };

    // cv::Mat mask = cv::Mat::zeros(frame.size(), CV_8UC1);
    // cv::drawContours(mask, contours, largestContour(contours), 255, cv::FILLED);

    // cv::Mat kernel = cv::Mat::ones(2 * step, 2 * step, CV_8UC1);
    // cv::erode(mask, mask, cv::Mat::ones(step, step, CV_8UC1));

    // while (cv::countNonZero(mask) > 0) {
    //     std::vector<std::vector<cv::Point>> layerContours;
    //     cv::findContours(mask, layerContours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);

    //     std::vector<cv::Point> contour = layerContours[largestContour(layerContours)];

    //     double arcLen = cv::arcLength(contour, true);
    //     int amount = std::max(1, static_cast<int>(arcLen / apSize * 1.4));

    //     for (int i = 0; i < amount; ++i) {
    //         double targetDist = (arcLen * i) / amount;
    //         double running = 0.0;
    //         for (size_t j = 1; j < contour.size(); ++j) {
    //             cv::Point2f p1 = contour[j - 1];
    //             cv::Point2f p2 = contour[j];
    //             double segLen = cv::norm(p2 - p1);
    //             if (running + segLen >= targetDist) {
    //                 double alpha = (targetDist - running) / segLen;
    //                 cv::Point2f pt = p1 + alpha * (p2 - p1);
    //                 aps.push_back(pt);
    //                 break;
    //             }
    //             running += segLen;
    //         }
    //     }

    //     cv::erode(mask, mask, kernel);
    // }
}

cv::Point2f Frame::correlate(cv::Mat reference, cv::Mat frame) {
    return cv::phaseCorrelate(reference, frame);
}
