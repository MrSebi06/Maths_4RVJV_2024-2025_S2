#ifndef BEZIER_APP_H
#define BEZIER_APP_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <list>
#include <map>
#include <string>
#include "../include/GLShader.h"
#include "../include/BezierCurve.h"
#include "../include/ImGuiManager.h"

class BezierApp {
public:
    enum class Mode {
        ADD_CONTROL_POINTS = 0,
        EDIT_CONTROL_POINTS = 1,
        CREATE_CLIP_WINDOW = 2,
        EDIT_CLIP_WINDOW = 3
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

    // Gestionnaire ImGui
    ImGuiManager imguiManager;

    // Dictionnaire des commandes avec leur description
    std::map<std::string, std::string> commandDescriptions;

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
    void selectNearestClipPoint(float x, float y);
    void clearClipWindow();

    // Initialiser les descriptions des commandes
    void initCommandDescriptions();

    // Convertir le mode en chaîne de caractères
    std::string getModeString() const;
};

#endif // BEZIER_APP_H