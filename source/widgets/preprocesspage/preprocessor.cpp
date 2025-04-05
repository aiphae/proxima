#include "preprocessor.h"
#include "frame.h"

cv::Mat Preprocessor::preview(cv::Mat &frame, PreprocessingConfig &config) {
    Frame processed(frame);

    if (config.toMonochrome) {
        processed.convertColor(cv::COLOR_BGR2GRAY);
        processed.convertColor(cv::COLOR_GRAY2BGR);
    }

    cv::Rect object;
    if (config.crop || config.rejectFrames) {
        object = processed.findObject(config.minObjectSize);
    }

    if (config.crop) {
        cv::Rect crop = processed.getObjectCrop(object, config.cropWidth, config.cropHeight);
        if (object.x && object.y) {
            cv::rectangle(processed.mat(), crop, cv::Scalar(0, 255, 0));

            cv::Point cropTextPos(crop.x, crop.y - 5);
            cv::putText(processed.mat(), "Object", cropTextPos, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0));

            cv::Point cropCenter(crop.x + crop.width / 2, crop.y + crop.height / 2);
            cv::drawMarker(processed.mat(), cropCenter, cv::Scalar(0, 255, 0), cv::MARKER_CROSS, 10, 1);
        }
    }

    if (config.rejectFrames) {
        if (object.x && object.y) {
            cv::rectangle(processed.mat(), object, cv::Scalar(255, 0, 0));

            cv::Point cropTextPos(object.x, object.y - 5);
            cv::putText(processed.mat(), "Object", cropTextPos, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0));

            cv::Point cropCenter(object.x + object.width / 2, object.y + object.height / 2);
            cv::drawMarker(processed.mat(), cropCenter, cv::Scalar(255, 0, 0), cv::MARKER_CROSS, 10, 1);
        }
    }

    return processed.mat();
}

cv::Mat Preprocessor::process(cv::Mat &frame, PreprocessingConfig &config) {
    Frame processed(frame);

    cv::Rect object;
    if (config.crop || config.rejectFrames) {
        object = processed.findObject(config.minObjectSize);
    }

    if (config.rejectFrames && object.width < 1) {
        return cv::Mat();
    }

    if (config.crop) {
        processed.mat() = processed.crop(processed.getObjectCrop(object, config.cropWidth, config.cropHeight));
    }

    if (config.toMonochrome) {
        processed.convertColor(cv::COLOR_BGR2GRAY);
        processed.convertColor(cv::COLOR_GRAY2BGR);
    }

    return processed.mat();
}
