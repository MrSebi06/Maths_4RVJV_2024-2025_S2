#include "../include/Point.h"

Point::Point(float x, float y) : x(x), y(y) {}

Point Point::operator+(const Point& p) const {
    return Point(x + p.x, y + p.y);
}

Point Point::operator*(float scalar) const {
    return Point(x * scalar, y * scalar);
}