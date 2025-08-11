#include "stacker.h"
#include "components/frame.h"

void Stacker::initialize(const cv::Mat reference, const _StackConfig &config) {
    _reset();

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
    // Compute global shift relative to reference
    cv::Point2f globalShift = computeShift(_reference, mat, 0.35);
    cv::Mat globalM = (cv::Mat_<double>(2, 3) <<
        1, 0, -globalShift.x,
        0, 1, -globalShift.y
    );

    cv::Size upsampledSize = _globalAccumulator.size();

    // Apply global alignment
    cv::Mat globalAligned;
    cv::warpAffine(mat, globalAligned, globalM, _reference.size(), cv::INTER_LANCZOS4);
    globalAligned.convertTo(globalAligned, CV_32FC3);

    // Upsample the globally aligned frame
    cv::Mat globalAlignedUpsampled;
    cv::resize(globalAligned, globalAlignedUpsampled, upsampledSize, cv::INTER_LANCZOS4);

    {
        std::lock_guard<std::mutex> lock(_mtx);
        _globalAccumulator += globalAlignedUpsampled * weight;
        _globalWeights += weight;
    }

    // Optional: Local alignment using alignment points (APs)
    if (_config.aps) {
        // Padding to exclude borders interpolation (e.g. BORDER_REPLICATE)
        constexpr int padding = 5;

        for (const auto &ap : *_config.aps) {
            // Original AP's ROI
            cv::Rect roi = ap.rect(), paddedRoi = roi;
            roi &= cv::Rect(0, 0, _reference.cols, _reference.rows);

            // Padded ROI
            paddedRoi = {
                paddedRoi.x - padding,
                paddedRoi.y - padding,
                paddedRoi.width + 2 * padding,
                paddedRoi.height + 2 * padding
            };
            paddedRoi &= cv::Rect(0, 0, _reference.cols, _reference.rows);

            // padX(Y)[0] is applied padding to the left (top), [1] is to the right (bottom)
            int padX[2] {roi.x - paddedRoi.x, paddedRoi.x + paddedRoi.width - roi.x - roi.width};
            int padY[2] {roi.y - paddedRoi.y, paddedRoi.y + paddedRoi.height - roi.y - roi.height};

            // Local shift between the ROI in reference and globally aligned frame
            cv::Point2f localShift = computeShift(_reference(roi), globalAligned(roi));
            cv::Mat localM = (cv::Mat_<double>(2, 3) <<
                1, 0, -localShift.x * _config.upsample,
                0, 1, -localShift.y * _config.upsample
            );

            // Extract and upsample padded patch
            cv::Mat paddedPatch = globalAligned(paddedRoi).clone();
            cv::resize(paddedPatch, paddedPatch,
                {static_cast<int>(paddedRoi.width * _config.upsample), static_cast<int>(paddedRoi.height * _config.upsample)},
                cv::INTER_LANCZOS4
            );
            // Apply the local shift
            cv::warpAffine(paddedPatch, paddedPatch, localM, paddedPatch.size(), cv::INTER_LANCZOS4, cv::BORDER_REPLICATE);

            // Extract the aligned inner patch
            cv::Rect roiInPadded(
                padX[0] * _config.upsample,
                padY[0] * _config.upsample,
                (paddedRoi.width - padX[0] - padX[1]) * _config.upsample,
                (paddedRoi.height - padY[0] - padY[1]) * _config.upsample
            );

            cv::Mat patchInPadded = paddedPatch(roiInPadded).clone();

            // Scale the original ROI to match upsampled space
            roi.x *= _config.upsample;
            roi.y *= _config.upsample;
            roi.width *= _config.upsample;
            roi.height *= _config.upsample;

            std::lock_guard<std::mutex> lock(_mtx);
            {
                _localAccumulator(roi) += patchInPadded * weight;
                _localWeights(roi) += weight;
            }
        }
    }
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

void Stacker::_reset() {
    _reference.release();
    _globalAccumulator.release();
    _globalWeights.release();
    _localAccumulator.release();
    _localWeights.release();
    _config = _StackConfig();
}
