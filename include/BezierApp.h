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
        EDIT_CONTROL_POINTS,
        CREATE_CLIP_WINDOW,  // Clipping
        EDIT_CLIP_WINDOW     // edit clipping
    };
    BezierApp(const char* title, int width, int height);
    ~BezierApp();

    void run();

private:
    int width, height;
    GLFWwindow* window;
    GLShader* shader;

    std::list<BezierCurve> curves;
    std::list<BezierCurve>::iterator selectedCurveIterator;

    Mode currentMode;
    int selectedPointIndex;
    bool menuNeedsUpdate = true;

    // Fenêtre de découpage (Cyrus-Beck)
    std::vector<Point> clipWindow;
    int selectedClipPointIndex = -1;
    bool enableClipping = false;

    void processInput();
    void render();
    void renderMenu();
    void framebufferSizeCallback(int width, int height);

    void mouseButtonCallback(int button, int action, int mods);
    void keyCallback(int key, int scancode, int action, int mods);

    void createNewCurve();
    void deleteCurve();
    void nextCurve();
    void selectNearestControlPoint(float x, float y);
    void selectNearestClipPoint(float x, float y);  // Déplacée ici
    void clearClipWindow();  // Déplacée ici
};

#endif // BEZIER_APP_H

