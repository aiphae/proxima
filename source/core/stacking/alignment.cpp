#include "alignment.h"

//
// Computed the translational shifty between 'reference' and 'target'
// using phase correlation.
//
// Parabola fitting is used for subpixel accuracy.
//
// Returns a cv::Point2f representing the (x, y) shift from 'reference' to 'target'
// so (-x, -y) is needed to warp 'target' to match 'reference'.
//
cv::Point2f computeShift(cv::Mat reference, cv::Mat target) {
    // Ensure images are the same size and type
    CV_Assert(reference.size() == target.size() && reference.type() == CV_32F && target.type() == CV_32F);

    int W = reference.cols;
    int H = reference.rows;

    // Compute the FFT of both images
    cv::Mat fft_ref, fft_tar;
    cv::dft(reference, fft_ref, cv::DFT_COMPLEX_OUTPUT);
    cv::dft(target, fft_tar, cv::DFT_COMPLEX_OUTPUT);

    // Compute the conjugate of the FFT of the target
    cv::Mat planes[2];
    cv::split(fft_tar, planes);
    planes[1] = -planes[1]; // Negate the imaginary part to compute conjugate
    cv::Mat conj_fft_tar;
    cv::merge(planes, 2, conj_fft_tar);

    // Compute the element-wise product of fft_ref and conj_fft_tar
    cv::Mat prod;
    cv::mulSpectrums(fft_ref, conj_fft_tar, prod, 0);

    // Compute the inverse FFT to get the cross-correlation map
    cv::Mat correlation;
    cv::idft(prod, correlation, cv::DFT_REAL_OUTPUT | cv::DFT_SCALE);

    // Find the location of the maximum value in the correlation map
    double min_val, max_val;
    cv::Point min_loc, max_loc;
    cv::minMaxLoc(correlation, &min_val, &max_val, &min_loc, &max_loc);
    int dx = max_loc.x;
    int dy = max_loc.y;

    // Refine the shift to subpixel accuracy using parabolic interpolation
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

    // Compute the total shift and adjust for periodicity
    float shift_x = dx + delta_x;
    float shift_y = dy + delta_y;

    // Adjust shifts to lie within [-W/2, W/2) and [-H/2, H/2)
    if (shift_x >= W / 2.0f) shift_x -= W;
    if (shift_y >= H / 2.0f) shift_y -= H;

    // Return the shift as a cv::Point2f
    return cv::Point2f(shift_x, shift_y);
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
        cv::goodFeaturesToTrack(processed, cvAps, 150, 0.25, config.size / 2);
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
