#include "../include/BezierApp.h"
#include <iostream>
#include <cmath>
#include "../include/CyriusBeck.h"
#include <algorithm>
#include "imgui.h"

// Définition de M_PI si nécessaire
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

BezierApp::BezierApp(const char* title, int width, int height)
    : width(width), height(height), currentMode(Mode::ADD_CONTROL_POINTS),
      selectedCurveIterator(curves.end()), selectedPointIndex(-1),
      selectedClipPointIndex(-1), enableClipping(false),
      mouseX(0.0f), mouseY(0.0f), screenMouseX(0), screenMouseY(0),
      isPointHovered(false), hoveredPointIndex(-1),
      hoveredClipPointIndex(-1), cursorMode(CursorMode::HIDDEN) {

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

    // Simplement cacher le curseur système
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

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

    // Nouveau callback pour le mouvement du curseur
    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
        BezierApp* app = static_cast<BezierApp*>(glfwGetWindowUserPointer(window));
        app->cursorPositionCallback(xpos, ypos);
    });

    // Création du VAO/VBO pour le curseur
    setupCursorBuffers();

    // Création de la première courbe vide
    createNewCurve();

    // Initialiser ImGui
    imguiManager.init(window);

    // Initialiser les descriptions des commandes
    initCommandDescriptions();
}

BezierApp::~BezierApp() {
    // Nettoyer les ressources du curseur
    glDeleteVertexArrays(1, &cursorVAO);
    glDeleteBuffers(1, &cursorVBO);

    delete shader;
    glfwDestroyWindow(window);
    glfwTerminate();
}

void BezierApp::setupCursorBuffers() {
    // Création du VAO/VBO pour le curseur personnalisé (croix)
    glGenVertexArrays(1, &cursorVAO);
    glGenBuffers(1, &cursorVBO);

    // Les coordonnées pour dessiner une croix (elles seront mises à jour dynamiquement)
    float cursorVertices[8] = {0}; // Initialiser avec des zéros

    glBindVertexArray(cursorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cursorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cursorVertices), cursorVertices, GL_DYNAMIC_DRAW); // DYNAMIC_DRAW car nous mettrons à jour souvent
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void BezierApp::mouseToOpenGL(double mouseX, double mouseY, float& oglX, float& oglY) const {
    // Conversion simple des coordonnées de la souris en coordonnées OpenGL
    oglX = (2.0f * static_cast<float>(mouseX) / static_cast<float>(width)) - 1.0f;
    oglY = 1.0f - (2.0f * static_cast<float>(mouseY) / static_cast<float>(height));
}

void BezierApp::cursorPositionCallback(double xpos, double ypos) {
    // Stocker les coordonnées écran
    screenMouseX = xpos;
    screenMouseY = ypos;

    // Convertir en coordonnées OpenGL
    mouseToOpenGL(xpos, ypos, mouseX, mouseY);

    // Vérifier si un point est survolé
    isPointHovered = checkPointHover(mouseX, mouseY);

    // Vérifier si un point de découpage est survolé
    if (!isPointHovered) {
        checkClipPointHover(mouseX, mouseY);
    }
}

bool BezierApp::checkPointHover(float x, float y) {
    // Vérifie si un point de contrôle est survolé
    hoveredPointIndex = -1;

    if (selectedCurveIterator == curves.end()) return false;

    // Vérifier chaque point de contrôle avec le padding de sélection
    for (int i = 0; i < selectedCurveIterator->getControlPointCount(); i++) {
        Point p = selectedCurveIterator->getControlPoint(i);
        float distance = std::sqrt((p.x - x) * (p.x - x) + (p.y - y) * (p.y - y));

        if (distance < selectionPadding) {
            hoveredPointIndex = i;
            return true;
        }
    }

    return false;
}

bool BezierApp::checkClipPointHover(float x, float y) {
    // Vérifie si un point de découpage est survolé
    hoveredClipPointIndex = -1;

    if (clipWindow.empty()) return false;

    // Vérifier chaque point de découpage avec le padding de sélection
    for (int i = 0; i < clipWindow.size(); i++) {
        Point p = clipWindow[i];
        float distance = std::sqrt((p.x - x) * (p.x - x) + (p.y - y) * (p.y - y));

        if (distance < selectionPadding) {
            hoveredClipPointIndex = i;
            return true;
        }
    }

    return false;
}

void BezierApp::initCommandDescriptions() {
    commandDescriptions["A / B"] = "Mode ajout de points";
    commandDescriptions["E"] = "Mode édition de points";
    commandDescriptions["N"] = "Nouvelle courbe";
    commandDescriptions["D"] = "Supprimer la courbe courante";
    commandDescriptions["+/-"] = "Augmenter/diminuer le pas";
    commandDescriptions["1"] = "Afficher/masquer courbe (méthode directe)";
    commandDescriptions["2"] = "Afficher/masquer courbe (De Casteljau)";
    commandDescriptions["3"] = "Afficher les deux courbes";
    commandDescriptions["T"] = "Appliquer une translation";
    commandDescriptions["S"] = "Appliquer un scaling";
    commandDescriptions["R"] = "Appliquer une rotation";
    commandDescriptions["C"] = "Appliquer un cisaillement";
    commandDescriptions["Tab"] = "Passer à la courbe suivante";
    commandDescriptions["F"] = "Mode création de fenêtre de découpage";
    commandDescriptions["G"] = "Mode édition de fenêtre de découpage";
    commandDescriptions["X"] = "Activer/désactiver le découpage";
    commandDescriptions["Delete"] = "Supprimer le point sélectionné";
    commandDescriptions["Backspace"] = "Effacer la fenêtre de découpage";
}

std::string BezierApp::getModeString() const {
    switch (currentMode) {
        case Mode::ADD_CONTROL_POINTS:
            return "Ajout de points";
        case Mode::EDIT_CONTROL_POINTS:
            return "Édition de points";
        case Mode::CREATE_CLIP_WINDOW:
            return "Création de fenêtre de découpage";
        case Mode::EDIT_CLIP_WINDOW:
            return "Édition de fenêtre de découpage";
        default:
            return "Inconnu";
    }
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

        // Commencer la frame ImGui
        imguiManager.beginFrame();

        render();
        renderCursor(); // Dessiner le curseur par-dessus tout
        renderMenu();

        // Terminer la frame ImGui
        imguiManager.endFrame();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Nettoyer ImGui
    imguiManager.shutdown();
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
    }
}

void BezierApp::processInput() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // Utiliser les coordonnées mouseX et mouseY pour l'édition

    // Déplacement d'un point de contrôle sélectionné avec la souris
    if (currentMode == Mode::EDIT_CONTROL_POINTS && selectedPointIndex != -1 && selectedCurveIterator != curves.end()) {
        // Déplacer le point de contrôle sélectionné
        selectedCurveIterator->updateControlPoint(selectedPointIndex, mouseX, mouseY);
    }
    // Déplacement d'un point de la fenêtre de découpage
    else if (currentMode == Mode::EDIT_CLIP_WINDOW && selectedClipPointIndex != -1) {
        // Déplacer le point de la fenêtre sélectionné
        clipWindow[selectedClipPointIndex].x = mouseX;
        clipWindow[selectedClipPointIndex].y = mouseY;

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

    // Dessiner toutes les courbes avec ou sans découpage
    for (auto& curve : curves) {
        if (enableClipping && clipWindow.size() >= 3 && CyrusBeck::isPolygonConvex(clipWindow)) {
            curve.draw(*shader, &clipWindow);
        } else {
            curve.draw(*shader);
        }
    }

    // Dessiner les points de contrôle de la courbe sélectionnée avec mise en évidence des points survolés
    if (selectedCurveIterator != curves.end()) {
        shader->Begin();

        // Configuration pour les points
        glBindVertexArray(0); // Désactiver les VAOs précédents
        glPointSize(10.0f); // Points plus visibles

        // Dessiner chaque point individuellement pour appliquer des couleurs différentes
        for (int i = 0; i < selectedCurveIterator->getControlPointCount(); i++) {
            Point p = selectedCurveIterator->getControlPoint(i);

            // Choisir la couleur en fonction de l'état du point
            if (i == hoveredPointIndex) {
                // Point survolé: orange vif
                shader->SetUniform("color", 1.0f, 0.6f, 0.0f);
            } else if (i == selectedPointIndex) {
                // Point sélectionné: rouge
                shader->SetUniform("color", 1.0f, 0.0f, 0.0f);
            } else {
                // Point normal: rouge plus sombre
                shader->SetUniform("color", 0.8f, 0.0f, 0.0f);
            }

            // Dessiner le point
            float point[] = { p.x, p.y };
            GLuint tempVAO, tempVBO;
            glGenVertexArrays(1, &tempVAO);
            glGenBuffers(1, &tempVBO);

            glBindVertexArray(tempVAO);
            glBindBuffer(GL_ARRAY_BUFFER, tempVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(point), point, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            glDrawArrays(GL_POINTS, 0, 1);

            glDeleteVertexArrays(1, &tempVAO);
            glDeleteBuffers(1, &tempVBO);
        }

        shader->End();
    }

    // Afficher le polygone de découpage s'il existe
    if (!clipWindow.empty()) {
        shader->Begin();

        // Dessiner chaque point individuellement avec mise en évidence
        glPointSize(10.0f);
        for (int i = 0; i < clipWindow.size(); i++) {
            // Choisir la couleur en fonction de l'état du point
            if (i == hoveredClipPointIndex) {
                // Point survolé: jaune vif
                shader->SetUniform("color", 1.0f, 1.0f, 0.0f);
            } else if (i == selectedClipPointIndex) {
                // Point sélectionné: jaune plus vif
                shader->SetUniform("color", 1.0f, 1.0f, 0.4f);
            } else {
                // Point normal: jaune plus sombre
                shader->SetUniform("color", 0.7f, 0.7f, 0.0f);
            }

            // Dessiner le point
            float point[] = { clipWindow[i].x, clipWindow[i].y };
            GLuint tempVAO, tempVBO;
            glGenVertexArrays(1, &tempVAO);
            glGenBuffers(1, &tempVBO);

            glBindVertexArray(tempVAO);
            glBindBuffer(GL_ARRAY_BUFFER, tempVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(point), point, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            glDrawArrays(GL_POINTS, 0, 1);

            glDeleteVertexArrays(1, &tempVAO);
            glDeleteBuffers(1, &tempVBO);
        }

        // Dessiner les lignes de la fenêtre
        if (clipWindow.size() >= 2) {
            // Changer la couleur en fonction de la convexité
            if (CyrusBeck::isPolygonConvex(clipWindow)) {
                shader->SetUniform("color", 0.7f, 0.7f, 0.0f);  // Jaune foncé si convexe
            } else {
                shader->SetUniform("color", 1.0f, 0.0f, 0.0f);  // Rouge si non convexe
            }

            // Créer le VBO/VAO pour les lignes
            GLuint linesVAO, linesVBO;
            glGenVertexArrays(1, &linesVAO);
            glGenBuffers(1, &linesVBO);

            glBindVertexArray(linesVAO);
            glBindBuffer(GL_ARRAY_BUFFER, linesVBO);
            glBufferData(GL_ARRAY_BUFFER, clipWindow.size() * sizeof(Point), clipWindow.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
            glEnableVertexAttribArray(0);

            glDrawArrays(GL_LINE_LOOP, 0, clipWindow.size());

            glDeleteVertexArrays(1, &linesVAO);
            glDeleteBuffers(1, &linesVBO);
        }

        shader->End();
    }
}

void BezierApp::renderCursor() {
    // Vérifier si ImGui a le focus
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        // Montrer le curseur normal sur ImGui
        if (cursorMode != CursorMode::NORMAL) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            cursorMode = CursorMode::NORMAL;
        }
        return;
    } else {
        // Cacher le curseur ailleurs
        if (cursorMode != CursorMode::HIDDEN) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            cursorMode = CursorMode::HIDDEN;
        }
    }

    // Si ImGui a le focus, ne pas dessiner notre curseur
    if (cursorMode == CursorMode::NORMAL) return;

    shader->Begin();

    // Choisir la couleur en fonction de l'état
    if (isPointHovered || hoveredClipPointIndex != -1) {
        shader->SetUniform("color", 0.0f, 1.0f, 1.0f); // Cyan si un point est survolé
    } else {
        shader->SetUniform("color", 1.0f, 1.0f, 1.0f); // Blanc normalement
    }

    // Design de curseur simple en croix
    float cursorSize = 0.02f;
    float crossLines[] = {
        // Ligne horizontale
        mouseX - cursorSize, mouseY,
        mouseX + cursorSize, mouseY,
        // Ligne verticale
        mouseX, mouseY - cursorSize,
        mouseX, mouseY + cursorSize
    };

    glBindVertexArray(cursorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cursorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crossLines), crossLines, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, 4);

    glBindVertexArray(0);
    shader->End();
}

void BezierApp::renderMenu() {
    // Utiliser ImGui pour afficher les commandes
    imguiManager.renderCommandsUI(commandDescriptions, getModeString());

    // Afficher aussi des statistiques sur la courbe courante
    // Positionnement initial uniquement (FirstUseEver)
    ImGui::SetNextWindowPos(ImVec2(width - 310, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 170), ImGuiCond_FirstUseEver);

    // Aucun flag spécifique qui bloquerait le déplacement
    if (ImGui::Begin("Statistiques", nullptr)) {
        ImGui::Text("Nombre de courbes: %zu", curves.size());
        ImGui::Text("Position souris: (%.2f, %.2f)", mouseX, mouseY);
        ImGui::Text("Position écran: (%.0f, %.0f)", screenMouseX, screenMouseY);

        if (selectedCurveIterator != curves.end()) {
            ImGui::Text("Points de contrôle: %d", selectedCurveIterator->getControlPointCount());
            ImGui::Text("Pas: %.4f", selectedCurveIterator->getStep());
            ImGui::Text("Méthode directe: %s", selectedCurveIterator->isShowingDirectMethod() ? "Oui" : "Non");
            ImGui::Text("De Casteljau: %s", selectedCurveIterator->isShowingDeCasteljau() ? "Oui" : "Non");
        }

        ImGui::Text("Découpage: %s", enableClipping ? "Activé" : "Désactivé");
        ImGui::Text("Points fenêtre: %zu", clipWindow.size());

        if (clipWindow.size() >= 3) {
            ImGui::Text("Fenêtre convexe: %s",
                       CyrusBeck::isPolygonConvex(clipWindow) ? "Oui" : "Non");
        }

        // Slider pour ajuster la sensibilité de sélection des points
        ImGui::SliderFloat("Rayon de sélection", &selectionPadding, 0.01f, 0.1f, "%.2f");
    }
    ImGui::End();

    // Si c'est la première fois qu'on lance l'application, afficher une aide
    static bool showHelpOnStart = true;
    if (showHelpOnStart) {
        // Position initiale centrée
        ImGui::SetNextWindowPos(ImVec2(width / 2 - 200, height / 2 - 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);

        // Retirer les flags ImGuiWindowFlags_NoResize et ImGuiWindowFlags_NoMove pour permettre le déplacement
        if (ImGui::Begin("Bienvenue", &showHelpOnStart)) {
            ImGui::TextWrapped("Bienvenue dans l'éditeur de courbes de Bézier!");
            ImGui::TextWrapped("Utilisez la souris pour ajouter des points de contrôle.");
            ImGui::TextWrapped("Consultez le panneau 'Commandes disponibles' pour voir toutes les fonctionnalités.");
            ImGui::TextWrapped("Vous pouvez déplacer toutes les fenêtres en cliquant et glissant leur barre de titre.");

            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Commandes de base:");
            ImGui::BulletText("A/B: Ajouter des points");
            ImGui::BulletText("E: Éditer des points");
            ImGui::BulletText("1/2/3: Méthodes d'affichage");

            if (ImGui::Button("Fermer cette aide")) {
                showHelpOnStart = false;
            }
        }
        ImGui::End();
    }
}

void BezierApp::mouseButtonCallback(int button, int action, int mods) {
    // Ignorer les événements de souris si ImGui a le focus
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // Utiliser directement mouseX et mouseY qui sont constamment mis à jour
        // via le callback de position du curseur
        switch (currentMode) {
        case Mode::ADD_CONTROL_POINTS:
            if (selectedCurveIterator != curves.end()) {
                selectedCurveIterator->addControlPoint(mouseX, mouseY);
                std::cout << "Point de contrôle ajouté: (" << mouseX << ", " << mouseY << ")" << std::endl;
            }
            break;

        case Mode::EDIT_CONTROL_POINTS:
            // Si un point est déjà survolé, le sélectionner directement
            if (isPointHovered && hoveredPointIndex != -1) {
                selectedPointIndex = hoveredPointIndex;
                std::cout << "Point de contrôle sélectionné: " << selectedPointIndex << std::endl;
            } else {
                // Sélectionner le point de contrôle le plus proche avec padding
                selectNearestControlPoint(mouseX, mouseY);
            }
            break;

        case Mode::CREATE_CLIP_WINDOW:
            // Ajouter un point à la fenêtre de découpage
            clipWindow.emplace_back(mouseX, mouseY);
            std::cout << "Point de fenêtre ajouté: (" << mouseX << ", " << mouseY << ")" << std::endl;

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
            // Si un point de découpage est survolé, le sélectionner directement
            if (hoveredClipPointIndex != -1) {
                selectedClipPointIndex = hoveredClipPointIndex;
                std::cout << "Point de fenêtre sélectionné: " << selectedClipPointIndex << std::endl;
            } else {
                // Sélectionner le point de la fenêtre le plus proche avec padding
                selectNearestClipPoint(mouseX, mouseY);
            }
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
        case GLFW_KEY_A:
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

        case GLFW_KEY_F:  // Pour Window (fenêtre)
            currentMode = Mode::CREATE_CLIP_WINDOW;
            std::cout << "Mode changer en: Creation de fenetre de decoupage ACTIVE" << std::endl;
            break;

        case GLFW_KEY_G:  // Pour Fenêtre
            currentMode = Mode::EDIT_CLIP_WINDOW;
            std::cout << "Mode: Edition de la fenetre de decoupage" << std::endl;
            break;

        case GLFW_KEY_X:  // X pour activer/désactiver le découpage
            enableClipping = !enableClipping;
            std::cout << "Découpage: " << (enableClipping ? "Activé" : "Désactivé") << std::endl;
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

    // Utiliser le selectionPadding
    float minDistance = selectionPadding * 2; // Distance initiale plus grande
    selectedPointIndex = -1;

    for (int i = 0; i < selectedCurveIterator->getControlPointCount(); i++) {
        Point p = selectedCurveIterator->getControlPoint(i);
        float distance = std::sqrt((p.x - x) * (p.x - x) + (p.y - y) * (p.y - y));

        if (distance < minDistance) {
            minDistance = distance;
            selectedPointIndex = i;
        }
    }

    if (selectedPointIndex != -1) {
        std::cout << "Point de contrôle sélectionné: " << selectedPointIndex << std::endl;
    }
}

void BezierApp::selectNearestClipPoint(float x, float y) {
    if (clipWindow.empty()) {
        selectedClipPointIndex = -1;
        return;
    }

    // Utiliser le selectionPadding
    float minDistance = selectionPadding * 2; // Distance initiale plus grande
    selectedClipPointIndex = -1;

    for (int i = 0; i < clipWindow.size(); i++) {
        Point p = clipWindow[i];
        float distance = std::sqrt((p.x - x) * (p.x - x) + (p.y - y) * (p.y - y));

        if (distance < minDistance) {
            minDistance = distance;
            selectedClipPointIndex = i;
        }
    }

    if (selectedClipPointIndex != -1) {
        std::cout << "Point de fenêtre sélectionné: " << selectedClipPointIndex << std::endl;
    }
}

void BezierApp::clearClipWindow() {
    clipWindow.clear();
    selectedClipPointIndex = -1;
    hoveredClipPointIndex = -1;
    std::cout << "Fenêtre de découpage effacée" << std::endl;
}