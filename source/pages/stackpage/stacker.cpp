#include "stacker.h"
#include "helpers.h"
#include "frame.h"
#include <opencv2/imgproc.hpp>

cv::Mat Stacker::stack(const StackingSource &source, const StackingConfiguration &config) {
    cv::Mat result;
    if (aps.empty()) {
        result = stackGlobal(source, config);
    }

    return result;
}

cv::Mat Stacker::stackGlobal(const StackingSource &source, const StackingConfiguration &config) {
    cv::Mat refGray;
    cv::cvtColor(source.reference, refGray, cv::COLOR_BGR2GRAY);
    refGray.convertTo(refGray, CV_32F);

    cv::Mat accumulator = cv::Mat::zeros(source.reference.size(), CV_32FC3);
    int stacked = 0;

    for (int i = 0; i < config.frames; ++i) {
        cv::Mat frame = getMatAtFrame(source.files, source.sorted[i].first);
        frame = Frame(frame).cropOnObject(config.width, config.height);

        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        gray.convertTo(gray, CV_32F);

        cv::Point2d shift = cv::phaseCorrelate(refGray, gray);

        cv::Mat translationMatrix = (cv::Mat_<double>(2, 3) <<
            1, 0, -shift.x,
            0, 1, -shift.y
        );

        cv::Mat aligned;
        cv::warpAffine(frame, aligned, translationMatrix, source.reference.size(),
                       cv::INTER_LANCZOS4, cv::BORDER_REPLICATE);

        aligned.convertTo(aligned, CV_32FC3);
        accumulator += aligned;
        ++stacked;
    }

    if (stacked == 0) {
        return cv::Mat();
    }

    accumulator /= stacked;
    cv::Mat finalStack;
    accumulator.convertTo(finalStack, CV_8UC3);

    return finalStack;
}
