#ifndef BEZIER_CURVE_H
#define BEZIER_CURVE_H

#include <vector>
#include <GL/glew.h>
#include "Point.h"
#include "Shader.h"

class BezierCurve {
private:
    std::vector<Point> controlPoints;
    std::vector<Point> directMethodPoints;
    std::vector<Point> deCasteljauPoints;
    float step;
    bool showDirectMethod;
    bool showDeCasteljau;

    // VAOs et VBOs pour OpenGL
    GLuint controlPolygonVAO, controlPolygonVBO;
    GLuint directMethodVAO, directMethodVBO;
    GLuint deCasteljauVAO, deCasteljauVBO;
    GLuint pointsVAO, pointsVBO;

    // Calcul du coefficient binomial à l'aide du triangle de Pascal
    std::vector<std::vector<int>> pascalTriangle;

    void generatePascalTriangle(int n);
    int binomialCoeff(int n, int k);

    // Initialiser les VAOs et VBOs
    void setupBuffers();
    void updateBuffers();

public:
    BezierCurve();
    ~BezierCurve();

    void addControlPoint(float x, float y);
    void clearControlPoints();
    void increaseStep();
    void decreaseStep();
    void recalculateCurves();

    void calculateDirectMethod();
    void calculateDeCasteljau();

    void draw(Shader& shader);

    void toggleDirectMethod();
    void toggleDeCasteljau();
    void showBoth();

    float getStep() const;
    int getControlPointCount() const;
    bool isShowingDirectMethod() const;
    bool isShowingDeCasteljau() const;
};

#endif // BEZIER_CURVE_H