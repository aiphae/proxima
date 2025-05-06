#ifndef ALIGNMENT_H
#define ALIGNMENT_H

#include <opencv2/opencv.hpp>

// Single alignment poit
class AlignmentPoint {
public:
    AlignmentPoint(int x, int y, int size) {
        roi = {x - size / 2, y - size / 2, size, size};
    }
    cv::Rect rect() const { return roi; }

private:
    cv::Rect roi;
};

// Set of alignment points
class AlignmentPointSet {
public:
    enum class Placement {
        FeatureBased,
        Uniform
    };

    struct Config {
        Placement placement;
        int size;
    };

    static AlignmentPointSet estimate(cv::Mat frame, Config config);
    bool empty() { return aps.empty(); }
    int size() { return aps.size(); }
    void clear() { aps.clear(); }

    // Range-based for loop support
    std::vector<AlignmentPoint>::iterator begin() { return aps.begin(); }
    std::vector<AlignmentPoint>::iterator end() { return aps.end(); }
    std::vector<AlignmentPoint>::const_iterator begin() const { return aps.cbegin(); }
    std::vector<AlignmentPoint>::const_iterator end() const { return aps.cend(); }

private:
    std::vector<AlignmentPoint> aps;
    void add(AlignmentPoint point) { aps.push_back(point); }
};

#endif // ALIGNMENT_H
