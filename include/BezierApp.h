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
        ADD_CONTROL_POINTS = 0,
        EDIT_CONTROL_POINTS = 1,
        CREATE_CLIP_WINDOW = 2,  // Clipping
        EDIT_CLIP_WINDOW = 3     // edit clipping
    };

    enum class ClippingAlgorithm {
        CYRUS_BECK,
        SUTHERLAND_HODGMAN
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

    ClippingAlgorithm clippingAlgorithm = ClippingAlgorithm::CYRUS_BECK;  // Algorithme par défaut

    void processInput();
    void render();
    void renderMenu();
    void framebufferSizeCallback(int width, int height);
    void setMode(Mode newMode);

    void mouseButtonCallback(int button, int action, int mods);
    void keyCallback(int key, int scancode, int action, int mods);

    void createNewCurve();
    void deleteCurve();
    void nextCurve();
    void selectNearestControlPoint(float x, float y);
    void selectNearestClipPoint(float x, float y);  // Déplacée ici
    void validateClippingAlgorithm();
    void clearClipWindow();  // Déplacée ici
};

#endif // BEZIER_APP_H

