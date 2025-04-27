#include "stacker.h"
#include "components/helpers.h"
#include "components/frame.h"
#include <opencv2/imgproc.hpp>

cv::Mat Stacker::stack() {
    return config.localAlign ? stackLocal() : stackGlobal(true);
}

cv::Mat Stacker::stackGlobal(bool crop) {
    cv::Mat reference = getMatAtFrame(source.files, source.sorted[0].first);

    cv::Size drizzleSize(reference.cols * config.drizzle, reference.rows * config.drizzle);
    cv::Mat accumulator = cv::Mat::zeros(drizzleSize, CV_32FC3);
    double weights = 0.0;

    cv::Mat referenceGray;
    cv::cvtColor(reference, referenceGray, cv::COLOR_BGR2GRAY);
    referenceGray.convertTo(referenceGray, CV_32F);

    for (int i = 0; i < config.framesToStack; ++i) {
        cv::Mat current = getMatAtFrame(source.files, source.sorted[i].first);
        if (current.empty()) {
            continue;
        }

        double frameWeight = source.sorted[i].second;

        cv::Mat currentGray;
        cv::cvtColor(current, currentGray, cv::COLOR_BGR2GRAY);
        currentGray.convertTo(currentGray, CV_32F);

        cv::Point2f shift = cv::phaseCorrelate(referenceGray, currentGray);

        cv::Mat M = (cv::Mat_<double>(2, 3) <<
            1, 0, -shift.x * config.drizzle,
            0, 1, -shift.y * config.drizzle
        );

        cv::Mat enlarged;
        cv::resize(current, enlarged, drizzleSize, 0, 0, cv::INTER_NEAREST);

        cv::Mat aligned;
        cv::warpAffine(enlarged, aligned, M, drizzleSize, cv::INTER_CUBIC);

        aligned.convertTo(aligned, CV_32FC3);

        accumulator += aligned * frameWeight;
        weights += frameWeight;

        emit progressUpdated(i + 1, config.framesToStack);
    }

    if (weights == 0.0) {
        return {};
    }

    cv::Mat result;
    accumulator /= weights;
    accumulator.convertTo(result, CV_8UC3);

    if (crop) {
        result = Frame::centerObject(result, config.outputWidth * config.drizzle, config.outputHeight * config.drizzle);
    }

    return result;
}

cv::Mat Stacker::stackLocal() {
    cv::Mat reference = getMatAtFrame(source.files, source.sorted[0].first);

    cv::Size drizzleSize(reference.cols * config.drizzle, reference.rows * config.drizzle);

    cv::Mat accumulator = cv::Mat::zeros(drizzleSize, CV_32FC3);
    cv::Mat weights = cv::Mat::zeros(drizzleSize, CV_32F);

    // Prepare reference gray
    cv::Mat referenceGray;
    cv::cvtColor(reference, referenceGray, cv::COLOR_BGR2GRAY);
    referenceGray.convertTo(referenceGray, CV_32F);

    // Upscale reference gray
    cv::Mat referenceGrayDrizzle;
    if (config.drizzle > 1) {
        cv::resize(referenceGray, referenceGrayDrizzle, drizzleSize, 0, 0, cv::INTER_NEAREST);
    }
    else {
        referenceGrayDrizzle = referenceGray;
    }

    for (int i = 1; i < config.framesToStack; ++i) {
        cv::Mat current = getMatAtFrame(source.files, source.sorted[i].first);
        if (current.empty()) {
            continue;
        }

        // Current gray
        cv::Mat currentGray;
        cv::cvtColor(current, currentGray, cv::COLOR_BGR2GRAY);
        currentGray.convertTo(currentGray, CV_32F);

        // Global alignment (phase correlation)
        cv::Point2f globalShift = cv::phaseCorrelate(referenceGray, currentGray);

        // Global shift scaled by drizzleFactor
        cv::Mat globalM = (cv::Mat_<double>(2, 3) <<
            1, 0, -globalShift.x * config.drizzle,
            0, 1, -globalShift.y * config.drizzle
        );

        // Upscale current frame
        cv::Mat enlarged, enlargedGray;
        if (config.drizzle > 1) {
            cv::resize(current, enlarged, drizzleSize, 0, 0, cv::INTER_NEAREST);
            cv::resize(currentGray, enlargedGray, drizzleSize, 0, 0, cv::INTER_NEAREST);
        } else {
            enlarged = current;
            enlargedGray = currentGray;
        }

        // Apply global shift
        cv::Mat globalAligned, globalAlignedGray;
        cv::warpAffine(enlarged, globalAligned, globalM, drizzleSize, cv::INTER_LANCZOS4, cv::BORDER_REFLECT);
        cv::warpAffine(enlargedGray, globalAlignedGray, globalM, drizzleSize, cv::INTER_LANCZOS4, cv::BORDER_REFLECT);

        float frameWeight = source.sorted[i].second;
        constexpr int padding = 5; // padding in drizzle space

        for (const auto &ap : config.aps) {
            // Adjust ROI for drizzle factor
            cv::Rect roi = ap.rect();
            roi.x *= config.drizzle;
            roi.y *= config.drizzle;
            roi.width *= config.drizzle;
            roi.height *= config.drizzle;

            cv::Rect safeRoi = roi & cv::Rect(0, 0, accumulator.cols, accumulator.rows);

            if (safeRoi.width <= 0 || safeRoi.height <= 0) {
                continue;
            }

            // Padded ROI to allow subpixel interpolation
            cv::Rect paddedRoi(
                std::max(safeRoi.x - padding, 0),
                std::max(safeRoi.y - padding, 0),
                std::min(safeRoi.width + 2 * padding, accumulator.cols - safeRoi.x + padding),
                std::min(safeRoi.height + 2 * padding, accumulator.rows - safeRoi.y + padding)
            );

            // Compute local shift based on non-drizzle reference
            // (scale back ROI for reference)
            cv::Rect originalRoi(
                roi.x / config.drizzle,
                roi.y / config.drizzle,
                roi.width / config.drizzle,
                roi.height / config.drizzle
            );
            cv::Point2f localShift = Frame::computeShift(referenceGray(originalRoi), currentGray(originalRoi));
            localShift.x *= config.drizzle;
            localShift.y *= config.drizzle;

            // Apply local warp
            cv::Mat warpedPatch;
            cv::Mat localM = (cv::Mat_<double>(2, 3) <<
                1, 0, -localShift.x,
                0, 1, -localShift.y
            );
            cv::warpAffine(globalAligned(paddedRoi), warpedPatch, localM, paddedRoi.size(), cv::INTER_CUBIC, cv::BORDER_REFLECT);
            warpedPatch.convertTo(warpedPatch, CV_32FC3);

            // Crop back to safe ROI inside padded
            cv::Rect roiInPadded(
                safeRoi.x - paddedRoi.x,
                safeRoi.y - paddedRoi.y,
                safeRoi.width,
                safeRoi.height
                );
            cv::Mat cropped = warpedPatch(roiInPadded);

            // Accumulate
            for (int y = 0; y < safeRoi.height; ++y) {
                for (int x = 0; x < safeRoi.width; ++x) {
                    cv::Vec3f val = cropped.at<cv::Vec3f>(y, x);
                    if (val[0] > 0.0f && val[1] > 0.0f && val[2] > 0.0f) {
                        int ax = safeRoi.x + x;
                        int ay = safeRoi.y + y;
                        weights.at<float>(ay, ax) += frameWeight;
                        accumulator.at<cv::Vec3f>(ay, ax) += val * frameWeight;
                    }
                }
            }
        }

        emit progressUpdated(i + 1, config.framesToStack);
    }

    // Stack global fallback
    cv::Mat stackedGlobal = stackGlobal(false);
    stackedGlobal.convertTo(stackedGlobal, CV_32FC3);

    // Final normalization
    cv::Mat result = cv::Mat::zeros(accumulator.size(), CV_32FC3);

    for (int y = 0; y < result.rows; ++y) {
        for (int x = 0; x < result.cols; ++x) {
            float w = weights.at<float>(y, x);
            if (w > 0.0f) {
                result.at<cv::Vec3f>(y, x) = accumulator.at<cv::Vec3f>(y, x) / w;
            } else {
                result.at<cv::Vec3f>(y, x) = stackedGlobal.at<cv::Vec3f>(y, x);
            }
        }
    }

    result.convertTo(result, CV_8UC3);

    result = Frame::centerObject(result, config.outputWidth * config.drizzle, config.outputHeight * config.drizzle);

    return result;
}
