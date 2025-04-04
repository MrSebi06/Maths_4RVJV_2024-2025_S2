#include "../include/BezierApp.h"
#include <iostream>

// Initialisation du singleton
BezierApp* BezierApp::instance = nullptr;
BezierApp::BezierApp() : window(nullptr), windowWidth(800),
                        windowHeight(600), shader(nullptr) {
}

BezierApp::~BezierApp() {
        delete shader;
    glfwTerminate();
}

BezierApp* BezierApp::getInstance() {
    if (instance == nullptr) {
        instance = new BezierApp();
    }
    return instance;
}

bool BezierApp::initialize() {
    // Initialiser GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    // Configurer GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // create window
    window = glfwCreateWindow(windowWidth, windowHeight, "Courbes de Bezier - OpenGL", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(window);
    
    // Init GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return false;
    }
    
    // Configurer le viewport
    glViewport(0, 0, windowWidth, windowHeight);
    
    // Charger les shaders
    try {
        shader = new Shader("shaders/vertex.glsl", "shaders/fragment.glsl");
    } catch (const std::exception& e) {
        std::cerr << "Failed to load shaders: " << e.what() << std::endl;
        return false;
    }
    
    // Configurer les callbacks
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, keyCallback);
    
    // Configurer OpenGL
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);  // Fond blanc
    
    return true;
}

void BezierApp::run() {
    // Boucle principale
    while (!glfwWindowShouldClose(window)) {
        // Traiter les événements
        glfwPollEvents();
        
        // Effacer l'écran
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Configurer le shader pour coordonnées normalisées
        shader->use();
        
        // Transformation pour adapter les coordonnées de la fenêtre
        // (-1, 1) en OpenGL à (0, windowWidth) et (0, windowHeight)
        float scaleX = 2.0f / windowWidth;
        float scaleY = 2.0f / windowHeight;
        shader->setVec2("scale", scaleX, -scaleY);  // Y inversé car OpenGL a l'origine en bas
        shader->setVec2("translate", -1.0f, 1.0f);
        
        // Dessiner la courbe de Bézier
        bezier.draw(*shader);
        
        // Draw text (instructions, Info, etc.)
        renderText();
        
        // exchange buffers
        glfwSwapBuffers(window);
    }
}

void BezierApp::renderText() {
    // Note: Pour simplifier, cette implémentation n'inclut pas le rendu de texte.
    // Le rendu de texte en OpenGL moderne est complexe et nécessiterait une bibliothèque
    // comme FreeType. Pour l'instant, on affiche les informations dans la console.
    
    // Afficher les infos dans la console à chaque changement significatif
    static int lastPointCount = -1;
    static float lastStep = -1.0f;
    static bool lastDirectMethod = false;
    static bool lastDeCasteljau = false;
    
    if (lastPointCount != bezier.getControlPointCount() ||
        lastStep != bezier.getStep() ||
        lastDirectMethod != bezier.isShowingDirectMethod() ||
        lastDeCasteljau != bezier.isShowingDeCasteljau()) {
        
        std::cout << "\n--- État actuel ---" << std::endl;
        std::cout << "Points de contrôle: " << bezier.getControlPointCount() << std::endl;
        std::cout << "Pas: " << bezier.getStep() << std::endl;
        std::cout << "Méthode directe: " << (bezier.isShowingDirectMethod() ? "Activée" : "Désactivée") << std::endl;
        std::cout << "De Casteljau: " << (bezier.isShowingDeCasteljau() ? "Activée" : "Désactivée") << std::endl;
        std::cout << "-------------------" << std::endl;
        
        lastPointCount = bezier.getControlPointCount();
        lastStep = bezier.getStep();
        lastDirectMethod = bezier.isShowingDirectMethod();
        lastDeCasteljau = bezier.isShowingDeCasteljau();
    }
}

// Callbacks statiques
void BezierApp::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    BezierApp* app = getInstance();
    app->windowWidth = width;
    app->windowHeight = height;
    glViewport(0, 0, width, height);
}

void BezierApp::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        BezierApp* app = getInstance();
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        
        app->bezier.addControlPoint(xpos, ypos);
        app->bezier.recalculateCurves();
    }
}

void BezierApp::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        BezierApp* app = getInstance();
        BezierCurve& bezier = app->bezier;
        
        switch (key) {
            case GLFW_KEY_1:
                bezier.toggleDirectMethod();
                break;
            case GLFW_KEY_2:
                bezier.toggleDeCasteljau();
                break;
            case GLFW_KEY_3:
                bezier.showBoth();
                break;
            case GLFW_KEY_C:
                bezier.clearControlPoints();
                break;
            case GLFW_KEY_EQUAL:  // Touche '+'
                bezier.decreaseStep();  // Diminuer le pas = augmenter la précision
                break;
            case GLFW_KEY_MINUS:  // Touche '-'
                bezier.increaseStep();  // Augmenter le pas = diminuer la précision
                break;
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, GLFW_TRUE);
                break;
        }
    }
}

BezierCurve& BezierApp::getBezierCurve() {
    return bezier;
}

int BezierApp::getWidth() const {
    return windowWidth;
}

int BezierApp::getHeight() const {
    return windowHeight;
}