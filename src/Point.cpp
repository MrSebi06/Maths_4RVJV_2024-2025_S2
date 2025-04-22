#include "../include/Point.h"

Point::Point(float x, float y) : x(x), y(y) {}

Point Point::operator+(const Point& other) const {
    return Point(x + other.x, y + other.y);
}

Point Point::operator-(const Point& other) const {
    return Point(x - other.x, y - other.y);
}

Point Point::operator*(float scalar) const {
    return Point(x * scalar, y * scalar);
}

float Point::distanceTo(const Point& other) const {
    float dx = x - other.x;
    float dy = y - other.y;
    return std::sqrt(dx * dx + dy * dy);
}

float Point::cross(const Point& other) const {
    return x * other.y - y * other.x;
}