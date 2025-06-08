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
        CREATE_CLIP_WINDOW = 2,  // Clipping
        EDIT_CLIP_WINDOW = 3     // edit clipping
    };

    enum class CursorMode {
        NORMAL = 0,      // Curseur système normal
        HIDDEN = 1,      // Curseur système caché
        DISABLED = 2     // Curseur système désactivé (mode capture)
    };

    BezierApp(const char* title, int width, int height);
    ~BezierApp();

    void toggleClippingAlgorithm();
    void run();

private:
    int width, height;
    GLFWwindow* window;
    GLShader* shader;

    // Curseur personnalisé
    float mouseX, mouseY;  // Position du curseur en coordonnées OpenGL
    double screenMouseX, screenMouseY; // Position du curseur en coordonnées écran
    GLuint cursorVAO, cursorVBO;
    bool isPointHovered;   // Indique si un point est actuellement survolé
    int hoveredPointIndex; // Indice du point survolé
    CursorMode cursorMode; // Mode du curseur actuel

    std::list<BezierCurve> curves;
    std::list<BezierCurve>::iterator selectedCurveIterator;

    Mode currentMode;
    int selectedPointIndex;
    bool menuNeedsUpdate = true;

    // Fenêtre de découpage (Cyrus-Beck)
    std::vector<Point> clipWindow;
    int selectedClipPointIndex = -1;
    bool enableClipping = false;
    int hoveredClipPointIndex = -1; // Indice du point de découpage survolé

    // Gestionnaire ImGui
    ImGuiManager imguiManager;

    // Paramètres de sélection
    float selectionPadding = 0.03f; // Rayon de sélection des points augmenté

    // Dictionnaire des commandes avec leur description
    std::map<std::string, std::string> commandDescriptions;

    bool usesSutherlandHodgman;

    void processInput();
    void render();
    void renderCursor(); // Méthode pour dessiner le curseur
    void renderMenu();
    void framebufferSizeCallback(int width, int height);
    void setMode(Mode newMode);

    void cursorPositionCallback(double xpos, double ypos); // Callback pour la position du curseur
    void mouseButtonCallback(int button, int action, int mods);
    void keyCallback(int key, int scancode, int action, int mods);

    void createNewCurve();
    void deleteCurve();
    void nextCurve();
    void selectNearestControlPoint(float x, float y);
    bool checkPointHover(float x, float y); // Vérifie si un point est survolé
    void selectNearestClipPoint(float x, float y);
    bool checkClipPointHover(float x, float y); // Vérifie si un point de découpage est survolé
    void clearClipWindow();

    // Initialiser les descriptions des commandes
    void initCommandDescriptions();

    // Convertir le mode en chaîne de caractères
    std::string getModeString() const;

    // Conversion coordonnées souris <-> OpenGL
    void mouseToOpenGL(double mouseX, double mouseY, float& oglX, float& oglY) const;
    void setupCursorBuffers();
};

#endif // BEZIER_APP_H