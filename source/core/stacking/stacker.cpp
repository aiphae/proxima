#include "stacker.h"
#include "components/frame.h"

void Stacker::initialize(cv::Mat reference, const _StackConfig &config) {
    _reference = reference;
    _reference = Frame::centerObject(reference, reference.cols, reference.rows);

    _config = config;

    // Initialize internal accumulators
    cv::Size upsampledSize{
        static_cast<int>(_reference.cols * _config.upsample),
        static_cast<int>(_reference.rows * _config.upsample)
    };
    _localAccumulator = cv::Mat::zeros(upsampledSize, CV_32FC3);
    _localWeights = cv::Mat::zeros(upsampledSize, CV_32F);
    _globalAccumulator = _localAccumulator.clone();
    _globalWeights = _localWeights.clone();

    // Include reference frame
    _globalAccumulator += _reference * 1.0f;
    _globalWeights += 1.0f;
}

void Stacker::add(cv::Mat mat, double weight) {
    // ...
}

cv::Mat Stacker::average() {
    // Final result mat
    cv::Mat result(_globalAccumulator.size(), CV_32FC3);

    // Combine local and global accumulations
    if (_config.aps) {
        for (int y = 0; y < result.rows; ++y) {
            for (int x = 0; x < result.cols; ++x) {
                float localW = _localWeights.at<float>(y, x);
                if (localW > 0.0f) {
                    result.at<cv::Vec3f>(y, x) = _localAccumulator.at<cv::Vec3f>(y, x) / localW;
                }
                else {
                    float globalW = _globalWeights.at<float>(y, x);
                    result.at<cv::Vec3f>(y, x) = _globalAccumulator.at<cv::Vec3f>(y, x) / globalW;
                }
            }
        }
    }
    // Normalize global accumulation only
    else {
        cv::Mat weights3;
        cv::Mat weightsChannels[] {_globalWeights, _globalWeights, _globalWeights};
        cv::merge(weightsChannels, 3, weights3);
        cv::divide(_globalAccumulator, weights3, result);
    }

    // Expand borders (padding with black)
    cv::Size expandSize{
        static_cast<int>(_config.outputWidth * _config.upsample),
        static_cast<int>(_config.outputHeight * _config.upsample)
    };
    result = Frame::expandBorders(result, expandSize.width, expandSize.height);

    return result;
}

void Stacker::reset() {
    _reference.release();
    _globalAccumulator.release();
    _globalWeights.release();
    _localAccumulator.release();
    _localWeights.release();
    _config = _StackConfig();
}

//
// Performs stacking on a series of frames managed by a MediaCollection,
// with support for global and local alignment using a weighted averaging approach.
//

// TODO!
// Change structure
// add internal storages for stacking
// Stacker::addFrame() method <- ALLOWS MULTITHREADING and NO NEED FOR A SEPARATE THREAD OBJECT (just std::thread(...).detach();)
// and/or callbacks
// Stacker stacker(
//
// (MAYBE!) StackJob for every stacking run

cv::Mat Stacker::stack(MediaCollection &manager, StackConfig &config, IterationCallback callback) {
    cv::Mat reference = manager.matAtFrame(config.sorted[0].first);
    if (reference.empty()) {
        return {};
    }
    // This is applied because APs have been estimated on the centered reference frame
    reference = Frame::centerObject(reference, reference.cols, reference.rows);

    double upsample = config.drizzle;
    cv::Size upsampledSize(
        reference.cols * upsample,
        reference.rows * upsample
    );

    // Accumulators for both local and global stacks
    cv::Mat localAccumulator = cv::Mat::zeros(upsampledSize, CV_32FC3);
    cv::Mat localWeights = cv::Mat::zeros(upsampledSize, CV_32F);
    cv::Mat globalAccumulator = localAccumulator.clone();
    cv::Mat globalWeights = localWeights.clone();

    int framesToStack = static_cast<int>(config.sorted.size());

    std::stable_sort(config.sorted.begin(), config.sorted.end(), [](const auto &a, const auto &b) {
        return a.first < b.first;
    });

    std::vector<int> preparedData;
    for (const auto& p : config.sorted) {
        preparedData.push_back(p.first);
    }

    std::vector<cv::Mat> frames = manager.matsAtFrames(preparedData);

    for (int i = 0; i < framesToStack; ++i) {
        cv::Mat current = frames[i];
        if (current.empty()) {
            continue;
        }

        double frameWeight = config.sorted[i].second;

        // Compute global shift relative to reference
        cv::Point2f globalShift = computeShift(reference, current, 0.35);
        cv::Mat globalM = (cv::Mat_<double>(2, 3) <<
            1, 0, -globalShift.x,
            0, 1, -globalShift.y
        );

        // Apply global alignment
        cv::Mat globalAligned;
        cv::warpAffine(current, globalAligned, globalM, reference.size(), cv::INTER_LANCZOS4);
        globalAligned.convertTo(globalAligned, CV_32FC3);

        // Upsample the globally aligned frame
        cv::Mat globalAlignedUpsampled;
        cv::resize(globalAligned, globalAlignedUpsampled, upsampledSize, cv::INTER_LANCZOS4);

        globalAccumulator += globalAlignedUpsampled * frameWeight;
        globalWeights += frameWeight;

        // Optional: Local alignment using alignment points (APs)
        if (config.localAlign) {
            // Padding to exclude borders interpolation (e.g. BORDER_REPLICATE)
            constexpr int padding = 5;

            for (const auto &ap : config.aps) {
                // Original AP's ROI
                cv::Rect roi = ap.rect(), paddedRoi = roi;
                roi &= cv::Rect(0, 0, reference.cols, reference.rows);

                // Padded ROI
                paddedRoi = {
                    paddedRoi.x - padding,
                    paddedRoi.y - padding,
                    paddedRoi.width + 2 * padding,
                    paddedRoi.height + 2 * padding
                };
                paddedRoi &= cv::Rect(0, 0, reference.cols, reference.rows);

                // padX(Y)[0] is applied padding to the left (top), [1] is to the right (bottom)
                int padX[2] {roi.x - paddedRoi.x, paddedRoi.x + paddedRoi.width - roi.x - roi.width};
                int padY[2] {roi.y - paddedRoi.y, paddedRoi.y + paddedRoi.height - roi.y - roi.height};

                // Local shift between the ROI in reference and globally aligned frame
                cv::Point2f localShift = computeShift(reference(roi), globalAligned(roi));
                cv::Mat localM = (cv::Mat_<double>(2, 3) <<
                    1, 0, -localShift.x * upsample,
                    0, 1, -localShift.y * upsample
                );

                // Extract and upsample padded patch
                cv::Mat paddedPatch = globalAligned(paddedRoi).clone();
                cv::resize(paddedPatch, paddedPatch,
                    {static_cast<int>(paddedRoi.width * upsample), static_cast<int>(paddedRoi.height * upsample)},
                    cv::INTER_LANCZOS4
                );
                // Apply the local shift
                cv::warpAffine(paddedPatch, paddedPatch, localM, paddedPatch.size(), cv::INTER_LANCZOS4, cv::BORDER_REPLICATE);

                // Extract the aligned inner patch
                cv::Rect roiInPadded(
                    padX[0] * upsample,
                    padY[0] * upsample,
                    (paddedRoi.width - padX[0] - padX[1]) * upsample,
                    (paddedRoi.height - padY[0] - padY[1]) * upsample
                );

                cv::Mat patchInPadded = paddedPatch(roiInPadded).clone();

                // Scale the original ROI to match upsampled space
                roi.x *= upsample;
                roi.y *= upsample;
                roi.width *= upsample;
                roi.height *= upsample;

                localAccumulator(roi) += patchInPadded * frameWeight;
                localWeights(roi) += frameWeight;
            }
        }

        // Optional: execute the callback function
        if (callback) {
            callback(i + 1, framesToStack);
        }
    }


}
