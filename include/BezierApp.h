#ifndef BEZIER_APP_H
#define BEZIER_APP_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "BezierCurve.h"
#include "Shader.h"

class BezierApp {
private:
    static BezierApp* instance;

    GLFWwindow* window;
    int windowWidth;
    int windowHeight;

    BezierCurve bezier;
    Shader* shader;

    // private constructer (singleton)
    BezierApp();

    // Méthodes pour gérer le texte (rendu en bitmap)
    void renderText();

    // Callbacks GLFW
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

public:
    ~BezierApp();

    static BezierApp* getInstance();

    bool initialize();
    void run();

    // Accesseurs
    BezierCurve& getBezierCurve();
    int getWidth() const;
    int getHeight() const;
};

#endif // BEZIER_APP_H