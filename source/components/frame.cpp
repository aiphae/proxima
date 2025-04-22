#include "frame.h"

#include <qDebug>

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

std::vector<AlignmentPoint> Frame::getAps(cv::Mat frame, int apSize, APPlacement placement) {
    std::vector<AlignmentPoint> aps;

    cv::Mat processed;
    cv::cvtColor(frame, processed, cv::COLOR_BGR2GRAY);

    std::vector<cv::Point> cvAps;

    if (placement == APPlacement::FeatureBased) {
        processed.convertTo(processed, CV_32FC1);
        cv::blur(processed, processed, {3, 3});
        cv::goodFeaturesToTrack(processed, cvAps, 150, 0.5, apSize / 2);
    }
    else {
        cv::threshold(processed, processed, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
        cv::blur(processed, processed, {5, 5});

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(processed, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        if (contours.empty()) {
            return {};
        }

        auto largestContour = [](std::vector<std::vector<cv::Point>> &contours) -> int {
            int largestContour = 0;
            double maxArea = 0.0;
            for (int i = 0; i < contours.size(); ++i) {
                double area = cv::contourArea(contours[i]);
                if (area > maxArea) {
                    maxArea = area;
                    largestContour = i;
                }
            }
            return largestContour;
        };

        const int step = apSize / 2;

        cv::Mat mask = cv::Mat::zeros(frame.size(), CV_8UC1);
        cv::drawContours(mask, contours, largestContour(contours), 255, cv::FILLED);

        cv::Mat kernel = cv::Mat::ones(2 * step, 2 * step, CV_8UC1);
        cv::erode(mask, mask, cv::Mat::ones(step, step, CV_8UC1));

        while (cv::countNonZero(mask) > 0) {
            std::vector<std::vector<cv::Point>> layerContours;
            cv::findContours(mask, layerContours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);

            std::vector<cv::Point> contour = layerContours[largestContour(layerContours)];

            double arcLen = cv::arcLength(contour, true);
            int amount = std::max(1, static_cast<int>(arcLen / apSize * 1.4));

            for (int i = 0; i < amount; ++i) {
                double targetDist = (arcLen * i) / amount;
                double running = 0.0;
                for (size_t j = 1; j < contour.size(); ++j) {
                    cv::Point2f p1 = contour[j - 1];
                    cv::Point2f p2 = contour[j];
                    double segLen = cv::norm(p2 - p1);
                    if (running + segLen >= targetDist) {
                        double alpha = (targetDist - running) / segLen;
                        cv::Point2f pt = p1 + alpha * (p2 - p1);
                        cvAps.push_back(pt);
                        break;
                    }
                    running += segLen;
                }
            }

            cv::erode(mask, mask, kernel);
        }
    }

    for (const auto &cvAp : cvAps) {
        aps.emplace_back(cvAp.x, cvAp.y, apSize);
    }

    return aps;
}

cv::Point2f Frame::computeShift(cv::Mat reference, cv::Mat target) {
    // Ensure images are the same size and type
    CV_Assert(reference.size() == target.size() && reference.type() == CV_32F && target.type() == CV_32F);

    int W = reference.cols;
    int H = reference.rows;

    // Step 1: Compute the FFT of both images
    cv::Mat fft_ref, fft_tar;
    cv::dft(reference, fft_ref, cv::DFT_COMPLEX_OUTPUT);
    cv::dft(target, fft_tar, cv::DFT_COMPLEX_OUTPUT);

    // Step 2: Compute the conjugate of the FFT of the target
    cv::Mat planes[2];
    cv::split(fft_tar, planes);
    planes[1] = -planes[1]; // Negate the imaginary part to compute conjugate
    cv::Mat conj_fft_tar;
    cv::merge(planes, 2, conj_fft_tar);

    // Step 3: Compute the element-wise product of fft_ref and conj_fft_tar
    cv::Mat prod;
    cv::mulSpectrums(fft_ref, conj_fft_tar, prod, 0);

    // Step 4: Compute the inverse FFT to get the cross-correlation map
    cv::Mat correlation;
    cv::idft(prod, correlation, cv::DFT_REAL_OUTPUT | cv::DFT_SCALE);

    // Step 5: Find the location of the maximum value in the correlation map
    double min_val, max_val;
    cv::Point min_loc, max_loc;
    cv::minMaxLoc(correlation, &min_val, &max_val, &min_loc, &max_loc);
    int dx = max_loc.x;
    int dy = max_loc.y;

    // Step 6: Refine the shift to subpixel accuracy using parabolic interpolation
    float delta_x = 0.0f;
    float delta_y = 0.0f;

    // Compute delta_x (x-direction subpixel offset)
    int left = (dx - 1 + W) % W;  // Periodic boundary handling
    int right = (dx + 1) % W;
    float a = correlation.at<float>(dy, left);
    float b = correlation.at<float>(dy, dx);
    float c = correlation.at<float>(dy, right);
    float denom_x = a - 2 * b + c;
    if (b > a && b > c && std::abs(denom_x) > 1e-5f) { // Ensure peak and avoid division by zero
        delta_x = (c - a) / (2 * denom_x);
    }

    // Compute delta_y (y-direction subpixel offset)
    int up = (dy - 1 + H) % H;
    int down = (dy + 1) % H;
    float d = correlation.at<float>(up, dx);
    float e = correlation.at<float>(dy, dx);
    float f = correlation.at<float>(down, dx);
    float denom_y = d - 2 * e + f;
    if (e > d && e > f && std::abs(denom_y) > 1e-5f) {
        delta_y = (f - d) / (2 * denom_y);
    }

    // Step 7: Compute the total shift and adjust for periodicity
    float shift_x = dx + delta_x;
    float shift_y = dy + delta_y;

    // Adjust shifts to lie within [-W/2, W/2) and [-H/2, H/2)
    if (shift_x >= W / 2.0f) shift_x -= W;
    if (shift_y >= H / 2.0f) shift_y -= H;

    // Return the shift as a cv::Point2f
    return cv::Point2f(shift_x, shift_y);
}
