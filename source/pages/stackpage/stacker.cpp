#include "stacker.h"
#include <opencv2/imgproc.hpp>

cv::Mat Stacker::stack(std::vector<cv::Mat> &frames) {
    cv::Mat result;
    if (aps.empty()) {
        result = stackGlobal(frames);
    }

    return result;
}

cv::Mat Stacker::stackGlobal(std::vector<cv::Mat> &frames) {
    cv::Mat refGray;
    cv::cvtColor(reference, refGray, cv::COLOR_BGR2GRAY);
    refGray.convertTo(refGray, CV_32F);

    cv::Mat accumulator = cv::Mat::zeros(reference.size(), CV_32FC3);
    int stacked = 0;

    for (int i = 0; i < frames.size(); ++i) {
        cv::Mat frame = frames[i];

        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        gray.convertTo(gray, CV_32F);

        cv::Point2d shift = cv::phaseCorrelate(refGray, gray);

        cv::Mat translationMatrix = (cv::Mat_<double>(2, 3) <<
            1, 0, -shift.x,
            0, 1, -shift.y
        );

        cv::Mat aligned;
        cv::warpAffine(frame, aligned, translationMatrix, reference.size(),
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
