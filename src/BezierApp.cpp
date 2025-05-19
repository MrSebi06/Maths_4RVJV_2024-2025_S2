#include "../include/BezierApp.h"
#include <iostream>
#include "../include/CyriusBeck.h"
#include <algorithm>

BezierApp::BezierApp(const char* title, int width, int height)
    : width(width), height(height), currentMode(Mode::ADD_CONTROL_POINTS),
      selectedCurveIterator(curves.end()), selectedPointIndex(-1),
      selectedClipPointIndex(-1), enableClipping(false) {

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

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
        BezierApp* app = static_cast<BezierApp*>(glfwGetWindowUserPointer(window));
        app->framebufferSizeCallback(width, height);
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

void BezierApp::framebufferSizeCallback(int newWidth, int newHeight) {
    width = newWidth;
    height = newHeight;
    glViewport(0, 0, width, height);

    // Si nécessaire, recalculer et redessiner toutes les courbes
    for (auto& curve : curves) {
        curve.recalculateCurves();
    }

    std::cout << "Fenêtre redimensionnée: " << width << "x" << height << std::endl;
}

void BezierApp::setMode(Mode newMode) {
    currentMode = newMode;
    std::cout << "Mode changé en: " << static_cast<int>(currentMode) << std::endl;

    // Autres actions à effectuer lors du changement de mode
    if (newMode == Mode::CREATE_CLIP_WINDOW) {
        std::cout << "Passé en mode création de fenêtre de découpage" << std::endl;
        // Vous pouvez ajouter d'autres initialisations ici
    }
}

void BezierApp::processInput() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    // Récupérer la position de la souris
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    // Convertir les coordonnées de la souris en coordonnées OpenGL
    float x = (2.0f * xpos) / width - 1.0f;
    float y = 1.0f - (2.0f * ypos) / height;

    // Déplacement d'un point de contrôle sélectionné avec la souris
    if (currentMode == Mode::EDIT_CONTROL_POINTS && selectedPointIndex != -1 && selectedCurveIterator != curves.end()) {
        // Déplacer le point de contrôle sélectionné
        selectedCurveIterator->updateControlPoint(selectedPointIndex, x, y);
    }
    // Déplacement d'un point de la fenêtre de découpage
    else if (currentMode == Mode::EDIT_CLIP_WINDOW && selectedClipPointIndex != -1) {
        // Déplacer le point de la fenêtre sélectionné
        clipWindow[selectedClipPointIndex].x = x;
        clipWindow[selectedClipPointIndex].y = y;

        // Vérifier si la fenêtre est toujours convexe
        if (clipWindow.size() >= 3) {
            if (!CyrusBeck::isPolygonConvex(clipWindow)) {
                std::cout << "Attention: La fenetre n'est pas convexe!" << std::endl;
            }
        }
    }
}

void BezierApp::render() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Afficher l'algorithme actuel à l'écran
    std::string algoText = "Algorithme : ";
    algoText += (clippingAlgorithm == ClippingAlgorithm::CYRUS_BECK) ?
                "Cyrus-Beck" : "Sutherland-Hodgman";

    // Dessiner toutes les courbes avec ou sans découpage
    for (auto& curve : curves) {
        if (enableClipping && clipWindow.size() >= 3) {
            switch (clippingAlgorithm) {
            case ClippingAlgorithm::CYRUS_BECK:
                // Utiliser Cyrus-Beck pour découper des segments
                    if (CyrusBeck::isPolygonConvex(clipWindow)) {
                        curve.draw(*shader, &clipWindow);
                    } else {
                        curve.draw(*shader);  // Fenêtre non convexe, dessiner sans découpage
                        std::cout << "Attention : Cyrus-Beck nécessite une fenêtre convexe" << std::endl;
                    }
                break;

            case ClippingAlgorithm::SUTHERLAND_HODGMAN:
                // Utiliser Sutherland-Hodgman pour découper des polygones
                    if (curve.isClosedCurve()) {
                        std::vector<Point> clippedPolygon = curve.clipClosedCurveWithSH(clipWindow);
                        curve.draw(*shader);  // Dessiner la courbe originale
                        curve.drawClippedWithSH(*shader, clippedPolygon);  // Dessiner le résultat découpé
                    } else {
                        curve.draw(*shader);  // Courbe non fermée, dessiner sans découpage
                        std::cout << "Attention : Sutherland-Hodgman nécessite une courbe fermée" << std::endl;
                    }
                break;
            }
        } else {
            curve.draw(*shader);  // Pas de découpage
        }
    }

    // Afficher le polygone de découpage s'il existe, quel que soit le mode actuel
    // Cette modification permet de garder le polygone visible même en changeant de mode
    if (!clipWindow.empty()) {
        shader->Begin();

        // Couleur différente selon l'algorithme
        if (clippingAlgorithm == ClippingAlgorithm::CYRUS_BECK) {
            shader->SetUniform("color", 1.0f, 1.0f, 0.0f);  // Jaune pour Cyrus-Beck
        } else {
            shader->SetUniform("color", 0.0f, 1.0f, 1.0f);  // Cyan pour Sutherland-Hodgman
        }

        // Dessiner les points de la fenêtre
        shader->SetUniform("color", 1.0f, 1.0f, 0.0f);  // Jaune

        // Créer un VAO et VBO temporaires pour les points
        GLuint pointsVAO, pointsVBO;
        glGenVertexArrays(1, &pointsVAO);
        glGenBuffers(1, &pointsVBO);

        glBindVertexArray(pointsVAO);
        glBindBuffer(GL_ARRAY_BUFFER, pointsVBO);
        glBufferData(GL_ARRAY_BUFFER, clipWindow.size() * sizeof(Point), clipWindow.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
        glEnableVertexAttribArray(0);

        glPointSize(7.0f);
        glDrawArrays(GL_POINTS, 0, clipWindow.size());

        // Dessiner les lignes de la fenêtre
        if (clipWindow.size() >= 2) {
            // Changer la couleur en fonction de la convexité
            if (CyrusBeck::isPolygonConvex(clipWindow)) {
                shader->SetUniform("color", 0.7f, 0.7f, 0.0f);  // Jaune foncé si convexe
            } else {
                shader->SetUniform("color", 1.0f, 0.0f, 0.0f);  // Rouge si non convexe
            }

            glDrawArrays(GL_LINE_LOOP, 0, clipWindow.size());
        }

        glDeleteVertexArrays(1, &pointsVAO);
        glDeleteBuffers(1, &pointsVBO);

        shader->End();
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

        // Ajouter les nouvelles commandes de fenêtrage
        std::cout << "F: Mode creation de fenetre de decoupage" << std::endl;
        std::cout << "G: Mode edition de fenetre de decoupage" << std::endl;
        std::cout << "X: Activer/désactiver le découpage" << std::endl;
        std::cout << "H: Basculer entre algorithmes de découpage" << std::endl;
        std::cout << "   Algorithme actuel: " <<
            (clippingAlgorithm == ClippingAlgorithm::CYRUS_BECK ?
             "Cyrus-Beck (segments)" : "Sutherland-Hodgman (polygones)") << std::endl;
        std::cout << "Delete: Supprimer un point selectionne" << std::endl;
        std::cout << "Backspace: Effacer la fenetre de decoupage" << std::endl;

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

        switch (currentMode) {
        case Mode::ADD_CONTROL_POINTS:
            if (selectedCurveIterator != curves.end()) {
                selectedCurveIterator->addControlPoint(x, y);
                std::cout << "Point de contrôle ajouté: (" << x << ", " << y << ")" << std::endl;
            }
            break;

        case Mode::EDIT_CONTROL_POINTS:
            // Sélectionner le point de contrôle le plus proche
                selectNearestControlPoint(x, y);
            break;

        case Mode::CREATE_CLIP_WINDOW:
            // Ajouter un point à la fenêtre de découpage
                clipWindow.emplace_back(x, y);
            std::cout << "Point de fenêtre ajouté: (" << x << ", " << y << ")" << std::endl;

            // Vérifier si la fenêtre est convexe
            if (clipWindow.size() >= 3) {
                if (CyrusBeck::isPolygonConvex(clipWindow)) {
                    std::cout << "La fenêtre est convexe." << std::endl;
                } else {
                    std::cout << "Attention: La fenêtre n'est pas convexe!" << std::endl;
                }
            }
            break;

        case Mode::EDIT_CLIP_WINDOW:
            // Sélectionner le point de la fenêtre le plus proche
                selectNearestClipPoint(x, y);
            break;
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        // Désélectionner le point lors du relâchement du bouton
        if (currentMode == Mode::EDIT_CONTROL_POINTS) {
            selectedPointIndex = -1;
        } else if (currentMode == Mode::EDIT_CLIP_WINDOW) {
            selectedClipPointIndex = -1;
        }
    }
}

void BezierApp::keyCallback(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        std::cout << "Touche pressée: " << key << " (code ASCII)" << std::endl;
    }
    if (action == GLFW_PRESS) {
        switch (key)
        {
        case GLFW_KEY_B:
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

        case GLFW_KEY_F:  // W pour Window (fenêtre)
            currentMode = Mode::CREATE_CLIP_WINDOW;
            std::cout << "Mode changer en: Creation de fenetre de decoupage ACTIVE" << std::endl;
            break;


        case GLFW_KEY_G:  // F pour Fenêtre
            currentMode = Mode::EDIT_CLIP_WINDOW;
            std::cout << "Mode: Edition de la fenetre de decoupage" << std::endl;
            break;

        case GLFW_KEY_X:
            enableClipping = !enableClipping;
            std::cout << "Découpage: " << (enableClipping ? "Activé" : "Désactivé") << std::endl;
            if (enableClipping) {
                validateClippingAlgorithm();  // Vérifier les conditions si on active le découpage
            }
            break;

        case GLFW_KEY_DELETE:
            if (currentMode == Mode::EDIT_CLIP_WINDOW && selectedClipPointIndex != -1) {
                // Supprimer un point de la fenêtre
                clipWindow.erase(clipWindow.begin() + selectedClipPointIndex);
                selectedClipPointIndex = -1;
                std::cout << "Point de fenêtre supprimé" << std::endl;
            } else if (currentMode == Mode::EDIT_CONTROL_POINTS && selectedPointIndex != -1 && selectedCurveIterator != curves.end()) {
                // Supprimer un point de contrôle
                selectedCurveIterator->removeControlPoint(selectedPointIndex);
                selectedPointIndex = -1;
                std::cout << "Point de contrôle supprimé" << std::endl;
            }
            break;

        case GLFW_KEY_BACKSPACE:
            clearClipWindow();
            break;

        case GLFW_KEY_H:
            // Basculer l'algorithme
                if (clippingAlgorithm == ClippingAlgorithm::CYRUS_BECK) {
                    clippingAlgorithm = ClippingAlgorithm::SUTHERLAND_HODGMAN;
                } else {
                    clippingAlgorithm = ClippingAlgorithm::CYRUS_BECK;
                }
            validateClippingAlgorithm();  // Vérifier les conditions
            break;
        }
        menuNeedsUpdate = true;
    }
}

void BezierApp::createNewCurve() {
    // Ajouter une nouvelle courbe vide à la liste
    curves.emplace_back();

    // Sélectionner la nouvelle courbe (la dernière de la liste)
    selectedCurveIterator = std::prev(curves.end());

    // Réinitialiser les indices de points sélectionnés
    selectedPointIndex = -1;

    std::cout << "Nouvelle courbe créée. Total: " << curves.size() << std::endl;
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

void BezierApp::selectNearestClipPoint(float x, float y) {
    if (clipWindow.empty()) {
        selectedClipPointIndex = -1;
        return;
    }

    Point p(x, y);
    int nearestIndex = 0;
    float minDistance = clipWindow[0].distanceTo(p);

    for (int i = 1; i < clipWindow.size(); ++i) {
        float distance = clipWindow[i].distanceTo(p);
        if (distance < minDistance) {
            minDistance = distance;
            nearestIndex = i;
        }
    }

    // Seuil de sélection (ajustable)
    if (minDistance > 0.1f) {
        selectedClipPointIndex = -1;  // Aucun point assez proche
    } else {
        selectedClipPointIndex = nearestIndex;
        std::cout << "Point de fenêtre sélectionné: " << selectedClipPointIndex << std::endl;
    }
}

void BezierApp::validateClippingAlgorithm() {
    if (enableClipping) {
        // Vérifier la convexité pour Cyrus-Beck
        if (clippingAlgorithm == ClippingAlgorithm::CYRUS_BECK && !CyrusBeck::isPolygonConvex(clipWindow)) {
            std::cout << "Attention : Cyrus-Beck nécessite une fenêtre convexe!" << std::endl;
        }

        // Vérifier que les courbes sont fermées pour Sutherland-Hodgman
        if (clippingAlgorithm == ClippingAlgorithm::SUTHERLAND_HODGMAN) {
            bool hasClosedCurve = false;
            for (auto& curve : curves) {
                if (curve.isClosedCurve()) {
                    hasClosedCurve = true;
                    break;
                }
            }

            if (!hasClosedCurve) {
                std::cout << "Attention : Sutherland-Hodgman necessite des courbes fermees!" << std::endl;
                std::cout << "Fermez une courbe en faisant coincider le premier et le dernier point" << std::endl;
            }
        }
    }
}

// Ajouter cette méthode pour effacer la fenêtre de découpage
void BezierApp::clearClipWindow() {
    clipWindow.clear();
    selectedClipPointIndex = -1;
    std::cout << "Fenêtre de découpage effacée" << std::endl;
}