
#include "../../include/clipping/SutherlandHodgman.h"

bool SutherlandHodgman::isInside(const Point& p, const Point& p1, const Point& p2) {
    return (p2.x - p1.x) * (p.y - p1.y) - (p2.y - p1.y) * (p.x - p1.x) >= 0;
}

Point SutherlandHodgman::computeIntersection(const Point& s, const Point& e, const Point& p1, const Point& p2) {
    float dc[2], dp[2];
    dc[0] = p1.x - p2.x;
    dc[1] = p1.y - p2.y;
    dp[0] = s.x - e.x;
    dp[1] = s.y - e.y;

    float n1 = p1.x * p2.y - p1.y * p2.x;
    float n2 = s.x * e.y - s.y * e.x;
    float n3 = 1.0f / (dc[0] * dp[1] - dc[1] * dp[0]);

    return Point((n1 * dp[0] - n2 * dc[0]) * n3, (n1 * dp[1] - n2 * dc[1]) * n3);
}

std::vector<Point> SutherlandHodgman::clipPolygon(const std::vector<Point>& subjectPolygon, const std::vector<Point>& clipPolygon) {
    std::vector<Point> outputList = subjectPolygon;

    // Pour chaque arête du polygone de découpage
    for (int i = 0; i < clipPolygon.size(); i++) {
        int nextI = (i + 1) % clipPolygon.size();
        const Point& clipEdgeStart = clipPolygon[i];
        const Point& clipEdgeEnd = clipPolygon[nextI];

        std::vector<Point> inputList = outputList;
        outputList.clear();

        if (inputList.empty()) {
            return outputList;
        }

        Point s = inputList.back();

        for (const Point& e : inputList) {
            // Si le point courant est à l'intérieur de l'arête de découpage
            if (isInside(e, clipEdgeStart, clipEdgeEnd)) {
                // Si le point précédent n'était pas à l'intérieur
                if (!isInside(s, clipEdgeStart, clipEdgeEnd)) {
                    // Ajouter le point d'intersection
                    outputList.push_back(computeIntersection(s, e, clipEdgeStart, clipEdgeEnd));
                }
                // Ajouter le point courant
                outputList.push_back(e);
            }
            // Si le point courant n'est pas à l'intérieur mais que le précédent l'était
            else if (isInside(s, clipEdgeStart, clipEdgeEnd)) {
                // Ajouter le point d'intersection
                outputList.push_back(computeIntersection(s, e, clipEdgeStart, clipEdgeEnd));
            }

            s = e;
        }
    }

    return outputList;
}

// dans le cas de courbes ouvertes et non polygones.
std::vector<std::vector<Point>> SutherlandHodgman::clipCurve(const std::vector<Point>& curve,
    const std::vector<Point>& clipPolygon) {
    std::vector<std::vector<Point>> clippedSegments;

    if (curve.size() < 2 || clipPolygon.size() < 3) {
        return clippedSegments;
    }

    // Traiter chaque segment de la courbe séparément
    for (size_t i = 0; i < curve.size() - 1; i++) {
        // Créer un segment de deux points
        std::vector<Point> segment = {curve[i], curve[i+1]};

        // Découper ce segment avec Sutherland-Hodgman
        //std::vector<Point> clippedSegment = clipPolygon(segment, clipPolygon);
        std::vector<Point> clippedSegment = SutherlandHodgman::clipPolygon(segment, clipPolygon);


        // Si le segment n'est pas entièrement découpé, l'ajouter aux résultats
        if (!clippedSegment.empty()) {
            clippedSegments.push_back(clippedSegment);
        }
    }

    return clippedSegments;
}