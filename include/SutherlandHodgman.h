#ifndef SUTHERLAND_HODGMAN_H
#define SUTHERLAND_HODGMAN_H

#include <vector>
#include "Point.h"

class SutherlandHodgman {
public:
    // Découpe un polygone par rapport à une fenêtre convexe
    static std::vector<Point> clipPolygon(const std::vector<Point>& inputPolygon, 
                                         const std::vector<Point>& clipWindow);
    
private:
    // Découpe un polygone par rapport à une arête de la fenêtre
    static std::vector<Point> clipPolygonAgainstEdge(const std::vector<Point>& inputPolygon,
                                                    const Point& edgeStart,
                                                    const Point& edgeEnd);
    
    // Détermine si un point est du côté intérieur d'une arête
    static bool isInside(const Point& p, const Point& edgeStart, const Point& edgeEnd);
    
    // Calcule le point d'intersection entre une ligne et une arête
    static Point computeIntersection(const Point& s, const Point& e,
                                    const Point& edgeStart, const Point& edgeEnd);
};

#endif // SUTHERLAND_HODGMAN_H