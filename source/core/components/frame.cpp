#include "frame.h"

//
// Centers an object in 'frame' by detecting the largest contour and
// adjusting a bounding rectangle to 'width' and 'height'.
//
// If the rectangle extends beyond the frame boundaries, out-of-bounds areas
// are filled with black.
//
cv::Mat Frame::centerObject(cv::Mat frame, int width, int height) {
    cv::Mat processed;
    cv::cvtColor(frame, processed, cv::COLOR_BGR2GRAY);

    // Downscale for faster object detection
    const double scale = 0.5;
    cv::resize(processed, processed, {}, scale, scale, cv::INTER_AREA);

    // Preprocess for contour detection
    cv::blur(processed, processed, cv::Size(3, 3));
    processed.convertTo(processed, CV_8U);
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
    rect = cv::Rect {rect.tl() * (1.0 / scale), rect.br() * (1.0 / scale)};

    // Center of the rectangle
    cv::Point center = (rect.tl() + rect.br()) / 2;
    // Calculate a rectangle with 'width' and 'height' around center
    rect = cv::Rect {center.x - width / 2, center.y - height / 2, width, height};

    // Check for out-of-bounds
    cv::Rect validRect = rect & cv::Rect {0, 0, frame.cols, frame.rows};
    if (validRect.width == rect.width && validRect.height == rect.height) {
        return frame(validRect).clone();
    }

    // Fill out-of-bounds area with black
    cv::Mat black = cv::Mat::zeros(rect.size(), frame.type());
    frame(validRect).copyTo(black(cv::Rect{validRect.tl() - rect.tl(), validRect.size()}));

    return black;
}

//
// Expands 'frame' in all directions to match 'width' and 'height'.
//
// Fills new borders with black.
//
cv::Mat Frame::expandBorders(cv::Mat frame, int width, int height) {
    int top = std::max(0, (height - frame.rows) / 2);
    int bottom = std::max(0, height - frame.rows - top);
    int left = std::max(0, (width - frame.cols) / 2);
    int right = std::max(0, width - frame.cols - left);

    cv::Mat bordered;
    cv::copyMakeBorder(frame, bordered, top, bottom, left, right, cv::BORDER_CONSTANT);

    return bordered;
}

//
// Estimates the quality of 'frame' by measuring its sharpness.
//
// Sharpness is calculated as the mean of the capped gradient magnitude,
// where higher values indicate sharper, more detailed images, and lower
// values suggest blurring.
//
double Frame::estimateQuality(cv::Mat frame) {
    // Convert to gray
    cv::Mat gray;
    if (frame.channels() == 3) {
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    }
    else {
        gray = frame;
    }

    // Downscale to reduce computation
    const double scale = 0.5;
    cv::resize(gray, gray, cv::Size(), scale, scale, cv::INTER_AREA);

    // Apply blur to reduce noise
    cv::GaussianBlur(gray, gray, cv::Size(3, 3), 0.5);

    // Compute the gradients in the x and y directions using Sobel operators
    cv::Mat gradX, gradY;
    cv::Sobel(gray, gradX, CV_64F, 1, 0, 3);
    cv::Sobel(gray, gradY, CV_64F, 0, 1, 3);

    // Calculate the gradient magnitude from the x and y gradients
    cv::Mat magnitude;
    cv::magnitude(gradX, gradY, magnitude);

    // Cap the gradient magnitude at 200 to prevent extreme values from dominating
    cv::Mat capped;
    cv::threshold(magnitude, capped, 200.0, 200.0, cv::THRESH_TRUNC);

    // Return the mean of the capped gradient magnitude
    return cv::mean(capped)[0];
}
