#include "../include/BezierApp.h"
#include <iostream>
#include <algorithm>

BezierApp::BezierApp(const char* title, int width, int height)
    : width(width), height(height), currentMode(Mode::ADD_CONTROL_POINTS),
      selectedCurveIterator(curves.end()), selectedPointIndex(-1) {

    // Initialisation de GLFW
    if (!glfwInit()) {
        std::cerr << "Échec de l'initialisation de GLFW" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Configuration de GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Création de la fenêtre
    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window) {
        std::cerr << "Échec de la création de la fenêtre GLFW" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);

    // Initialisation de GLEW
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Échec de l'initialisation de GLEW: " << glewGetErrorString(err) << std::endl;
        exit(EXIT_FAILURE);
    }

    // Configuration du viewport
    glViewport(0, 0, width, height);

    // Création du shader

    shader = new GLShader();
    bool vsOk = shader->LoadVertexShader("../shader/basic.vs.glsl");
    bool fsOk = shader->LoadFragmentShader("../shader/basic.fs.glsl");
    bool createOk = shader->Create();

    if (!vsOk || !fsOk || !createOk) {
        std::cerr << "Erreur lors du chargement des shaders!" << std::endl;
        // Ajoutez cette ligne pour terminer le programme proprement
        exit(EXIT_FAILURE);
    }
    // Configuration des callbacks
    glfwSetWindowUserPointer(window, this);

    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
        BezierApp* app = static_cast<BezierApp*>(glfwGetWindowUserPointer(window));
        app->mouseButtonCallback(button, action, mods);
    });

    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        BezierApp* app = static_cast<BezierApp*>(glfwGetWindowUserPointer(window));
        app->keyCallback(key, scancode, action, mods);
    });

    // Création de la première courbe vide
    createNewCurve();
}

BezierApp::~BezierApp() {
    delete shader;
    glfwDestroyWindow(window);
    glfwTerminate();
}

void BezierApp::run() {
    // Afficher la version d'OpenGL et les informations du renderer
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "Renderer: " << renderer << std::endl;
    std::cout << "OpenGL version: " << version << std::endl;

    // Activer le mélange des couleurs et la taille des points
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPointSize(5.0f); // Points plus gros pour mieux les voir

    while (!glfwWindowShouldClose(window)) {
        processInput();
        render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void BezierApp::processInput() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // Déplacement d'un point de contrôle sélectionné avec la souris
    if (currentMode == Mode::EDIT_CONTROL_POINTS && selectedPointIndex != -1 && selectedCurveIterator != curves.end()) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        // Convertir les coordonnées de la souris en coordonnées OpenGL
        float x = (2.0f * xpos) / width - 1.0f;
        float y = 1.0f - (2.0f * ypos) / height;

        // Déplacer le point de contrôle sélectionné
        selectedCurveIterator->updateControlPoint(selectedPointIndex, x, y);
    }
}

void BezierApp::render() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Dessiner toutes les courbes
    for (auto& curve : curves) {
        curve.draw(*shader);
    }

    // Afficher le menu à l'écran
    renderMenu();
}

void BezierApp::renderMenu() {
    // Implémentation simple du menu - à améliorer avec une bibliothèque d'interface utilisateur
    // Pour l'instant, nous affichons les commandes dans la console
    if (menuNeedsUpdate) {
        std::cout << "\n=== Menu des commandes ===" << std::endl;
        std::cout << "A: Mode ajout de points" << std::endl;
        std::cout << "E: Mode édition de points" << std::endl;
        std::cout << "N: Nouvelle courbe" << std::endl;
        std::cout << "D: Supprimer la courbe courante" << std::endl;
        std::cout << "+/-: Augmenter/diminuer le pas" << std::endl;
        std::cout << "1: Afficher/masquer courbe (méthode directe)" << std::endl;
        std::cout << "2: Afficher/masquer courbe (De Casteljau)" << std::endl;
        std::cout << "3: Afficher les deux courbes" << std::endl;
        std::cout << "T: Appliquer une translation" << std::endl;
        std::cout << "S: Appliquer un scaling" << std::endl;
        std::cout << "R: Appliquer une rotation" << std::endl;
        std::cout << "C: Appliquer un cisaillement" << std::endl;
        std::cout << "Tab: Passer à la courbe suivante" << std::endl;
        std::cout << "=========================" << std::endl;

        menuNeedsUpdate = false;
    }
}

void BezierApp::mouseButtonCallback(int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        // Convertir les coordonnées de la souris en coordonnées OpenGL
        float x = (2.0f * xpos) / width - 1.0f;
        float y = 1.0f - (2.0f * ypos) / height;

        if (currentMode == Mode::ADD_CONTROL_POINTS && selectedCurveIterator != curves.end()) {
            selectedCurveIterator->addControlPoint(x, y);
            std::cout << "Point de contrôle ajouté: (" << x << ", " << y << ")" << std::endl;
        }
        else if (currentMode == Mode::EDIT_CONTROL_POINTS) {
            // Sélectionner le point de contrôle le plus proche
            selectNearestControlPoint(x, y);
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        // Désélectionner le point lors du relâchement du bouton
        if (currentMode == Mode::EDIT_CONTROL_POINTS) {
            selectedPointIndex = -1;
        }
    }
}

void BezierApp::keyCallback(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_A:
                currentMode = Mode::ADD_CONTROL_POINTS;
                std::cout << "Mode: Ajout de points de contrôle" << std::endl;
                break;

            case GLFW_KEY_E:
                currentMode = Mode::EDIT_CONTROL_POINTS;
                std::cout << "Mode: Édition de points de contrôle" << std::endl;
                break;

            case GLFW_KEY_N:
                createNewCurve();
                std::cout << "Nouvelle courbe créée" << std::endl;
                break;

            case GLFW_KEY_D:
                deleteCurve();
                break;

            case GLFW_KEY_EQUAL: // Touche '+'
                if (selectedCurveIterator != curves.end()) {
                    selectedCurveIterator->increaseStep();
                }
                break;

            case GLFW_KEY_MINUS: // Touche '-'
                if (selectedCurveIterator != curves.end()) {
                    selectedCurveIterator->decreaseStep();
                }
                break;

            case GLFW_KEY_1:
                if (selectedCurveIterator != curves.end()) {
                    selectedCurveIterator->toggleDirectMethod();
                }
                break;

            case GLFW_KEY_2:
                if (selectedCurveIterator != curves.end()) {
                    selectedCurveIterator->toggleDeCasteljau();
                }
                break;

            case GLFW_KEY_3:
                if (selectedCurveIterator != curves.end()) {
                    selectedCurveIterator->showBoth();
                }
                break;

            case GLFW_KEY_TAB:
                nextCurve();
                break;

            case GLFW_KEY_T:
                // Appliquer une translation
                if (selectedCurveIterator != curves.end()) {
                    float dx = 0.1f; // À ajuster
                    float dy = 0.1f; // À ajuster
                    selectedCurveIterator->translate(dx, dy);
                }
                break;

            case GLFW_KEY_S:
                // Appliquer un scaling
                if (selectedCurveIterator != curves.end()) {
                    float sx = 1.1f; // À ajuster
                    float sy = 1.1f; // À ajuster
                    selectedCurveIterator->scale(sx, sy);
                }
                break;

            case GLFW_KEY_R:
                // Appliquer une rotation
                if (selectedCurveIterator != curves.end()) {
                    float angle = 15.0f; // Rotation de 15 degrés
                    selectedCurveIterator->rotate(angle);
                }
                break;

            case GLFW_KEY_C:
                // Appliquer un cisaillement
                if (selectedCurveIterator != curves.end()) {
                    float shx = 0.1f; // À ajuster
                    float shy = 0.0f; // À ajuster
                    selectedCurveIterator->shear(shx, shy);
                }
                break;
        }

        menuNeedsUpdate = true;
    }
}

void BezierApp::createNewCurve() {
    curves.emplace_back();
    selectedCurveIterator = std::prev(curves.end());
    std::cout << "Nouvelle courbe sélectionnée. Total: " << curves.size() << std::endl;
}

void BezierApp::deleteCurve() {
    if (curves.empty()) {
        std::cout << "Aucune courbe à supprimer" << std::endl;
        return;
    }

    if (selectedCurveIterator != curves.end()) {
        auto nextIterator = std::next(selectedCurveIterator);
        if (nextIterator == curves.end()) {
            nextIterator = curves.begin();
        }

        curves.erase(selectedCurveIterator);

        if (curves.empty()) {
            selectedCurveIterator = curves.end();
            std::cout << "Toutes les courbes ont été supprimées" << std::endl;
        } else {
            selectedCurveIterator = nextIterator;
            std::cout << "Courbe supprimée. Courbe suivante sélectionnée." << std::endl;
        }
    }
}

void BezierApp::nextCurve() {
    if (curves.empty()) {
        std::cout << "Aucune courbe disponible" << std::endl;
        return;
    }

    if (selectedCurveIterator == curves.end()) {
        selectedCurveIterator = curves.begin();
    } else {
        selectedCurveIterator++;
        if (selectedCurveIterator == curves.end()) {
            selectedCurveIterator = curves.begin();
        }
    }

    std::cout << "Courbe suivante sélectionnée" << std::endl;
}

void BezierApp::selectNearestControlPoint(float x, float y) {
    if (selectedCurveIterator == curves.end()) return;

    selectedPointIndex = selectedCurveIterator->getNearestControlPoint(x, y);
    if (selectedPointIndex != -1) {
        std::cout << "Point de contrôle sélectionné: " << selectedPointIndex << std::endl;
    }
}