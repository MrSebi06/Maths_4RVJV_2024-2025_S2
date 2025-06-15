#pragma once
#include <vector>
#include "Point.h"

class SutherlandHodgman {
public:
    // Découpe un polygone en utilisant un autre polygone comme fenêtre de découpage
    static std::vector<Point> clipPolygon(const std::vector<Point>& subjectPolygon, const std::vector<Point>& clipPolygon);

    // Fonction optionnelle pour adapter l'algorithme aux courbes ouvertes
    static std::vector<std::vector<Point>> clipCurve(const std::vector<Point>& curve, const std::vector<Point>& clipPolygon);

private:
    // Vérifie si un point est à l'intérieur d'une arête
    static bool isInside(const Point& p, const Point& p1, const Point& p2);

    // Calcule l'intersection entre un segment et une arête
    static Point computeIntersection(const Point& s, const Point& e, const Point& p1, const Point& p2);
};