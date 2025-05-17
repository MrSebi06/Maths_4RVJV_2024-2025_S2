#include "../include/CyriusBeck.h"
#include <cmath>
#include <algorithm>

// Fonction utilitaire pour calculer le produit scalaire de deux vecteurs
float CyrusBeck::dotProduct(const Point& v1, const Point& v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

// Fonction utilitaire pour calculer la normale extérieure d'une arête du polygone de découpage
Point CyrusBeck::calculateNormal(const Point& p1, const Point& p2) {
    // Vecteur de l'arête
    Point edge(p2.x - p1.x, p2.y - p1.y);
    // Normale (perpendiculaire)
    Point normal(-edge.y, edge.x);
    // Normaliser
    float length = sqrt(normal.x * normal.x + normal.y * normal.y);
    if (length > 0.0001f) {
        normal.x /= length;
        normal.y /= length;
    }
    return normal;
}

// Vérifier si un point est à l'intérieur de la fenêtre de découpage
bool CyrusBeck::isPointInsideWindow(const Point& p, const std::vector<Point>& clipWindow) {
    if (clipWindow.size() < 3) return false;

    for (size_t i = 0; i < clipWindow.size(); i++) {
        size_t next = (i + 1) % clipWindow.size();
        
        // Vecteur du point au sommet du polygone
        Point e(p.x - clipWindow[i].x, p.y - clipWindow[i].y);
        
        // Normale extérieure
        Point n = calculateNormal(clipWindow[i], clipWindow[next]);
        
        // Produit scalaire pour déterminer de quel côté de l'arête se trouve le point
        if (dotProduct(e, n) > 0) {
            return false; // Le point est à l'extérieur de cette arête
        }
    }
    
    return true; // Le point est à l'intérieur de toutes les arêtes
}

// Algorithme de Cyrus-Beck pour découper un segment contre un polygone convexe
std::vector<Point> CyrusBeck::clipSegmentToWindow(const Point& p1, const Point& p2, 
                                               const std::vector<Point>& clipWindow) {
    std::vector<Point> result;
    
    if (clipWindow.size() < 3) {
        // Fenêtre invalide, retourner le segment d'origine
        result.push_back(p1);
        result.push_back(p2);
        return result;
    }
    
    // Vecteur de direction du segment
    Point d(p2.x - p1.x, p2.y - p1.y);
    
    float tE = 0.0f;  // Paramètre d'entrée (0 par défaut)
    float tL = 1.0f;  // Paramètre de sortie (1 par défaut)
    
    for (size_t i = 0; i < clipWindow.size(); i++) {
        size_t next = (i + 1) % clipWindow.size();
        
        // Normale extérieure de l'arête
        Point n = calculateNormal(clipWindow[i], clipWindow[next]);
        
        // Vecteur du premier point au sommet de l'arête
        Point w(p1.x - clipWindow[i].x, p1.y - clipWindow[i].y);
        
        float dn = dotProduct(d, n);
        float wn = dotProduct(w, n);
        
        if (fabs(dn) < 0.0001f) {
            // Le segment est parallèle à l'arête
            if (wn > 0) {
                // Le segment est entièrement à l'extérieur
                return result;  // Retourner vide
            }
            // Sinon, continuer avec la prochaine arête
        } else {
            // Calculer le paramètre t pour l'intersection
            float t = -wn / dn;
            
            if (dn < 0) {
                // Le segment entre dans le polygone
                tE = std::max(tE, t);
            } else {
                // Le segment sort du polygone
                tL = std::min(tL, t);
            }
            
            if (tE > tL) {
                // Le segment est complètement à l'extérieur
                return result;  // Retourner vide
            }
        }
    }
    
    // Calculer les points d'intersection
    if (tE > 0 || tL < 1) {
        // Le segment a été découpé, calculer les nouveaux points
        Point pE(p1.x + tE * d.x, p1.y + tE * d.y);
        Point pL(p1.x + tL * d.x, p1.y + tL * d.y);
        
        result.push_back(pE);
        if (tE < tL) {
            result.push_back(pL);
        }
    } else {
        // Le segment est entièrement à l'intérieur
        result.push_back(p1);
        result.push_back(p2);
    }

    return result;
}

// Découper une courbe (série de segments) contre un polygone convexe
std::vector<std::vector<Point>> CyrusBeck::clipCurveToWindow(const std::vector<Point>& curvePoints,
                                                           const std::vector<Point>& clipWindow) {
    std::vector<std::vector<Point>> clippedCurveSegments;

    if (curvePoints.size() < 2 || clipWindow.size() < 3) {
        return clippedCurveSegments;
    }

    // Variable pour stocker le segment courant
    std::vector<Point> currentSegment;

    // Pour chaque segment de la courbe
    for (size_t i = 0; i < curvePoints.size() - 1; i++) {
        const Point& p1 = curvePoints[i];
        const Point& p2 = curvePoints[i + 1];

        // Découper le segment
        std::vector<Point> clippedSegment = clipSegmentToWindow(p1, p2, clipWindow);

        if (!clippedSegment.empty()) {
            // Si le segment est à l'intérieur et que nous avons un segment en cours
            if (!currentSegment.empty() &&
                currentSegment.back().x == clippedSegment.front().x &&
                currentSegment.back().y == clippedSegment.front().y) {
                // Ajouter seulement le deuxième point pour continuer le segment
                currentSegment.push_back(clippedSegment.back());
            } else {
                // Si c'est un nouveau segment
                if (!currentSegment.empty()) {
                    // Sauvegarder le segment courant
                    clippedCurveSegments.push_back(currentSegment);
                    currentSegment.clear();
                }
                // Commencer un nouveau segment
                currentSegment = clippedSegment;
            }
        } else if (!currentSegment.empty()) {
            // Si le segment est à l'extérieur et que nous avons un segment en cours
            clippedCurveSegments.push_back(currentSegment);
            currentSegment.clear();
        }
    }

    // Ajouter le dernier segment s'il existe
    if (!currentSegment.empty()) {
        clippedCurveSegments.push_back(currentSegment);
    }

    return clippedCurveSegments;
}

// Vérifier si un polygone est convexe
bool CyrusBeck::isPolygonConvex(const std::vector<Point>& polygon) {
    if (polygon.size() < 3) {
        return false;  // Pas assez de points pour former un polygone
    }

    // Vérifier l'orientation de chaque triplet de points consécutifs
    int orientation = 0;
    bool firstCheck = true;

    for (size_t i = 0; i < polygon.size(); i++) {
        size_t j = (i + 1) % polygon.size();
        size_t k = (i + 2) % polygon.size();

        // Calcul du produit vectoriel pour déterminer l'orientation
        float cross = (polygon[j].x - polygon[i].x) * (polygon[k].y - polygon[j].y) -
                      (polygon[j].y - polygon[i].y) * (polygon[k].x - polygon[j].x);

        int currentOrientation = (cross > 0) ? 1 : ((cross < 0) ? -1 : 0);

        if (currentOrientation != 0) {
            if (firstCheck) {
                orientation = currentOrientation;
                firstCheck = false;
            } else if (orientation != currentOrientation) {
                return false;  // Changement d'orientation, pas convexe
            }
        }
    }

    return true;
}