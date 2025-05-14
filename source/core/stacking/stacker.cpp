#include "stacker.h"
#include "components/frame.h"

cv::Mat Stacker::stack(MediaManager &manager, StackConfig &config) {
    return config.localAlign ? stackLocal(manager, config, true) : stackGlobal(manager, config, true, true);
}

//
// Performs global stacking.
//
// Each frame is warped globally via phase correlation.
//
// 'bool emitSignals' - whether to send the GUI-updating signal to the main thread.
// 'bool crop' - whether to crop the resulting image according to the config values.
//
cv::Mat Stacker::stackGlobal(MediaManager &manager, StackConfig &config, bool emitSignals, bool crop) {
    cv::Mat reference = manager.matAtFrame(config.sorted[0].first);
    if (reference.empty()) {
        return {};
    }

    cv::Size drizzleSize {
        static_cast<int>(reference.cols * config.drizzle),
        static_cast<int>(reference.rows * config.drizzle)
    };

    cv::Mat accumulator = cv::Mat::zeros(drizzleSize, CV_32FC3);
    double weights = 0.0;

    cv::Mat referenceGray;
    cv::cvtColor(reference, referenceGray, cv::COLOR_BGR2GRAY);
    referenceGray.convertTo(referenceGray, CV_32F);

    int framesToStack = config.sorted.size();
    for (int i = 0; i < framesToStack; ++i) {
        cv::Mat current = manager.matAtFrame(config.sorted[i].first);
        if (current.empty()) {
            continue;
        }

        // Higher-quality frames have more impact
        double frameWeight = config.sorted[i].second;

        cv::Mat currentGray;
        cv::cvtColor(current, currentGray, cv::COLOR_BGR2GRAY);
        currentGray.convertTo(currentGray, CV_32F);

        // Compute the shift between the current image and the reference one
        cv::Point2f shift = cv::phaseCorrelate(referenceGray, currentGray);
        cv::Mat M = (cv::Mat_<double>(2, 3) <<
            1, 0, -shift.x * config.drizzle,
            0, 1, -shift.y * config.drizzle
        );

        // Enlarge to allow drizzle
        cv::Mat enlarged;
        cv::resize(current, enlarged, drizzleSize, 0, 0, cv::INTER_NEAREST);

        // Warp the current image
        cv::Mat aligned;
        cv::warpAffine(enlarged, aligned, M, drizzleSize, cv::INTER_CUBIC);
        aligned.convertTo(aligned, CV_32FC3);

        // Accumulate the results
        accumulator += aligned * frameWeight;
        weights += frameWeight;

        // Emit the signal to the main thread
        if (emitSignals) {
            QString status = QString("%1/%2").arg(i + 1).arg(framesToStack);
            emit progressUpdated(status);
        }
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

//
// Performs local stacking.
//
// For each frame, each AP from 'source.aps' is shifted
// individually and accumulated into the according position.
//
// 'bool emitSignals' - whether to send the GUI-updating signal to the main thread.
//
cv::Mat Stacker::stackLocal(MediaManager &manager, StackConfig &config, bool emitSignals) {
    cv::Mat reference = manager.matAtFrame(config.sorted[0].first);
    if (reference.empty()) {
        return {};
    }

    cv::Size drizzleSize {
        static_cast<int>(reference.cols * config.drizzle),
        static_cast<int>(reference.rows * config.drizzle)
    };

    cv::Mat accumulator = cv::Mat::zeros(drizzleSize, CV_32FC3);
    cv::Mat weights = cv::Mat::zeros(drizzleSize, CV_32F);

    cv::Mat referenceGray;
    cv::cvtColor(reference, referenceGray, cv::COLOR_BGR2GRAY);
    referenceGray.convertTo(referenceGray, CV_32F);

    int framesToStack = config.sorted.size();
    for (int i = 0; i < framesToStack; ++i) {
        cv::Mat current = manager.matAtFrame(config.sorted[i].first);
        if (current.empty()) {
            continue;
        }

        cv::Mat currentGray;
        cv::cvtColor(current, currentGray, cv::COLOR_BGR2GRAY);
        currentGray.convertTo(currentGray, CV_32F);

        // Compute global shift between images
        cv::Point2f globalShift = cv::phaseCorrelate(referenceGray, currentGray);

        // Warp current color frame to drizzle size
        cv::Mat enlarged;
        if (config.drizzle > 1) {
            cv::resize(current, enlarged, drizzleSize, 0, 0, cv::INTER_NEAREST);
        }
        else {
            enlarged = current;
        }

        cv::Mat globalM = (cv::Mat_<double>(2, 3) <<
            1, 0, -globalShift.x * config.drizzle,
            0, 1, -globalShift.y * config.drizzle
        );

        cv::Mat globalAligned;
        cv::warpAffine(enlarged, globalAligned, globalM, drizzleSize, cv::INTER_LANCZOS4, cv::BORDER_REFLECT);

        // Align currentGray (non-drizzled) for local shift computation
        cv::Mat currentGrayAligned;
        cv::Mat globalMUndrizzled = (cv::Mat_<double>(2, 3) <<
            1, 0, -globalShift.x,
            0, 1, -globalShift.y
        );
        cv::warpAffine(currentGray, currentGrayAligned, globalMUndrizzled, currentGray.size(), cv::INTER_LANCZOS4, cv::BORDER_REFLECT);

        float frameWeight = config.sorted[i].second;
        constexpr int padding = 5;

        for (const auto &ap : config.aps) {
            // ROI in non-drizzled domain
            cv::Rect originalRoi = ap.rect();
            cv::Rect validOriginalRoi = originalRoi & cv::Rect(0, 0, referenceGray.cols, referenceGray.rows);
            if (validOriginalRoi.width <= 1 || validOriginalRoi.height <= 1) {
                continue;
            }

            // Compute local shift between reference and globally aligned gray image
            cv::Point2f localShift = Frame::computeShift(
                referenceGray(validOriginalRoi),
                currentGrayAligned(validOriginalRoi)
            );
            localShift.x *= config.drizzle;
            localShift.y *= config.drizzle;

            // ROI in drizzle scale
            cv::Rect roi = ap.rect();
            roi.x *= config.drizzle;
            roi.y *= config.drizzle;
            roi.width *= config.drizzle;
            roi.height *= config.drizzle;

            cv::Rect safeRoi = roi & cv::Rect(0, 0, accumulator.cols, accumulator.rows);
            if (safeRoi.width <= 0 || safeRoi.height <= 0) {
                continue;
            }

            // Expand ROI to allow interpolation at edges
            cv::Rect paddedRoi(
                std::max(safeRoi.x - padding, 0),
                std::max(safeRoi.y - padding, 0),
                std::min(safeRoi.width + 2 * padding, accumulator.cols - (safeRoi.x - padding)),
                std::min(safeRoi.height + 2 * padding, accumulator.rows - (safeRoi.y - padding))
            );

            // Apply local shift to the globally aligned image
            cv::Mat warpedPatch;
            cv::Mat localM = (cv::Mat_<double>(2, 3) <<
                1, 0, -localShift.x,
                0, 1, -localShift.y
            );
            cv::warpAffine(globalAligned(paddedRoi), warpedPatch, localM, paddedRoi.size(), cv::INTER_CUBIC, cv::BORDER_REFLECT);
            warpedPatch.convertTo(warpedPatch, CV_32FC3);

            // Crop back to original ROI
            cv::Rect roiInPadded(
                safeRoi.x - paddedRoi.x,
                safeRoi.y - paddedRoi.y,
                safeRoi.width,
                safeRoi.height
            );
            cv::Mat cropped = warpedPatch(roiInPadded);

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

        if (emitSignals) {
            QString status = QString("%1/%2").arg(i + 1).arg(framesToStack);
            emit progressUpdated(status);
        }
    }

    // Fallback to global stack for unpopulated areas
    cv::Mat stackedGlobal = stackGlobal(manager, config, false, false);
    stackedGlobal.convertTo(stackedGlobal, CV_32FC3);

    cv::Mat result = cv::Mat::zeros(accumulator.size(), CV_32FC3);
    for (int y = 0; y < result.rows; ++y) {
        for (int x = 0; x < result.cols; ++x) {
            float w = weights.at<float>(y, x);
            if (w > 0.0f) {
                result.at<cv::Vec3f>(y, x) = accumulator.at<cv::Vec3f>(y, x) / w;
            }
            else {
                result.at<cv::Vec3f>(y, x) = stackedGlobal.at<cv::Vec3f>(y, x);
            }
        }
    }

    result.convertTo(result, CV_8UC3);
    result = Frame::centerObject(result, config.outputWidth * config.drizzle, config.outputHeight * config.drizzle);
    return result;
}
