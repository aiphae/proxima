#include "frame.h"

cv::Mat Frame::cropOnObject(cv::Mat frame, int width, int height) {
    return crop(frame, getObjectCrop(frame, findObject(frame), width, height));
}

cv::Rect Frame::findObject(cv::Mat frame) {
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

    // Resize for faster object detection
    const double scale = 0.25;
    cv::Mat small;
    cv::resize(gray, small, {}, scale, scale, cv::INTER_AREA);

    // Fast blur and threshold (skip Otsu)
    cv::blur(small, small, cv::Size(3, 3));
    cv::threshold(small, small, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(small, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        return {};
    }

    std::vector<cv::Point> allPoints;
    for (const auto& contour : contours) {
        allPoints.insert(allPoints.end(), contour.begin(), contour.end());
    }

    // Scale bounding box back to original size
    cv::Rect smallRect = cv::boundingRect(allPoints);
    return cv::Rect(cv::Point(smallRect.tl() * (1.0 / scale)),
                    cv::Point(smallRect.br() * (1.0 / scale)));
}

// Calculates a rectangle centered on 'object' with 'width' and 'height'.
// Does not check whether any points are out of bounds of the image.
cv::Rect Frame::getObjectCrop(cv::Mat frame, cv::Rect object, int width, int height) {
    cv::Point center = (object.tl() + object.br()) / 2;
    return cv::Rect(center.x - width / 2, center.y - height / 2, width, height);
}

// Crops the image with 'rect' and fills the out-of-bounds ares with black.
cv::Mat Frame::crop(cv::Mat frame, cv::Rect rect) {
    cv::Rect validRect = rect & cv::Rect(0, 0, frame.cols, frame.rows);

    if (validRect.width == rect.width && validRect.height == rect.height) {
        return frame(validRect).clone();
    }

    cv::Mat black = cv::Mat::zeros(rect.size(), frame.type());
    frame(validRect).copyTo(black(cv::Rect(validRect.tl() - rect.tl(), validRect.size())));
    return black;
}

double Frame::estimateQuality(cv::Mat frame) {
    cv::Mat gray;
    if (frame.channels() == 3) {
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = frame;
    }

    // Optional: apply a small Gaussian blur to reduce noise sensitivity
    cv::GaussianBlur(gray, gray, cv::Size(3, 3), 0.5);

    // Compute Sobel gradients (x and y)
    cv::Mat gradX, gradY;
    cv::Sobel(gray, gradX, CV_64F, 1, 0, 3);
    cv::Sobel(gray, gradY, CV_64F, 0, 1, 3);

    // Compute gradient magnitude
    cv::Mat magnitude;
    cv::magnitude(gradX, gradY, magnitude);

    // Optionally clip very high gradients to reduce influence of noise/hot pixels
    cv::Mat capped;
    cv::threshold(magnitude, capped, 200.0, 200.0, cv::THRESH_TRUNC);  // tune this if needed

    // Compute mean gradient magnitude (i.e., global sharpness metric)
    return cv::mean(capped)[0];
}
