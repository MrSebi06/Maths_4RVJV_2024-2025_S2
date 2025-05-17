#ifndef CYRUS_BECK_H
#define CYRUS_BECK_H

#include <vector>
#include "../include/Point.h"

class CyrusBeck {
public:
    // Fonction utilitaire pour calculer le produit scalaire de deux vecteurs
    static float dotProduct(const Point& v1, const Point& v2);

    // Fonction utilitaire pour calculer la normale extérieure d'une arête du polygone de découpage
    static Point calculateNormal(const Point& p1, const Point& p2);

    // Vérifier si un point est à l'intérieur de la fenêtre de découpage
    static bool isPointInsideWindow(const Point& p, const std::vector<Point>& clipWindow);

    // Algorithme de Cyrus-Beck pour découper un segment contre un polygone convexe
    static std::vector<Point> clipSegmentToWindow(const Point& p1, const Point& p2,
                                                const std::vector<Point>& clipWindow);

    // Découper une courbe (série de segments) contre un polygone convexe
    static std::vector<std::vector<Point>> clipCurveToWindow(const std::vector<Point>& curvePoints,
                                                            const std::vector<Point>& clipWindow);

    // Vérifier si un polygone est convexe
    static bool isPolygonConvex(const std::vector<Point>& polygon);
};

#endif // CYRUS_BECK_H