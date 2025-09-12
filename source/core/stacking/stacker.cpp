#include "stacker.h"
#include "components/frame.h"

void Stacker::initialize(const cv::Mat &reference, const StackConfig &config) {
    reset_();

    reference_ = reference;
    reference_ = Frame::centerObject(reference, reference.cols, reference.rows);

    config_ = config;

    // Initialize internal accumulators
    cv::Size upsampledSize{
        static_cast<int>(reference_.cols * config_.upsample),
        static_cast<int>(reference_.rows * config_.upsample)
    };
    localAccumulator_ = cv::Mat::zeros(upsampledSize, CV_32FC3);
    localWeights_ = cv::Mat::zeros(upsampledSize, CV_32F);
    globalAccumulator_ = localAccumulator_.clone();
    globalWeights_ = localWeights_.clone();

    cv::Mat upsampledReference = reference;
    if (config.upsample != 1.0) {
        cv::resize(reference, upsampledReference, upsampledSize, cv::INTER_LANCZOS4);
    }

    // Include reference frame
    globalAccumulator_ += upsampledReference * 1.0f;
    globalWeights_ += 1.0f;
}

void Stacker::add(cv::Mat mat, double weight) {
    // Compute global shift relative to reference
    cv::Point2f globalShift = computeShift(reference_, mat, 0.35);
    cv::Mat globalM = (cv::Mat_<double>(2, 3) <<
        1, 0, -globalShift.x,
        0, 1, -globalShift.y
    );

    cv::Size upsampledSize = globalAccumulator_.size();

    // Apply global alignment
    cv::Mat globalAligned;
    cv::warpAffine(mat, globalAligned, globalM, reference_.size(), cv::INTER_LANCZOS4);
    globalAligned.convertTo(globalAligned, CV_32FC3);

    // Upsample the globally aligned frame
    cv::Mat globalAlignedUpsampled;
    cv::resize(globalAligned, globalAlignedUpsampled, upsampledSize, cv::INTER_LANCZOS4);

    {
        std::lock_guard<std::mutex> lock(mtx_);
        globalAccumulator_ += globalAlignedUpsampled * weight;
        globalWeights_ += weight;
    }

    // Optional: Local alignment using alignment points (APs)
    if (config_.aps) {
        // Padding to exclude borders interpolation (e.g. BORDER_REPLICATE)
        constexpr int padding = 5;

        for (const auto &ap : *config_.aps) {
            // Original AP's ROI
            cv::Rect roi = ap.rect(), paddedRoi = roi;
            roi &= cv::Rect(0, 0, reference_.cols, reference_.rows);

            // Padded ROI
            paddedRoi = {
                paddedRoi.x - padding,
                paddedRoi.y - padding,
                paddedRoi.width + 2 * padding,
                paddedRoi.height + 2 * padding
            };
            paddedRoi &= cv::Rect(0, 0, reference_.cols, reference_.rows);

            // padX(Y)[0] is applied padding to the left (top), [1] is to the right (bottom)
            int padX[2] {roi.x - paddedRoi.x, paddedRoi.x + paddedRoi.width - roi.x - roi.width};
            int padY[2] {roi.y - paddedRoi.y, paddedRoi.y + paddedRoi.height - roi.y - roi.height};

            // Local shift between the ROI in reference and globally aligned frame
            cv::Point2f localShift = computeShift(reference_(roi), globalAligned(roi));
            cv::Mat localM = (cv::Mat_<double>(2, 3) <<
                1, 0, -localShift.x * config_.upsample,
                0, 1, -localShift.y * config_.upsample
            );

            // Extract and upsample padded patch
            cv::Mat paddedPatch = globalAligned(paddedRoi).clone();
            cv::resize(paddedPatch, paddedPatch,
                {static_cast<int>(paddedRoi.width * config_.upsample), static_cast<int>(paddedRoi.height * config_.upsample)},
                cv::INTER_LANCZOS4
            );
            // Apply the local shift
            cv::warpAffine(paddedPatch, paddedPatch, localM, paddedPatch.size(), cv::INTER_LANCZOS4, cv::BORDER_REPLICATE);

            // Extract the aligned inner patch
            cv::Rect roiInPadded(
                padX[0] * config_.upsample,
                padY[0] * config_.upsample,
                (paddedRoi.width - padX[0] - padX[1]) * config_.upsample,
                (paddedRoi.height - padY[0] - padY[1]) * config_.upsample
            );

            cv::Mat patchInPadded = paddedPatch(roiInPadded).clone();

            // Scale the original ROI to match upsampled space
            roi.x *= config_.upsample;
            roi.y *= config_.upsample;
            roi.width *= config_.upsample;
            roi.height *= config_.upsample;

            std::lock_guard<std::mutex> lock(mtx_);
            {
                localAccumulator_(roi) += patchInPadded * weight;
                localWeights_(roi) += weight;
            }
        }
    }
}

cv::Mat Stacker::average() {
    // Final result mat
    cv::Mat result(globalAccumulator_.size(), CV_32FC3);

    // Combine local and global accumulations
    if (config_.aps) {
        for (int y = 0; y < result.rows; ++y) {
            for (int x = 0; x < result.cols; ++x) {
                float localW = localWeights_.at<float>(y, x);
                if (localW > 0.0f) {
                    result.at<cv::Vec3f>(y, x) = localAccumulator_.at<cv::Vec3f>(y, x) / localW;
                }
                else {
                    float globalW = globalWeights_.at<float>(y, x);
                    result.at<cv::Vec3f>(y, x) = globalAccumulator_.at<cv::Vec3f>(y, x) / globalW;
                }
            }
        }
    }
    // Normalize global accumulation only
    else {
        cv::Mat weights3;
        cv::Mat weightsChannels[] {globalWeights_, globalWeights_, globalWeights_};
        cv::merge(weightsChannels, 3, weights3);
        cv::divide(globalAccumulator_, weights3, result);
    }

    // Expand borders (padding with black)
    cv::Size expandSize{
        static_cast<int>(config_.outputWidth * config_.upsample),
        static_cast<int>(config_.outputHeight * config_.upsample)
    };
    result = Frame::expandBorders(result, expandSize.width, expandSize.height);

    return result;
}

void Stacker::reset_() {
    reference_.release();
    globalAccumulator_.release();
    globalWeights_.release();
    localAccumulator_.release();
    localWeights_.release();
    config_ = StackConfig();
}
