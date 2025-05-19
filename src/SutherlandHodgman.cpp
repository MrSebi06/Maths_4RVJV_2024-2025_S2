#include "../include/SutherlandHodgman.h"

std::vector<Point> SutherlandHodgman::clipPolygon(const std::vector<Point>& inputPolygon,
                                                 const std::vector<Point>& clipWindow) {
    if (inputPolygon.empty() || clipWindow.size() < 3) {
        return inputPolygon;  // Retourner le polygone d'entrée s'il est vide ou si la fenêtre est invalide
    }

    std::vector<Point> outputPolygon = inputPolygon;

    // Pour chaque arête de la fenêtre de découpage
    for (size_t i = 0; i < clipWindow.size(); i++) {
        size_t nextIndex = (i + 1) % clipWindow.size();
        const Point& edgeStart = clipWindow[i];
        const Point& edgeEnd = clipWindow[nextIndex];

        // Découper le polygone par rapport à cette arête
        outputPolygon = clipPolygonAgainstEdge(outputPolygon, edgeStart, edgeEnd);

        // Si après découpage, le polygone est vide, retourner vide
        if (outputPolygon.empty()) {
            return outputPolygon;
        }
    }

    return outputPolygon;
}

std::vector<Point> SutherlandHodgman::clipPolygonAgainstEdge(const std::vector<Point>& inputPolygon,
                                                            const Point& edgeStart,
                                                            const Point& edgeEnd) {
    std::vector<Point> outputPolygon;

    if (inputPolygon.empty()) {
        return outputPolygon;
    }

    // Dernier point du polygone d'entrée
    Point s = inputPolygon.back();
    bool sInside = isInside(s, edgeStart, edgeEnd);

    // Pour chaque arête du polygone d'entrée
    for (const Point& e : inputPolygon) {
        bool eInside = isInside(e, edgeStart, edgeEnd);

        // Cas 1: Arête traverse de l'extérieur vers l'intérieur
        if (!sInside && eInside) {
            // Ajouter le point d'intersection
            outputPolygon.push_back(computeIntersection(s, e, edgeStart, edgeEnd));
            // Ajouter le point d'arrivée
            outputPolygon.push_back(e);
        }
        // Cas 2: Arête reste à l'intérieur
        else if (sInside && eInside) {
            // Ajouter seulement le point d'arrivée
            outputPolygon.push_back(e);
        }
        // Cas 3: Arête traverse de l'intérieur vers l'extérieur
        else if (sInside && !eInside) {
            // Ajouter seulement le point d'intersection
            outputPolygon.push_back(computeIntersection(s, e, edgeStart, edgeEnd));
        }
        // Cas 4: Arête reste à l'extérieur - ne rien ajouter

        // Mettre à jour le point de départ pour la prochaine itération
        s = e;
        sInside = eInside;
    }

    return outputPolygon;
}

bool SutherlandHodgman::isInside(const Point& p, const Point& edgeStart, const Point& edgeEnd) {
    // Vecteur de l'arête
    float edgeX = edgeEnd.x - edgeStart.x;
    float edgeY = edgeEnd.y - edgeStart.y;

    // Vecteur du point par rapport au début de l'arête
    float pX = p.x - edgeStart.x;
    float pY = p.y - edgeStart.y;

    // Produit vectoriel (2D)
    float cross = edgeX * pY - edgeY * pX;

    // Si le produit vectoriel est négatif, le point est à l'intérieur (à droite de l'arête orientée)
    return cross <= 0;
}

Point SutherlandHodgman::computeIntersection(const Point& s, const Point& e,
                                            const Point& edgeStart, const Point& edgeEnd) {
    // Calcul de l'intersection entre la ligne (s,e) et l'arête (edgeStart,edgeEnd)

    // Vecteurs des lignes
    float dc_x = edgeStart.x - edgeEnd.x;
    float dc_y = edgeStart.y - edgeEnd.y;
    float dp_x = s.x - e.x;
    float dp_y = s.y - e.y;

    // Calcul des numérateurs et du dénominateur
    float n1 = edgeStart.x * edgeEnd.y - edgeStart.y * edgeEnd.x;
    float n2 = s.x * e.y - s.y * e.x;
    float n3 = 1.0f / (dc_x * dp_y - dc_y * dp_x);

    // Calcul des coordonnées du point d'intersection
    float x = (n1 * dp_x - n2 * dc_x) * n3;
    float y = (n1 * dp_y - n2 * dc_y) * n3;

    return Point(x, y);
}