#include "alignment.h"

cv::Point2f computeShift(cv::Mat reference, cv::Mat target, double confidence) {
    // Ensure input images are 32F grayscale
    cv::Mat refGray, tgtGray;
    if (reference.channels() > 1) {
        cv::cvtColor(reference, refGray, cv::COLOR_BGR2GRAY);
    }
    else {
        refGray = reference.clone();
    }

    if (target.channels() > 1) {
        cv::cvtColor(target, tgtGray, cv::COLOR_BGR2GRAY);
    }
    else {
        tgtGray = target.clone();
    }

    refGray.convertTo(refGray, CV_32F);
    tgtGray.convertTo(tgtGray, CV_32F);

    // Apply Hanning window to both images
    cv::Mat hann;
    cv::createHanningWindow(hann, refGray.size(), CV_32F);
    refGray = refGray.mul(hann);
    tgtGray = tgtGray.mul(hann);

    // Perform phase correlation
    cv::Point2d shift;
    double response = 0.0;
    shift = cv::phaseCorrelate(refGray, tgtGray, hann, &response);

    // Check if the response is strong enough
    if (response < confidence) {
        return cv::Point2f(0, 0);
    }

    return cv::Point2f(static_cast<float>(shift.x), static_cast<float>(shift.y));
}

//
// Arranges alignment points over 'frame' based on the specified placement mode.
//
// Two modes (AlignmentPointSet::Placement enum):
// - FeatureBased: uses corner detection (cv::goodFeaturesToTrack());
// - Uniform: places the points evenly over the detected object.
//
AlignmentPointSet AlignmentPointSet::estimate(cv::Mat frame, Config config) {
    AlignmentPointSet set;

    cv::Mat processed;
    cv::cvtColor(frame, processed, cv::COLOR_BGR2GRAY);

    std::vector<cv::Point> cvAps;

    if (config.placement == Placement::FeatureBased) {
        // Detect strong corners
        processed.convertTo(processed, CV_32FC1);
        cv::blur(processed, processed, {3, 3});
        cv::goodFeaturesToTrack(processed, cvAps, 150, 0.75, config.size / 2);
    }
    else {
        cv::threshold(processed, processed, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
        cv::blur(processed, processed, {5, 5});

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(processed, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        if (contours.empty()) {
            return {};
        }

        // Lambda to find the largest contour by area
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

        // Distance between alignment points
        const int step = config.size / 2;

        // Create mask of the largest contour
        cv::Mat mask = cv::Mat::zeros(frame.size(), CV_8UC1);
        cv::drawContours(mask, contours, largestContour(contours), 255, cv::FILLED);

        // Erode the mask so the points aren't placed too close to the edges
        cv::Mat kernel = cv::Mat::ones(2 * step, 2 * step, CV_8UC1);
        cv::erode(mask, mask, cv::Mat::ones(step, step, CV_8UC1));

        while (cv::countNonZero(mask) > 0) {
            std::vector<std::vector<cv::Point>> layerContours;
            cv::findContours(mask, layerContours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);

            // Find the largest contour in the current layer
            std::vector<cv::Point> contour = layerContours[largestContour(layerContours)];

            // Arc length of the countour
            double arcLen = cv::arcLength(contour, true);
            // Number of points to place
            int amount = std::max(1, static_cast<int>(arcLen / config.size * 1.4));

            for (int i = 0; i < amount; ++i) {
                // Target distance along the contour for the current point
                double targetDist = (arcLen * i) / amount;
                double running = 0.0;
                // Iterate through contour segments to find the point at the target distance
                for (size_t j = 1; j < contour.size(); ++j) {
                    cv::Point2f p1 = contour[j - 1];
                    cv::Point2f p2 = contour[j];
                    double segLen = cv::norm(p2 - p1);
                    if (running + segLen >= targetDist) {
                        // Interpolate to find the exact point
                        double alpha = (targetDist - running) / segLen;
                        cv::Point2f pt = p1 + alpha * (p2 - p1);
                        cvAps.push_back(pt);
                        break;
                    }
                    running += segLen;
                }
            }

            // Further erode to go 'deeper'
            cv::erode(mask, mask, kernel);
        }
    }

    for (auto &cvAp : cvAps) {
        set.add({cvAp.x, cvAp.y, config.size});
    }

    return set;
}
