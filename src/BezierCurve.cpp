#include "../include/BezierCurve.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <limits>

BezierCurve::BezierCurve() : step(0.01f), showDirectMethod(false), showDeCasteljau(false) {
    setupBuffers();
}

BezierCurve::~BezierCurve() {
    // Nettoyer les VAOs et VBOs
    glDeleteVertexArrays(1, &controlPolygonVAO);
    glDeleteBuffers(1, &controlPolygonVBO);

    glDeleteVertexArrays(1, &directMethodVAO);
    glDeleteBuffers(1, &directMethodVBO);

    glDeleteVertexArrays(1, &deCasteljauVAO);
    glDeleteBuffers(1, &deCasteljauVBO);

    glDeleteVertexArrays(1, &pointsVAO);
    glDeleteBuffers(1, &pointsVBO);
}

void BezierCurve::setupBuffers() {
    // Créer et configurer les VAOs et VBOs pour le polygone de contrôle
    glGenVertexArrays(1, &controlPolygonVAO);
    glGenBuffers(1, &controlPolygonVBO);

    // Créer et configurer les VAOs et VBOs pour la méthode directe
    glGenVertexArrays(1, &directMethodVAO);
    glGenBuffers(1, &directMethodVBO);

    // Créer et configurer les VAOs et VBOs pour la méthode de De Casteljau
    glGenVertexArrays(1, &deCasteljauVAO);
    glGenBuffers(1, &deCasteljauVBO);

    // Créer et configurer les VAOs et VBOs pour les points de contrôle
    glGenVertexArrays(1, &pointsVAO);
    glGenBuffers(1, &pointsVBO);
}

void BezierCurve::updateBuffers() {
    // Mettre à jour le VBO du polygone de contrôle
    if (!controlPoints.empty()) {
        glBindVertexArray(controlPolygonVAO);
        glBindBuffer(GL_ARRAY_BUFFER, controlPolygonVBO);
        glBufferData(GL_ARRAY_BUFFER, controlPoints.size() * sizeof(Point), controlPoints.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // Mettre à jour le VBO des points de contrôle (pour les afficher comme des points)
        glBindVertexArray(pointsVAO);
        glBindBuffer(GL_ARRAY_BUFFER, pointsVBO);
        glBufferData(GL_ARRAY_BUFFER, controlPoints.size() * sizeof(Point), controlPoints.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    // Mettre à jour le VBO de la méthode directe
    if (!directMethodPoints.empty()) {
        glBindVertexArray(directMethodVAO);
        glBindBuffer(GL_ARRAY_BUFFER, directMethodVBO);
        glBufferData(GL_ARRAY_BUFFER, directMethodPoints.size() * sizeof(Point), directMethodPoints.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    // Mettre à jour le VBO de la méthode de De Casteljau
    if (!deCasteljauPoints.empty()) {
        glBindVertexArray(deCasteljauVAO);
        glBindBuffer(GL_ARRAY_BUFFER, deCasteljauVBO);
        glBufferData(GL_ARRAY_BUFFER, deCasteljauPoints.size() * sizeof(Point), deCasteljauPoints.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}

void BezierCurve::generatePascalTriangle(int n) {
    pascalTriangle.resize(n + 1);
    for (int i = 0; i <= n; i++) {
        pascalTriangle[i].resize(i + 1);
        pascalTriangle[i][0] = pascalTriangle[i][i] = 1;
        for (int j = 1; j < i; j++) {
            pascalTriangle[i][j] = pascalTriangle[i - 1][j - 1] + pascalTriangle[i - 1][j];
        }
    }
}

int BezierCurve::binomialCoeff(int n, int k) {
    return pascalTriangle[n][k];
}

void BezierCurve::addControlPoint(float x, float y) {
    controlPoints.push_back(Point(x, y));

    // Régénérer le triangle de Pascal si nécessaire
    if (pascalTriangle.size() < controlPoints.size()) {
        generatePascalTriangle(controlPoints.size() - 1);
    }

    // Recalculer les courbes si elles sont affichées
    recalculateCurves();
    updateBuffers();
}

void BezierCurve::updateControlPoint(int index, float x, float y) {
    if (index >= 0 && index < controlPoints.size()) {
        controlPoints[index].x = x;
        controlPoints[index].y = y;

        // Recalculer les courbes
        recalculateCurves();
        updateBuffers();
    }
}

void BezierCurve::removeControlPoint(int index) {
    if (index >= 0 && index < controlPoints.size()) {
        controlPoints.erase(controlPoints.begin() + index);

        // Recalculer les courbes
        recalculateCurves();
        updateBuffers();
    }
}

Point BezierCurve::getControlPoint(int index) const {
    if (index >= 0 && index < controlPoints.size()) {
        return controlPoints[index];
    }
    return Point(0, 0); // Point par défaut si l'index est invalide
}

float BezierCurve::distanceToControlPoint(int index, float x, float y) const {
    if (index >= 0 && index < controlPoints.size()) {
        Point p(x, y);
        return controlPoints[index].distanceTo(p);
    }
    return std::numeric_limits<float>::max(); // Distance maximale si l'index est invalide
}

int BezierCurve::getNearestControlPoint(float x, float y) const {
    if (controlPoints.empty()) {
        return -1;
    }

    Point p(x, y);
    int nearestIndex = 0;
    float minDistance = controlPoints[0].distanceTo(p);

    for (int i = 1; i < controlPoints.size(); ++i) {
        float distance = controlPoints[i].distanceTo(p);
        if (distance < minDistance) {
            minDistance = distance;
            nearestIndex = i;
        }
    }

    // Seuil de sélection (ajustable)
    if (minDistance > 0.1f) {
        return -1; // Aucun point assez proche
    }

    return nearestIndex;
}

void BezierCurve::clearControlPoints() {
    controlPoints.clear();
    directMethodPoints.clear();
    deCasteljauPoints.clear();
    showDirectMethod = false;
    showDeCasteljau = false;
    updateBuffers();
}

void BezierCurve::increaseStep() {
    step = std::min(0.1f, step + 0.001f);
    std::cout << "Pas: " << step << std::endl;
    recalculateCurves();
}

void BezierCurve::decreaseStep() {
    step = std::max(0.001f, step - 0.001f);
    std::cout << "Pas: " << step << std::endl;
    recalculateCurves();
}

void BezierCurve::recalculateCurves() {
    if (showDirectMethod) {
        calculateDirectMethod();
    }
    if (showDeCasteljau) {
        calculateDeCasteljau();
    }
}

void BezierCurve::calculateDirectMethod() {
    auto start = std::chrono::high_resolution_clock::now();

    directMethodPoints.clear();
    int n = controlPoints.size() - 1;

    if (n < 1) return;

    for (float t = 0; t <= 1.0f; t += step) {
        Point p(0, 0);
        for (int i = 0; i <= n; i++) {
            float bernstein = binomialCoeff(n, i) * pow(t, i) * pow(1 - t, n - i);
            p = p + controlPoints[i] * bernstein;
        }
        directMethodPoints.push_back(p);
    }

    // Ajouter le dernier point (t = 1)
    directMethodPoints.push_back(controlPoints[n]);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Temps de calcul (méthode directe): " << duration.count() << " ms" << std::endl;

    showDirectMethod = true;
    updateBuffers();
}

void BezierCurve::calculateDeCasteljau() {
    auto start = std::chrono::high_resolution_clock::now();

    deCasteljauPoints.clear();
    int n = controlPoints.size() - 1;

    if (n < 1) return;

    for (float t = 0; t <= 1.0f; t += step) {
        // Copier les points de contrôle
        std::vector<Point> temp = controlPoints;

        // Algorithme de De Casteljau
        for (int r = 1; r <= n; r++) {
            for (int i = 0; i <= n - r; i++) {
                temp[i] = temp[i] * (1 - t) + temp[i + 1] * t;
            }
        }

        deCasteljauPoints.push_back(temp[0]);
    }

    // Ajouter le dernier point (t = 1)
    deCasteljauPoints.push_back(controlPoints[n]);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Temps de calcul (De Casteljau): " << duration.count() << " ms" << std::endl;

    showDeCasteljau = true;
    updateBuffers();
}
void BezierCurve::draw(GLShader& shader, const std::vector<Point>* clipWindow) {
    shader.Begin();

    // Dessiner le polygone de contrôle (lignes bleues)
    if (controlPoints.size() >= 2) {
        shader.SetUniform("color", 0.0f, 0.0f, 1.0f);
        glBindVertexArray(controlPolygonVAO);
        glDrawArrays(GL_LINE_STRIP, 0, controlPoints.size());

        // Dessiner les points de contrôle (points rouges)
        shader.SetUniform("color", 1.0f, 0.0f, 0.0f);
        glBindVertexArray(pointsVAO);
        glPointSize(5.0f);
        glDrawArrays(GL_POINTS, 0, controlPoints.size());
    }

    // Si une fenêtre de découpage est spécifiée et valide, appliquer l'algorithme de Cyrus-Beck
    if (clipWindow && clipWindow->size() >= 3 && CyrusBeck::isPolygonConvex(*clipWindow)) {
        // Utiliser les points calculés par la méthode directe ou De Casteljau
        const std::vector<Point>& curvePoints = directMethodPoints.empty() ?
                                              deCasteljauPoints : directMethodPoints;

        if (!curvePoints.empty()) {
            // Découper la courbe avec l'algorithme de Cyrus-Beck
            std::vector<std::vector<Point>> clippedSegments =
                CyrusBeck::clipCurveToWindow(curvePoints, *clipWindow);

            // Dessiner chaque segment découpé
            for (const auto& segment : clippedSegments) {
                if (segment.size() >= 2) {
                    // Créer un VAO et VBO temporaires pour chaque segment
                    GLuint segmentVAO, segmentVBO;
                    glGenVertexArrays(1, &segmentVAO);
                    glGenBuffers(1, &segmentVBO);

                    glBindVertexArray(segmentVAO);
                    glBindBuffer(GL_ARRAY_BUFFER, segmentVBO);
                    glBufferData(GL_ARRAY_BUFFER, segment.size() * sizeof(Point), segment.data(), GL_STATIC_DRAW);
                    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
                    glEnableVertexAttribArray(0);

                    // Dessiner le segment découpé
                    shader.SetUniform("color", 0.0f, 1.0f, 1.0f); // Cyan
                    glDrawArrays(GL_LINE_STRIP, 0, segment.size());

                    // Nettoyer
                    glDeleteVertexArrays(1, &segmentVAO);
                    glDeleteBuffers(1, &segmentVBO);
                }
            }
        }
    }
    // Sinon, dessiner la courbe normalement
    else {
        // Dessiner la courbe de Bézier (méthode directe)
        if (showDirectMethod && directMethodPoints.size() >= 2) {
            shader.SetUniform("color", 0.0f, 1.0f, 0.0f);
            glBindVertexArray(directMethodVAO);
            glDrawArrays(GL_LINE_STRIP, 0, directMethodPoints.size());
        }

        // Dessiner la courbe de Bézier (méthode de De Casteljau)
        if (showDeCasteljau && deCasteljauPoints.size() >= 2) {
            if (showDirectMethod) {
                shader.SetUniform("color", 1.0f, 0.0f, 1.0f);
            } else {
                shader.SetUniform("color", 0.0f, 1.0f, 0.0f);
            }
            glBindVertexArray(deCasteljauVAO);
            glDrawArrays(GL_LINE_STRIP, 0, deCasteljauPoints.size());
        }
    }

    glBindVertexArray(0);
    shader.End();
}

void BezierCurve::toggleDirectMethod() {
    showDirectMethod = !showDirectMethod;
    if (showDirectMethod && directMethodPoints.empty()) {
        calculateDirectMethod();
    }
}

void BezierCurve::toggleDeCasteljau() {
    showDeCasteljau = !showDeCasteljau;
    if (showDeCasteljau && deCasteljauPoints.empty()) {
        calculateDeCasteljau();
    }
}

void BezierCurve::showBoth() {
    if (directMethodPoints.empty()) {
        calculateDirectMethod();
    }
    if (deCasteljauPoints.empty()) {
        calculateDeCasteljau();
    }
    showDirectMethod = true;
    showDeCasteljau = true;
}

float BezierCurve::getStep() const {
    return step;
}

int BezierCurve::getControlPointCount() const {
    return controlPoints.size();
}

bool BezierCurve::isShowingDirectMethod() const {
    return showDirectMethod;
}

bool BezierCurve::isShowingDeCasteljau() const {
    return showDeCasteljau;
}

// Méthodes de transformation
void BezierCurve::translate(float dx, float dy) {
    for (auto& point : controlPoints) {
        point.x += dx;
        point.y += dy;
    }

    recalculateCurves();
    updateBuffers();
}

void BezierCurve::scale(float sx, float sy) {
    // Calculer le centre du polygone de contrôle
    float centerX = 0.0f, centerY = 0.0f;
    for (const auto& point : controlPoints) {
        centerX += point.x;
        centerY += point.y;
    }
    centerX /= controlPoints.size();
    centerY /= controlPoints.size();

    // Appliquer le scaling par rapport au centre
    for (auto& point : controlPoints) {
        point.x = centerX + (point.x - centerX) * sx;
        point.y = centerY + (point.y - centerY) * sy;
    }

    recalculateCurves();
    updateBuffers();
}

void BezierCurve::rotate(float angle) {
    // Convertir l'angle en radians
    float radians = angle * M_PI / 180.0f;
    float cosA = cos(radians);
    float sinA = sin(radians);

    // Calculer le centre du polygone de contrôle
    float centerX = 0.0f, centerY = 0.0f;
    for (const auto& point : controlPoints) {
        centerX += point.x;
        centerY += point.y;
    }
    centerX /= controlPoints.size();
    centerY /= controlPoints.size();

    // Appliquer la rotation par rapport au centre
    for (auto& point : controlPoints) {
        // Translater au centre
        float x = point.x - centerX;
        float y = point.y - centerY;

        // Appliquer la rotation
        float newX = x * cosA - y * sinA;
        float newY = x * sinA + y * cosA;

        // Translater de retour
        point.x = newX + centerX;
        point.y = newY + centerY;
    }

    recalculateCurves();
    updateBuffers();
}

void BezierCurve::shear(float shx, float shy) {
    // Calculer le centre du polygone de contrôle
    float centerX = 0.0f, centerY = 0.0f;
    for (const auto& point : controlPoints) {
        centerX += point.x;
        centerY += point.y;
    }
    centerX /= controlPoints.size();
    centerY /= controlPoints.size();

    // Appliquer le cisaillement par rapport au centre
    for (auto& point : controlPoints) {
        float dx = point.x - centerX;
        float dy = point.y - centerY;

        point.x = centerX + dx + shx * dy;
        point.y = centerY + dy + shy * dx;
    }

    recalculateCurves();
    updateBuffers();
}

// Méthode pour dupliquer un point de contrôle (multiplicité)
void BezierCurve::duplicateControlPoint(int index) {
    if (index >= 0 && index < controlPoints.size()) {
        Point p = controlPoints[index];
        // Insérer le point dupliqué après le point original
        controlPoints.insert(controlPoints.begin() + index + 1, p);

        // Régénérer le triangle de Pascal si nécessaire
        if (pascalTriangle.size() < controlPoints.size()) {
            generatePascalTriangle(controlPoints.size() - 1);
        }

        recalculateCurves();
        updateBuffers();
    }
}

// Méthodes pour l'enveloppe convexe
int BezierCurve::orientation(const Point& p, const Point& q, const Point& r) const {
    float val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);

    if (fabs(val) < 1e-6) return 0;  // Colinéaire
    return (val > 0) ? 1 : 2;  // Sens horaire ou anti-horaire
}

std::vector<Point> BezierCurve::computeConvexHull() const {
    std::vector<Point> hull;

    // Besoin d'au moins 3 points pour former une enveloppe convexe
    if (controlPoints.size() < 3) {
        return controlPoints;  // Retourner tous les points
    }

    // Trouver le point le plus à gauche
    int leftmost = 0;
    for (int i = 1; i < controlPoints.size(); i++) {
        if (controlPoints[i].x < controlPoints[leftmost].x) {
            leftmost = i;
        }
    }

    int p = leftmost;
    int q;

    // Algorithme de Jarvis (marche du cadeau)
    do {
        hull.push_back(controlPoints[p]);

        q = (p + 1) % controlPoints.size();

        for (int i = 0; i < controlPoints.size(); i++) {
            if (orientation(controlPoints[p], controlPoints[i], controlPoints[q]) == 2) {
                q = i;
            }
        }

        p = q;

    } while (p != leftmost);

    return hull;
}

bool BezierCurve::intersectsWithCurve(const BezierCurve& other) const {
    // Calculer les enveloppes convexes des deux courbes
    std::vector<Point> hull1 = computeConvexHull();
    std::vector<Point> hull2 = other.computeConvexHull();

    // Vérifier si les deux enveloppes convexes s'intersectent
    // Implémentation simple : vérifier si l'un des segments de hull1 intersecte
    // l'un des segments de hull2

    for (int i = 0; i < hull1.size(); i++) {
        int next_i = (i + 1) % hull1.size();

        for (int j = 0; j < hull2.size(); j++) {
            int next_j = (j + 1) % hull2.size();

            // Vérifier si les segments (hull1[i], hull1[next_i]) et (hull2[j], hull2[next_j]) s'intersectent
            int o1 = orientation(hull1[i], hull1[next_i], hull2[j]);
            int o2 = orientation(hull1[i], hull1[next_i], hull2[next_j]);
            int o3 = orientation(hull2[j], hull2[next_j], hull1[i]);
            int o4 = orientation(hull2[j], hull2[next_j], hull1[next_i]);

            // Si les orientations sont différentes, les segments s'intersectent
            if (o1 != o2 && o3 != o4) {
                return true;
            }
        }
    }

    return false;
}

// Méthodes pour le raccordement de courbes
void BezierCurve::joinC0(BezierCurve& other) {
    if (controlPoints.empty() || other.controlPoints.empty()) {
        return;
    }

    // Raccordement C0 : le dernier point de cette courbe est égal au premier point de l'autre courbe
    Point lastPoint = controlPoints.back();
    other.controlPoints[0] = lastPoint;

    other.recalculateCurves();
    other.updateBuffers();
}

void BezierCurve::joinC1(BezierCurve& other) {
    if (controlPoints.size() < 2 || other.controlPoints.size() < 2) {
        return;
    }

    // Raccordement C0
    joinC0(other);

    // Raccordement C1 : les vecteurs tangents aux points de jonction sont colinéaires
    // Nous alignons le deuxième point de la seconde courbe avec la tangente de la première

    // Vecteur tangent à la fin de la première courbe
    Point tangent1 = controlPoints[controlPoints.size() - 1] - controlPoints[controlPoints.size() - 2];

    // Calculer la distance du deuxième point de la seconde courbe par rapport au point de jonction
    Point p0 = other.controlPoints[0];
    Point p1 = other.controlPoints[1];
    float distance = p0.distanceTo(p1);

    // Calculer le nouveau deuxième point pour maintenir la même distance mais suivre la tangente
    Point newP1 = p0 + tangent1 * (distance / tangent1.distanceTo(Point(0, 0)));

    other.controlPoints[1] = newP1;

    other.recalculateCurves();
    other.updateBuffers();
}

void BezierCurve::joinC2(BezierCurve& other) {
    if (controlPoints.size() < 3 || other.controlPoints.size() < 3) {
        return;
    }

    // Raccordement C1
    joinC1(other);

    // Raccordement C2 : continuité de la courbure au point de jonction
    // Nous devons ajuster le troisième point de la seconde courbe

    // Pour une courbe de Bézier cubique, la courbure dépend des trois premiers points
    Point p0 = other.controlPoints[0];
    Point p1 = other.controlPoints[1];

    // Pour une continuité C2, la formule est complexe mais peut être simplifiée
    // dans le cas des courbes de Bézier. Nous utilisons une approximation :

    // Dans la première courbe
    Point q0 = controlPoints[controlPoints.size() - 3];
    Point q1 = controlPoints[controlPoints.size() - 2];
    Point q2 = controlPoints[controlPoints.size() - 1];

    // Calculer le nouveau troisième point pour la continuité C2
    // Formule simplifiée : p2 = 2*p1 - p0 + (q2 - 2*q1 + q0)
    Point newP2 = p1 * 2.0f - p0 + (q2 - q1 * 2.0f + q0);

    if (other.controlPoints.size() > 2) {
        other.controlPoints[2] = newP2;
    } else {
        other.controlPoints.push_back(newP2);
    }

    other.recalculateCurves();
    other.updateBuffers();
}