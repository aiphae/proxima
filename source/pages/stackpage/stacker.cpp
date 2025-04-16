#include "stacker.h"
#include "helpers.h"
#include "frame.h"
#include <opencv2/imgproc.hpp>
#include <QDebug>

cv::Mat Stacker::stack() {
    return aps.empty() ? stackGlobal() : stackLocal();
}

cv::Mat Stacker::stackGlobal() {
    cv::Mat accumulator = cv::Mat::zeros(reference.size(), CV_32FC3);
    int stacked = 0;

    cv::Mat referenceGray;
    cv::cvtColor(reference, referenceGray, cv::COLOR_BGR2GRAY);
    referenceGray.convertTo(referenceGray, CV_32F);

    for (int i = 1; i < framesToStack; ++i) { // Starting from 1 because 0 is the reference frame
        cv::Mat current = getMatAtFrame(files, sorted[i].first);
        if (current.empty()) {
            continue;
        }

        cv::Mat currentGray;
        cv::cvtColor(current, currentGray, cv::COLOR_BGR2GRAY);
        currentGray.convertTo(currentGray, CV_32F);

        cv::Point2f shift = Frame::correlate(referenceGray, currentGray);
        cv::Mat M = (cv::Mat_<double>(2, 3) <<
            1, 0, -shift.x,
            0, 1, -shift.y
        );

        cv::Mat aligned;
        cv::warpAffine(current, aligned, M, accumulator.size(), cv::INTER_LANCZOS4, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
        aligned.convertTo(aligned, CV_32FC3);

        accumulator += aligned;
        ++stacked;
    }

    if (stacked == 0) {
        return {};
    }

    cv::Mat result;
    accumulator /= stacked;
    accumulator.convertTo(result, CV_8UC3);

    return result;
}

cv::Mat Stacker::stackLocal() {
    cv::Mat accumulator = cv::Mat::zeros(reference.size(), CV_32FC3);
    cv::Mat weights = cv::Mat::zeros(reference.size(), CV_32F);

    // Reference grayscale
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

        // First, align the frame to the reference
        cv::Point2f globalShift = Frame::correlate(referenceGray, currentGray);
        cv::Mat globalM = (cv::Mat_<double>(2, 3) <<
            1, 0, -globalShift.x,
            0, 1, -globalShift.y
        );

        // Warp both color and gray current frames
        cv::Mat globalAligned, globalAlignedGray;
        cv::warpAffine(current, globalAligned, globalM, current.size(), cv::INTER_LANCZOS4);
        cv::warpAffine(currentGray, globalAlignedGray, globalM, current.size(), cv::INTER_LANCZOS4);

        for (const auto &ap : aps) {
            cv::Rect roi = ap.rect() & cv::Rect(0, 0, accumulator.cols, accumulator.rows);
            if (roi.width <= 1 || roi.height <= 1)
                continue;

            // Then, align the corresponding patch
            cv::Point2f localShift = Frame::correlate(referenceGray(roi), globalAligned(roi));
            cv::Mat localM = (cv::Mat_<double>(2, 3) <<
                1, 0, -localShift.x,
                0, 1, -localShift.y
            );

            // Warp the image according to the local shift
            cv::Mat localAligned;
            cv::warpAffine(globalAligned, localAligned, localM, accumulator.size(), cv::INTER_LANCZOS4);
            localAligned.convertTo(localAligned, CV_32FC3);

            // Accumulate the results
            accumulator += localAligned;
            weights += 1.0f;
        }
    }

    // Compute the globally stacked image in case of fallback
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
