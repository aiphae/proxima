#include "stacker.h"
#include "helpers.h"
#include "frame.h"
#include <opencv2/imgproc.hpp>

cv::Mat Stacker::stack(const StackingSource &source, const StackingConfiguration &config) {
    return aps.empty() ? stackGlobal(source, config) : cv::Mat();
}

cv::Mat Stacker::stackGlobal(const StackingSource &source, const StackingConfiguration &config) {
    cv::Mat refGray;
    cv::cvtColor(source.reference, refGray, cv::COLOR_BGR2GRAY);
    refGray.convertTo(refGray, CV_32F);

    cv::Mat accumulator = cv::Mat::zeros(source.reference.size(), CV_32FC3);
    int stacked = 0;

    for (int i = 0; i < config.frames; ++i) {
        cv::Mat frame = getMatAtFrame(source.files, source.sorted[i].first);
        if (frame.empty()) {
            continue;
        }

        frame = Frame::cropOnObject(frame, config.width, config.height);

        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        gray.convertTo(gray, CV_32F);

        const cv::Point2d shift = cv::phaseCorrelate(refGray, gray);
        const cv::Mat transform = (cv::Mat_<double>(2, 3) <<
            1, 0, -shift.x,
            0, 1, -shift.y
        );

        cv::Mat aligned;
        cv::warpAffine(frame, aligned, transform, source.reference.size(), cv::INTER_LANCZOS4, cv::BORDER_REPLICATE);

        aligned.convertTo(aligned, CV_32FC3);
        accumulator += aligned;
        ++stacked;
    }

    if (stacked == 0) {
        return {};
    }

    accumulator /= stacked;
    cv::Mat result;
    accumulator.convertTo(result, CV_8UC3);

    return result;
}
