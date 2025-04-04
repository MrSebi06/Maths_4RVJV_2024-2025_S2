#include "../include/BezierCurve.h"
#include <iostream>
#include <chrono>
#include <cmath>

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

    updateBuffers();
}

void BezierCurve::clearControlPoints() {
    controlPoints.clear();
    directMethodPoints.clear();
    deCasteljauPoints.clear();
    showDirectMethod = false;
    showDeCasteljau = false;
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

void BezierCurve::draw(Shader& shader) {
    shader.use();

    // Dessiner le polygone de contrôle (lignes bleues)
    if (controlPoints.size() >= 2) {
        shader.setVec3("color", 0.0f, 0.0f, 1.0f); // Bleu
        glBindVertexArray(controlPolygonVAO);
        glDrawArrays(GL_LINE_STRIP, 0, controlPoints.size());

        // Dessiner les points de contrôle (points rouges)
        shader.setVec3("color", 1.0f, 0.0f, 0.0f); // Rouge
        glBindVertexArray(pointsVAO);
        glPointSize(5.0f);
        glDrawArrays(GL_POINTS, 0, controlPoints.size());
    }

    // Dessiner la courbe de Bézier (méthode directe)
    if (showDirectMethod && directMethodPoints.size() >= 2) {
        shader.setVec3("color", 0.0f, 1.0f, 0.0f); // Vert
        glBindVertexArray(directMethodVAO);
        glDrawArrays(GL_LINE_STRIP, 0, directMethodPoints.size());
    }

    // Dessiner la courbe de Bézier (méthode de De Casteljau)
    if (showDeCasteljau && deCasteljauPoints.size() >= 2) {
        if (showDirectMethod) {
            shader.setVec3("color", 1.0f, 0.0f, 1.0f); // Magenta
        } else {
            shader.setVec3("color", 0.0f, 1.0f, 0.0f); // Vert
        }
        glBindVertexArray(deCasteljauVAO);
        glDrawArrays(GL_LINE_STRIP, 0, deCasteljauPoints.size());
    }

    glBindVertexArray(0);
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