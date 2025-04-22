#ifndef BEZIER_APP_H
#define BEZIER_APP_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <list>
#include "../include/GLShader.h"  // Changé ici
#include "../include/BezierCurve.h"

class BezierApp {
public:
    enum class Mode {
        ADD_CONTROL_POINTS,
        EDIT_CONTROL_POINTS
    };

    BezierApp(const char* title, int width, int height);
    ~BezierApp();

    void run();

private:
    int width, height;
    GLFWwindow* window;
    GLShader* shader;  // Changé ici

    std::list<BezierCurve> curves;
    std::list<BezierCurve>::iterator selectedCurveIterator;

    Mode currentMode;
    int selectedPointIndex;
    bool menuNeedsUpdate = true;

    void processInput();
    void render();
    void renderMenu();

    void mouseButtonCallback(int button, int action, int mods);
    void keyCallback(int key, int scancode, int action, int mods);

    void createNewCurve();
    void deleteCurve();
    void nextCurve();
    void selectNearestControlPoint(float x, float y);
};

#endif // BEZIER_APP_H