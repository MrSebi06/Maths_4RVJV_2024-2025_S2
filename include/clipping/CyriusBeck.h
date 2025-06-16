#pragma once
#include <vector>
#include "../commons/Point.h"

class CyrusBeck {
public:
    // Vérifie si un polygone est convexe
    static bool isPolygonConvex(const std::vector<Point>& polygon);
    
    // Découpe une courbe (représentée comme une série de points) selon une fenêtre de découpage
    static std::vector<std::vector<Point>> clipCurveToWindow(const std::vector<Point>& curve, const std::vector<Point>& clipWindow);
    
private:
    // Calcule le produit vectoriel 2D
    static float crossProduct(const Point& a, const Point& b);
    
    // Calcule le produit scalaire 2D
    static float dotProduct(const Point& a, const Point& b);
    
    // Vérifie si un segment de ligne (p1, p2) intersecte une arête de la fenêtre (e1, e2)
    static bool clipSegmentToEdge(const Point& p1, const Point& p2, const Point& e1, const Point& e2,
                                 float& tE, float& tL, Point& normal);
};