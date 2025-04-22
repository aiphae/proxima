#include "stacker.h"
#include "helpers.h"
#include <opencv2/imgproc.hpp>
#include "frame.h"

cv::Mat Stacker::stack() {
    return aps.empty() ? stackGlobal() : stackLocal();
}

cv::Mat Stacker::stackGlobal() {
    cv::Mat accumulator = cv::Mat::zeros(reference.size(), CV_32FC3);
    double weights = 0.0;

    cv::Mat referenceGray;
    cv::cvtColor(reference, referenceGray, cv::COLOR_BGR2GRAY);
    referenceGray.convertTo(referenceGray, CV_32F);

    for (int i = 1; i < framesToStack; ++i) {
        double frameWeight = sorted[i].second;

        cv::Mat current = getMatAtFrame(files, sorted[i].first);
        if (current.empty()) {
            continue;
        }

        cv::Mat currentGray;
        cv::cvtColor(current, currentGray, cv::COLOR_BGR2GRAY);
        currentGray.convertTo(currentGray, CV_32F);

        cv::Point2f shift = cv::phaseCorrelate(referenceGray, currentGray);

        cv::Mat M = (cv::Mat_<double>(2, 3) <<
            1, 0, -shift.x,
            0, 1, -shift.y
        );

        cv::Mat aligned;
        cv::warpAffine(current, aligned, M, accumulator.size(), cv::INTER_CUBIC);

        aligned.convertTo(aligned, CV_32FC3);

        accumulator += aligned * frameWeight;
        weights += frameWeight;
    }

    if (weights == 0.0) {
        return {};
    }

    cv::Mat result;
    accumulator /= weights;
    accumulator.convertTo(result, CV_8UC3);

    return result;
}

cv::Mat Stacker::stackLocal() {
    cv::Mat accumulator = cv::Mat::zeros(reference.size(), CV_32FC3);
    cv::Mat weights = cv::Mat::zeros(reference.size(), CV_32F);

    cv::Mat referenceGray;
    cv::cvtColor(reference, referenceGray, cv::COLOR_BGR2GRAY);
    referenceGray.convertTo(referenceGray, CV_32F);

    for (int i = 1; i < framesToStack; ++i) {
        cv::Mat current = getMatAtFrame(files, sorted[i].first);
        if (current.empty()) {
            continue;
        }

        cv::Mat currentGray;
        cv::cvtColor(current, currentGray, cv::COLOR_BGR2GRAY);
        currentGray.convertTo(currentGray, CV_32F);

        cv::Point2f globalShift = cv::phaseCorrelate(referenceGray, currentGray);
        cv::Mat globalM = (cv::Mat_<double>(2, 3) <<
            1, 0, -globalShift.x,
            0, 1, -globalShift.y
        );

        cv::Mat globalAligned, globalAlignedGray;
        cv::warpAffine(current, globalAligned, globalM, current.size(), cv::INTER_LANCZOS4);
        cv::warpAffine(currentGray, globalAlignedGray, globalM, current.size(), cv::INTER_LANCZOS4);

        // Weight based on frame quality so higher-quality frames impact more
        float frameWeight = sorted[i].second;
        // Padding to extract a larger ROI for more precise interpolation
        constexpr int padding = 5;

        for (const auto &ap : aps) {
            cv::Rect roi = ap.rect() & cv::Rect(0, 0, accumulator.cols, accumulator.rows);

            // Extended ROI to support interpolation
            cv::Rect paddedRoi(
                std::max(roi.x - padding, 0),
                std::max(roi.y - padding, 0),
                std::min(roi.width + 2 * padding, accumulator.cols - roi.x + padding),
                std::min(roi.height + 2 * padding, accumulator.rows - roi.y + padding)
            );

            // Compute local shift from original ROI
            cv::Point2f localShift = Frame::computeShift(referenceGray(roi), globalAlignedGray(roi));

            // Warp padded ROI
            cv::Mat warpedPatch;
            cv::Mat localM = (cv::Mat_<double>(2, 3) <<
                1, 0, -localShift.x,
                0, 1, -localShift.y
            );
            cv::warpAffine(globalAligned(paddedRoi), warpedPatch, localM, paddedRoi.size(), cv::INTER_CUBIC, cv::BORDER_REFLECT);
            warpedPatch.convertTo(warpedPatch, CV_32FC3);

            // Crop back to original ROI size
            cv::Rect roiInPadded(
                roi.x - paddedRoi.x,
                roi.y - paddedRoi.y,
                roi.width,
                roi.height
            );
            cv::Mat cropped = warpedPatch(roiInPadded);

            // Accumulate into the correct position
            for (int y = 0; y < roi.height; ++y) {
                for (int x = 0; x < roi.width; ++x) {
                    cv::Vec3f val = cropped.at<cv::Vec3f>(y, x);
                    float w = frameWeight;
                    if (val[0] > 0.0f && val[1] > 0.0f && val[2] > 0.0f) {
                        int ax = roi.x + x;
                        int ay = roi.y + y;
                        weights.at<float>(ay, ax) += w;
                        accumulator.at<cv::Vec3f>(ay, ax) += val * w;
                    }
                }
            }
        }
    }

    cv::Mat stackedGlobal = stackGlobal();
    stackedGlobal.convertTo(stackedGlobal, CV_32FC3);
    cv::Mat result = cv::Mat::zeros(accumulator.size(), CV_32FC3);

    for (int y = 0; y < result.rows; ++y) {
        for (int x = 0; x < result.cols; ++x) {
            float w = weights.at<float>(y, x);
            if (w > 0) {
                result.at<cv::Vec3f>(y, x) = accumulator.at<cv::Vec3f>(y, x) / w;
            }
            else {
                result.at<cv::Vec3f>(y, x) = stackedGlobal.at<cv::Vec3f>(y, x);
            }
        }
    }

    result.convertTo(result, CV_8UC3);
    return result;
}
