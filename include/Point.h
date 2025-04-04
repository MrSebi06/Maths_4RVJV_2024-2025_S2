#ifndef POINT_H
#define POINT_H

class Point {
public:
    float x, y;

    Point(float x = 0, float y = 0);

    Point operator+(const Point& p) const;
    Point operator*(float scalar) const;
};

#endif // POINT_H