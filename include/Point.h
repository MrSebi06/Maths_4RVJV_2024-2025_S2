#ifndef POINT_H
#define POINT_H

#include <cmath>

class Point {
public:
    float x, y;

    Point(float x = 0.0f, float y = 0.0f);

    // Opérateurs pour faciliter les calculs
    Point operator+(const Point& other) const;
    Point operator-(const Point& other) const;
    Point operator*(float scalar) const;

    // Calcul de distance
    float distanceTo(const Point& other) const;

    // Produit vectoriel 2D (utile pour l'orientation dans l'algorithme d'enveloppe convexe)
    float cross(const Point& other) const;
};

#endif // POINT_H