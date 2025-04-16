#include "alignmentpoint.h"

AlignmentPoint::AlignmentPoint(int x, int y, int size) {
    roi = {x - size / 2, y - size / 2, size, size};
}
