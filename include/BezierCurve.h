#ifndef BEZIER_CURVE_H
#define BEZIER_CURVE_H

#include <vector>
#include <GL/glew.h>
#include "../include/GLShader.h"
#include "../include/Point.h"
#include "../include/CyriusBeck.h"

class BezierCurve {
public:
    BezierCurve();
    ~BezierCurve();

    // Méthodes de gestion des points de contrôle
    void addControlPoint(float x, float y);
    void updateControlPoint(int index, float x, float y);
    void removeControlPoint(int index);
    void clearControlPoints();
    Point getControlPoint(int index) const;
    float distanceToControlPoint(int index, float x, float y) const;
    int getNearestControlPoint(float x, float y) const;
    int getControlPointCount() const;

    // Méthodes de gestion du pas
    void increaseStep();
    void decreaseStep();
    float getStep() const;

    // Méthodes de calcul des courbes
    void calculateDirectMethod();
    void calculateDeCasteljau();
    void recalculateCurves();

    // Méthodes d'affichage
    void toggleDirectMethod();
    void toggleDeCasteljau();
    void showBoth();
    bool isShowingDirectMethod() const;
    bool isShowingDeCasteljau() const;
    //void draw(GLShader& shader);
    void draw(GLShader& shader, const std::vector<Point>* clipWindow = nullptr);

    // Méthodes de transformation
    void translate(float dx, float dy);
    void scale(float sx, float sy);
    void rotate(float angle);
    void shear(float shx, float shy);

    // Méthodes pour la multiplicité
    void duplicateControlPoint(int index);

    // Méthodes pour l'enveloppe convexe
    std::vector<Point> computeConvexHull() const;
    bool intersectsWithCurve(const BezierCurve& other) const;

    // Méthodes pour le raccordement
    void joinC0(BezierCurve& other);
    void joinC1(BezierCurve& other);
    void joinC2(BezierCurve& other);

private:
    // Points de contrôle et points de la courbe
    std::vector<Point> controlPoints;
    std::vector<Point> directMethodPoints;
    std::vector<Point> deCasteljauPoints;

    // Triangle de Pascal pour les calculs de combinaisons
    std::vector<std::vector<int>> pascalTriangle;

    // Paramètres
    float step;
    bool showDirectMethod;
    bool showDeCasteljau;

    // Buffers OpenGL
    GLuint controlPolygonVAO, controlPolygonVBO;
    GLuint directMethodVAO, directMethodVBO;
    GLuint deCasteljauVAO, deCasteljauVBO;
    GLuint pointsVAO, pointsVBO;

    // Méthodes internes
    void setupBuffers();
    void updateBuffers();
    void generatePascalTriangle(int n);
    int binomialCoeff(int n, int k);

    // Méthode de Jarvis pour l'enveloppe convexe
    int orientation(const Point& p, const Point& q, const Point& r) const;
};

#endif // BEZIER_CURVE_H