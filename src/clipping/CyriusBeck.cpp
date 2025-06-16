#include "../../include/clipping/CyriusBeck.h"
#include <cmath>
#include <algorithm>

bool CyrusBeck::isPolygonConvex(const std::vector<Point>& polygon) {
    // Un polygone avec moins de 3 points n'est pas considéré comme convexe
    if (polygon.size() < 3) {
        return false;
    }

    // Pour chaque triplet de points consécutifs
    bool sign = false;
    bool signSet = false;

    for (size_t i = 0; i < polygon.size(); i++) {
        size_t j = (i + 1) % polygon.size();
        size_t k = (i + 2) % polygon.size();

        // Vecteurs
        Point v1(polygon[j].x - polygon[i].x, polygon[j].y - polygon[i].y);
        Point v2(polygon[k].x - polygon[j].x, polygon[k].y - polygon[j].y);

        // Produit vectoriel
        float cross = crossProduct(v1, v2);

        // Déterminer le signe du premier produit vectoriel non nul
        if (!signSet && cross != 0) {
            sign = (cross > 0);
            signSet = true;
        }

        // Si le signe change, le polygone n'est pas convexe
        if (signSet && ((cross > 0) != sign) && cross != 0) {
            return false;
        }
    }

    return true;
}

float CyrusBeck::crossProduct(const Point& a, const Point& b) {
    return a.x * b.y - a.y * b.x;
}

float CyrusBeck::dotProduct(const Point& a, const Point& b) {
    return a.x * b.x + a.y * b.y;
}

bool CyrusBeck::clipSegmentToEdge(const Point& p1, const Point& p2, const Point& e1, const Point& e2,
                                 float& tE, float& tL, Point& normal) {
    // Vecteur du segment
    Point d(p2.x - p1.x, p2.y - p1.y);

    // Vecteur normal à l'arête (e1, e2)
    normal.x = e2.y - e1.y;
    normal.y = -(e2.x - e1.x);

    // Normaliser le vecteur normal
    float length = std::sqrt(normal.x * normal.x + normal.y * normal.y);
    if (length > 0) {
        normal.x /= length;
        normal.y /= length;
    }

    // Produit scalaire entre le vecteur du segment et le vecteur normal
    float numerator = dotProduct(normal, Point(e1.x - p1.x, e1.y - p1.y));
    float denominator = dotProduct(normal, d);

    // Segment parallèle à l'arête
    if (std::abs(denominator) < 1e-6) {
        // Si le segment est à l'extérieur de l'arête, il est rejeté
        if (numerator < 0) {
            return false;
        }
        // Sinon, le segment est parallèle et à l'intérieur, pas de découpage
        return true;
    }

    // Calculer le paramètre t d'intersection
    float t = numerator / denominator;

    // Mettre à jour tE et tL en fonction du signe de denominator
    if (denominator < 0) {
        tE = std::max(tE, t);
    } else {
        tL = std::min(tL, t);
    }

    // Si tE > tL, le segment est complètement à l'extérieur
    return tE <= tL;
}

std::vector<std::vector<Point>> CyrusBeck::clipCurveToWindow(const std::vector<Point>& curve, const std::vector<Point>& clipWindow) {
    std::vector<std::vector<Point>> clippedSegments;

    // Vérifier si la courbe a au moins 2 points
    if (curve.size() < 2 || clipWindow.size() < 3) {
        return clippedSegments;
    }

    // Traiter chaque segment de la courbe
    for (size_t i = 0; i < curve.size() - 1; i++) {
        Point p1 = curve[i];
        Point p2 = curve[i + 1];

        // Paramètres pour l'algorithme de Cyrus-Beck
        float tE = 0.0f;  // Paramètre d'entrée
        float tL = 1.0f;  // Paramètre de sortie

        bool visible = true;

        // Vérifier chaque arête de la fenêtre de découpage
        for (size_t j = 0; j < clipWindow.size(); j++) {
            size_t nextJ = (j + 1) % clipWindow.size();
            Point e1 = clipWindow[j];
            Point e2 = clipWindow[nextJ];

            Point normal;

            // Découper le segment par rapport à cette arête
            if (!clipSegmentToEdge(p1, p2, e1, e2, tE, tL, normal)) {
                visible = false;
                break;
            }
        }

        // Si le segment est visible (au moins partiellement)
        if (visible && tE <= tL) {
            // Calculer les points découpés
            Point clippedP1(p1.x + tE * (p2.x - p1.x), p1.y + tE * (p2.y - p1.y));
            Point clippedP2(p1.x + tL * (p2.x - p1.x), p1.y + tL * (p2.y - p1.y));

            // Ajouter le segment découpé
            std::vector<Point> segment;
            segment.push_back(clippedP1);
            segment.push_back(clippedP2);
            clippedSegments.push_back(segment);
        }
    }

    return clippedSegments;
}