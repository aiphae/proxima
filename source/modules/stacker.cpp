#include "stacker.h"
#include "components/helpers.h"
#include "components/frame.h"

cv::Mat Stacker::stack() {
    return config.localAlign ? stackLocal(true) : stackGlobal(true, true);
}

// Performs global stacking.
//
// Each frame is warped globally via phase correlation.
//
// 'bool emitSignals' - whether to send the GUI-updating signal to the main thread.
// 'bool crop' - whether to crop the resulting image according to the config values.
//
cv::Mat Stacker::stackGlobal(bool emitSignals, bool crop) {
    cv::Mat reference = getMatAtFrame(source.files, source.sorted[0].first);
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

    for (int i = 0; i < config.framesToStack; ++i) {
        cv::Mat current = getMatAtFrame(source.files, source.sorted[i].first);
        if (current.empty()) {
            continue;
        }

        // Higher-quality frames have more impact
        double frameWeight = source.sorted[i].second;

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
            QString status = QString("%1/%2").arg(i + 1, config.framesToStack);
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

// Performs local stacking.
//
// For each frame, each AP from 'source.aps' is shifted
// individually and accumulated into the according position.
//
// 'bool emitSignals' - whether to send the GUI-updating signal to the main thread.
//
cv::Mat Stacker::stackLocal(bool emitSignals) {
    cv::Mat reference = getMatAtFrame(source.files, source.sorted[0].first);
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

    for (int i = 0; i < config.framesToStack; ++i) {
        cv::Mat current = getMatAtFrame(source.files, source.sorted[i].first);
        if (current.empty()) {
            continue;
        }

        cv::Mat currentGray;
        cv::cvtColor(current, currentGray, cv::COLOR_BGR2GRAY);
        currentGray.convertTo(currentGray, CV_32F);

        // Compute global shift between images
        cv::Point2f globalShift = cv::phaseCorrelate(referenceGray, currentGray);
        cv::Mat globalM = (cv::Mat_<double>{2, 3} <<
            1, 0, -globalShift.x * config.drizzle,
            0, 1, -globalShift.y * config.drizzle
        );

        // Resize according to the drizzle factor
        cv::Mat enlarged, enlargedGray;
        if (config.drizzle > 1) {
            cv::resize(current, enlarged, drizzleSize, 0, 0, cv::INTER_NEAREST);
            cv::resize(currentGray, enlargedGray, drizzleSize, 0, 0, cv::INTER_NEAREST);
        }
        else {
            enlarged = current;
            enlargedGray = currentGray;
        }

        // Apply the global shift
        cv::Mat globalAligned, globalAlignedGray;
        cv::warpAffine(enlarged, globalAligned, globalM, drizzleSize, cv::INTER_LANCZOS4, cv::BORDER_REFLECT);
        cv::warpAffine(enlargedGray, globalAlignedGray, globalM, drizzleSize, cv::INTER_LANCZOS4, cv::BORDER_REFLECT);

        // Use frame weight so higher-quality frames have more impact on the result
        float frameWeight = source.sorted[i].second;
        // Padding to extract a larger ROI so interpolation doesn't account black pixels at the edges
        constexpr int padding = 5;

        for (const auto &ap : config.aps) {
            cv::Rect roi = ap.rect();
            // Ensure ROI is within bounds
            roi = roi & cv::Rect{0, 0, current.cols, current.rows};

            // Scale ROI for drizzle
            cv::Rect drizzleRoi {
                roi.x *= config.drizzle,
                roi.y += config.drizzle,
                roi.width *= config.drizzle,
                roi.height *= config.drizzle
            };

            // Padded ROI for interpolation
            cv::Rect paddedRoi {
                std::max(roi.x - padding, 0),
                std::max(roi.y - padding, 0),
                std::min(roi.width + 2 * padding, accumulator.cols - roi.x + padding),
                std::min(roi.height + 2 * padding, accumulator.rows - roi.y + padding)
            };

            // Estimate local shift and scale it for drizzle
            cv::Point2f localShift = Frame::computeShift(referenceGray(roi), currentGray(roi));
            localShift.x *= config.drizzle;
            localShift.y *= config.drizzle;

            // Apply local shift to the padded ROI
            cv::Mat warpedPatch {paddedRoi.size(), CV_32FC3};
            cv::Mat localM = (cv::Mat_<double>{2, 3} <<
                1, 0, -localShift.x,
                0, 1, -localShift.y
            );
            cv::warpAffine(globalAligned(paddedRoi), warpedPatch, localM, paddedRoi.size(), cv::INTER_CUBIC, cv::BORDER_REFLECT);

            // Back to the original ROI size
            cv::Rect roiInPadded {
                roi.x - paddedRoi.x,
                roi.y - paddedRoi.y,
                roi.width,
                roi.height
            };
            cv::Mat cropped = warpedPatch(roiInPadded);

            // Accumulate into shifted position
            for (int y = 0; y < roi.height; ++y) {
                for (int x = 0; x < roi.width; ++x) {
                    cv::Vec3f val = cropped.at<cv::Vec3f>(y, x);
                    if (val[0] == 0.0f && val[1] == 0.0f && val[2] == 0.0f) {
                        continue;
                    }

                    // Position where a patch should be accumulated should also be shifted.
                    // Since 'localShift' is floating point, bilinear splatting is applied
                    // to spread a pixel to 4 neighboring points

                    // Where the pixel should land
                    float fx = roi.x + x + localShift.x;
                    float fy = roi.y + y + localShift.y;

                    // Integer part of the pixel
                    int ix = static_cast<int>(std::floor(fx));
                    int iy = static_cast<int>(std::floor(fy));
                    // Fractional part
                    float dx = fx - ix;
                    float dy = fy - iy;

                    // Distribute to 4 neighbors
                    if (ix >= 0 && ix + 1 < accumulator.cols && iy >= 0 && iy + 1 < accumulator.rows) {
                        accumulator.at<cv::Vec3f>(iy, ix) += val * frameWeight * (1 - dx) * (1 - dy);
                        accumulator.at<cv::Vec3f>(iy, ix + 1) += val * frameWeight * dx * (1 - dy);
                        accumulator.at<cv::Vec3f>(iy + 1, ix) += val * frameWeight * (1 - dx) * dy;
                        accumulator.at<cv::Vec3f>(iy + 1, ix + 1) += val * frameWeight * dx * dy;

                        weights.at<float>(iy, ix) += frameWeight * (1 - dx) * (1 - dy);
                        weights.at<float>(iy, ix + 1) += frameWeight * dx * (1 - dy);
                        weights.at<float>(iy + 1, ix) += frameWeight * (1 - dx) * dy;
                        weights.at<float>(iy + 1, ix + 1) += frameWeight * dx * dy;
                    }
                }
            }
        }

        // Emit a signal to the main thread to update the progress bar
        if (emitSignals) {
            QString status = QString("%1/%2").arg(i + 1, config.framesToStack);
            emit progressUpdated(status);
        }
    }

    // Get global stack for fallback
    cv::Mat stackedGlobal = stackGlobal(false, false);
    stackedGlobal.convertTo(stackedGlobal, CV_32FC3);

    // Combine local and global stacks
    cv::Mat result = cv::Mat::zeros(accumulator.size(), CV_32FC3);
    for (int y = 0; y < result.rows; ++y) {
        for (int x = 0; x < result.cols; ++x) {
            float w = weights.at<float>(y, x);
            if (w > 0.0f) {
                result.at<cv::Vec3f>(y, x) = accumulator.at<cv::Vec3f>(y, x) / w;
            }
            else {
                // Use value from global stack
                result.at<cv::Vec3f>(y, x) = stackedGlobal.at<cv::Vec3f>(y, x);
            }
        }
    }

    result.convertTo(result, CV_8UC3);
    result = Frame::centerObject(result, config.outputWidth * config.drizzle, config.outputHeight * config.drizzle);
    return result;
}
