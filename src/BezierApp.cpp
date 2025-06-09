#include "../include/BezierApp.h"
#include "../include/BezierCurve.h"
#include <iostream>
#include <cmath>
#include "../include/CyriusBeck.h"
#include "../include/SutherlandHodgman.h"
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <sstream>

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
          hoveredClipPointIndex(-1), cursorMode(CursorMode::HIDDEN),
          usesSutherlandHodgman(false),
        // === NOUVELLES INITIALISATIONS 3D ===
          currentViewMode(ViewMode::VIEW_2D),
          renderMode3D(RenderMode3D::SOLID_WITH_LIGHTING),
          currentExtrusionType(ExtrusionType::LINEAR),
          camera3D(glm::vec3(0.0f, 0.0f, 3.0f)),
          shader3D(nullptr),
          extrusionHeight(1.0f),
          extrusionScale(1.0f),
          revolutionAngle(360.0f),
          revolutionSegments(20),
          surfaceGenerated(false),
          lightPos(1.2f, 1.0f, 2.0f),
          lightColor(1.0f, 1.0f, 1.0f),
          objectColor(0.8f, 0.6f, 0.4f),
          deltaTime(0.0f),
          lastFrame(0.0f),
          cameraControlEnabled(false),
          firstMouse(true),
          lastMouseX(width / 2.0f),
          lastMouseY(height / 2.0f) {

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


    // Création du shader 2D
    shader = new GLShader();
    bool vsOk = shader->LoadVertexShader("../shader/basic.vs.glsl");
    bool fsOk = shader->LoadFragmentShader("../shader/basic.fs.glsl");
    bool createOk = shader->Create();

    // === AJOUT DES LIGNES DE DEBUG ICI ===
    std::cout << "=== DEBUG SHADERS 2D ===" << std::endl;
    std::cout << "Vertex Shader: " << (vsOk ? "OK" : "ERREUR") << std::endl;
    std::cout << "Fragment Shader: " << (fsOk ? "OK" : "ERREUR") << std::endl;
    std::cout << "Création Shader: " << (createOk ? "OK" : "ERREUR") << std::endl;
    std::cout << "Pointeur shader: " << (shader ? "Valide" : "NULL") << std::endl;
    std::cout << "=========================" << std::endl;

    if (!vsOk || !fsOk || !createOk) {
        std::cerr << "Erreur lors du chargement des shaders 2D!" << std::endl;
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

    // === CONFIGURATION 3D ===
    glEnable(GL_DEPTH_TEST);
    setupShaders3D();
    currentSurface.setupBuffers();

    // Ajouter les nouvelles commandes 3D
    commandDescriptions["V"] = "Basculer mode vue (2D/3D/Dual)";
    commandDescriptions["L"] = "Extrusion linéaire";
    commandDescriptions["M"] = "Changer mode de rendu 3D";
    commandDescriptions["H"] = "Augmenter hauteur d'extrusion";
    commandDescriptions["U"] = "Diminuer hauteur d'extrusion";

    // === DEBUG FINAL ===
    std::cout << "Constructeur BezierApp terminé avec succès" << std::endl;
}

BezierApp::~BezierApp() {
    // Nettoyage 3D
    delete shader3D;
    currentSurface.cleanup();

    // Nettoyer les ressources du curseur
    glDeleteVertexArrays(1, &cursorVAO);
    glDeleteBuffers(1, &cursorVBO);

    delete shader;
    glfwDestroyWindow(window);
    glfwTerminate();
}

// Implémentez la méthode pour basculer entre les algorithmes
void BezierApp::toggleClippingAlgorithm() {
    usesSutherlandHodgman = !usesSutherlandHodgman;

    // Mettre à jour l'algorithme pour toutes les courbes
    for (auto& curve : curves) {
        curve.setClippingAlgorithm(usesSutherlandHodgman ?
                                   BezierCurve::ClippingAlgorithm::SUTHERLAND_HODGMAN :
                                   BezierCurve::ClippingAlgorithm::CYRUS_BECK);
    }

    std::cout << "Algorithme de découpage changé pour: " <<
              (usesSutherlandHodgman ? "Sutherland-Hodgman" : "Cyrus-Beck") << std::endl;
}

void BezierApp::setupShaders3D() {
    std::cout << "=== DEBUG SHADERS 3D DÉTAILLÉ ===" << std::endl;

    // Vérifier les fichiers
    std::cout << "VS file exists: " << std::filesystem::exists("../shader/surface3d.vs.glsl") << std::endl;
    std::cout << "FS file exists: " << std::filesystem::exists("../shader/surface3d.fs.glsl") << std::endl;

    shader3D = new GLShader();

    // Test vertex shader
    std::cout << "Loading vertex shader..." << std::endl;
    bool vsOk = shader3D->LoadVertexShader("../shader/surface3d.vs.glsl");
    std::cout << "Vertex shader result: " << (vsOk ? "OK" : "FAILED") << std::endl;

    // Test fragment shader seulement si vertex OK
    bool fsOk = false;
    if (vsOk) {
        std::cout << "Loading fragment shader..." << std::endl;
        fsOk = shader3D->LoadFragmentShader("../shader/surface3d.fs.glsl");
        std::cout << "Fragment shader result: " << (fsOk ? "OK" : "FAILED") << std::endl;
    }

    // Test création seulement si les deux OK
    bool createOk = false;
    if (vsOk && fsOk) {
        std::cout << "Creating shader program..." << std::endl;
        createOk = shader3D->Create();
        std::cout << "Program creation result: " << (createOk ? "OK" : "FAILED") << std::endl;
    }

    std::cout << "=================================" << std::endl;

    if (!vsOk || !fsOk || !createOk) {
        std::cerr << "Avertissement: Shaders 3D non trouvés - Mode 3D désactivé" << std::endl;
        delete shader3D;
        shader3D = nullptr;
    } else {
        std::cout << "🎉 Shaders 3D chargés avec succès!" << std::endl;
    }
}

void BezierApp::setupCursorBuffers() {
    // Création du VAO/VBO pour le curseur personnalisé (croix)
    glGenVertexArrays(1, &cursorVAO);
    glGenBuffers(1, &cursorVBO);

    // Les coordonnées pour dessiner une croix (elles seront mises à jour dynamiquement)
    float cursorVertices[8] = {0}; // Initialiser avec des zéros

    glBindVertexArray(cursorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cursorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cursorVertices), cursorVertices, GL_DYNAMIC_DRAW);
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
    commandDescriptions["Z"] = "Basculer entre Cyrus-Beck et Sutherland-Hodgman";
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

std::string BezierApp::getViewModeString() const {
    switch (currentViewMode) {
        case ViewMode::VIEW_2D: return "2D";
        case ViewMode::VIEW_3D: return "3D";
        case ViewMode::VIEW_DUAL: return "Dual";
        default: return "Inconnu";
    }
}

std::string BezierApp::getRenderMode3DString() const {
    switch (renderMode3D) {
        case RenderMode3D::WIREFRAME: return "Filaire";
        case RenderMode3D::SOLID_NO_LIGHTING: return "Plein sans éclairage";
        case RenderMode3D::SOLID_WITH_LIGHTING: return "Plein avec éclairage";
        default: return "Inconnu";
    }
}

std::string BezierApp::getExtrusionTypeString() const {
    switch (currentExtrusionType) {
        case ExtrusionType::LINEAR: return "Linéaire";
        case ExtrusionType::REVOLUTION: return "Révolution";
        case ExtrusionType::GENERALIZED: return "Généralisée";
        default: return "Inconnu";
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
    glPointSize(5.0f);

    while (!glfwWindowShouldClose(window)) {
        // Calcul du temps
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput();

        // Commencer la frame ImGui
        imguiManager.beginFrame();

        // Rendu selon le mode actuel
        switch (currentViewMode) {
            case ViewMode::VIEW_2D:
                render(); // Rendu 2D original
                break;
            case ViewMode::VIEW_3D:
                render3D();
                break;
            case ViewMode::VIEW_DUAL:
                renderDual();
                break;
        }

        renderCursor();
        renderMenu();

        // Terminer la frame ImGui
        imguiManager.endFrame();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Nettoyer ImGui
    imguiManager.shutdown();
}

void BezierApp::render3D() {
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!surfaceGenerated || !shader3D) {
        // Afficher un message d'aide en 3D
        return;
    }

    shader3D->Begin();

    // Matrices de transformation
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera3D.getViewMatrix();
    glm::mat4 projection = camera3D.getProjectionMatrix((float)width / (float)height);
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

    // Uniformes
    GLuint modelLoc = glGetUniformLocation(shader3D->GetProgram(), "model");
    GLuint viewLoc = glGetUniformLocation(shader3D->GetProgram(), "view");
    GLuint projLoc = glGetUniformLocation(shader3D->GetProgram(), "projection");
    GLuint normalLoc = glGetUniformLocation(shader3D->GetProgram(), "normalMatrix");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix3fv(normalLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    // Éclairage
    glUniform3fv(glGetUniformLocation(shader3D->GetProgram(), "lightPos"), 1, glm::value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(shader3D->GetProgram(), "lightColor"), 1, glm::value_ptr(lightColor));
    glUniform3fv(glGetUniformLocation(shader3D->GetProgram(), "objectColor"), 1, glm::value_ptr(objectColor));
    glUniform3fv(glGetUniformLocation(shader3D->GetProgram(), "viewPos"), 1, glm::value_ptr(camera3D.getPosition()));

    bool useLighting = (renderMode3D == RenderMode3D::SOLID_WITH_LIGHTING);
    glUniform1i(glGetUniformLocation(shader3D->GetProgram(), "useLighting"), useLighting);

    // Mode de rendu
    switch (renderMode3D) {
        case RenderMode3D::WIREFRAME:
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;
        default:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
    }

    // Dessiner la surface
    glBindVertexArray(currentSurface.VAO);
    glDrawElements(GL_TRIANGLES, currentSurface.indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    shader3D->End();
}

void BezierApp::renderDual() {
    // Vue dual : 2D à gauche, 3D à droite
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Vue 2D (moitié gauche)
    glViewport(0, 0, width / 2, height);
    render(); // Votre méthode render() 2D existante

    // Vue 3D (moitié droite)
    glViewport(width / 2, 0, width / 2, height);
    render3D();

    // Restaurer le viewport complet pour l'interface
    glViewport(0, 0, width, height);
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

    if (newMode == Mode::CREATE_CLIP_WINDOW) {
        std::cout << "Passé en mode création de fenêtre de découpage" << std::endl;
    }
}

void BezierApp::processInput() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // Utiliser les coordonnées mouseX et mouseY pour l'édition
    if (currentMode == Mode::EDIT_CONTROL_POINTS && selectedPointIndex != -1 && selectedCurveIterator != curves.end()) {
        selectedCurveIterator->updateControlPoint(selectedPointIndex, mouseX, mouseY);
    }
    else if (currentMode == Mode::EDIT_CLIP_WINDOW && selectedClipPointIndex != -1) {
        clipWindow[selectedClipPointIndex].x = mouseX;
        clipWindow[selectedClipPointIndex].y = mouseY;

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
        if (enableClipping && clipWindow.size() >= 3) {
            curve.draw(*shader, &clipWindow);
        } else {
            curve.draw(*shader);
        }
    }

    // Dessiner les points de contrôle de la courbe sélectionnée
    if (selectedCurveIterator != curves.end()) {
        shader->Begin();
        glBindVertexArray(0);
        glPointSize(10.0f);

        for (int i = 0; i < selectedCurveIterator->getControlPointCount(); i++) {
            Point p = selectedCurveIterator->getControlPoint(i);

            if (i == hoveredPointIndex) {
                shader->SetUniform("color", 1.0f, 0.6f, 0.0f);
            } else if (i == selectedPointIndex) {
                shader->SetUniform("color", 1.0f, 0.0f, 0.0f);
            } else {
                shader->SetUniform("color", 0.8f, 0.0f, 0.0f);
            }

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
        glPointSize(10.0f);

        for (int i = 0; i < clipWindow.size(); i++) {
            if (i == hoveredClipPointIndex) {
                shader->SetUniform("color", 1.0f, 1.0f, 0.0f);
            } else if (i == selectedClipPointIndex) {
                shader->SetUniform("color", 1.0f, 1.0f, 0.4f);
            } else {
                shader->SetUniform("color", 0.7f, 0.7f, 0.0f);
            }

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
            if (CyrusBeck::isPolygonConvex(clipWindow)) {
                shader->SetUniform("color", 0.7f, 0.7f, 0.0f);
            } else {
                shader->SetUniform("color", 1.0f, 0.0f, 0.0f);
            }

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
        if (cursorMode != CursorMode::NORMAL) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            cursorMode = CursorMode::NORMAL;
        }
        return;
    } else {
        if (cursorMode != CursorMode::HIDDEN) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            cursorMode = CursorMode::HIDDEN;
        }
    }

    if (cursorMode == CursorMode::NORMAL) return;

    shader->Begin();

    if (isPointHovered || hoveredClipPointIndex != -1) {
        shader->SetUniform("color", 0.0f, 1.0f, 1.0f);
    } else {
        shader->SetUniform("color", 1.0f, 1.0f, 1.0f);
    }

    float cursorSize = 0.02f;
    float crossLines[] = {
            mouseX - cursorSize, mouseY,
            mouseX + cursorSize, mouseY,
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
    ImGui::SetNextWindowPos(ImVec2(width - 310, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Statistiques", nullptr)) {
        ImGui::Text("Nombre de courbes: %zu", curves.size());
        ImGui::Text("Position souris: (%.2f, %.2f)", mouseX, mouseY);
        ImGui::Text("Position écran: (%.0f, %.0f)", screenMouseX, screenMouseY);
        ImGui::Text("Mode de vue: %s", getViewModeString().c_str());
        ImGui::Text("Découpage: %s", enableClipping ? "Activé" : "Désactivé");
        ImGui::Text("Algorithme: %s", usesSutherlandHodgman ? "Sutherland-Hodgman" : "Cyrus-Beck");
        ImGui::Text("Points fenêtre: %zu", clipWindow.size());

        if (selectedCurveIterator != curves.end()) {
            ImGui::Text("Points de contrôle: %d", selectedCurveIterator->getControlPointCount());
            ImGui::Text("Pas: %.4f", selectedCurveIterator->getStep());
            ImGui::Text("Méthode directe: %s", selectedCurveIterator->isShowingDirectMethod() ? "Oui" : "Non");
            ImGui::Text("De Casteljau: %s", selectedCurveIterator->isShowingDeCasteljau() ? "Oui" : "Non");
        }

        if (clipWindow.size() >= 3) {
            ImGui::Text("Fenêtre convexe: %s",
                        CyrusBeck::isPolygonConvex(clipWindow) ? "Oui" : "Non");
        }

        // Slider pour ajuster la sensibilité de sélection des points
        ImGui::SliderFloat("Rayon de sélection", &selectionPadding, 0.01f, 0.1f, "%.2f");
    }
    ImGui::End();

    // === NOUVEAU PANNEAU 3D ===
    renderExtrusionControls();

    // Si c'est la première fois qu'on lance l'application, afficher une aide
    static bool showHelpOnStart = true;
    if (showHelpOnStart) {
        ImGui::SetNextWindowPos(ImVec2(width / 2 - 200, height / 2 - 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Bienvenue", &showHelpOnStart)) {
            ImGui::TextWrapped("Bienvenue dans l'éditeur de courbes de Bézier 2D/3D!");
            ImGui::TextWrapped("Utilisez la souris pour ajouter des points de contrôle.");
            ImGui::TextWrapped("Consultez le panneau 'Commandes disponibles' pour voir toutes les fonctionnalités.");

            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Commandes de base:");
            ImGui::BulletText("A/B: Ajouter des points");
            ImGui::BulletText("E: Éditer des points");
            ImGui::BulletText("1/2/3: Méthodes d'affichage");

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Nouvelles commandes 3D:");
            ImGui::BulletText("V: Basculer vue 2D/3D/Dual");
            ImGui::BulletText("L: Extrusion linéaire");
            ImGui::BulletText("H/U: Hauteur d'extrusion");

            if (ImGui::Button("Fermer cette aide")) {
                showHelpOnStart = false;
            }
        }
        ImGui::End();
    }
}

void BezierApp::renderExtrusionControls() {
    ImGui::SetNextWindowPos(ImVec2(10, height - 220), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Contrôles 3D")) {
        // Mode de vue
        const char* viewModes[] = { "2D", "3D", "Dual" };
        int currentView = static_cast<int>(currentViewMode);
        if (ImGui::Combo("Mode de vue", &currentView, viewModes, 3)) {
            currentViewMode = static_cast<ViewMode>(currentView);
        }

        // Type d'extrusion
        const char* extrusionTypes[] = { "Linéaire", "Révolution", "Généralisée" };
        int currentExtrusion = static_cast<int>(currentExtrusionType);
        if (ImGui::Combo("Type d'extrusion", &currentExtrusion, extrusionTypes, 3)) {
            currentExtrusionType = static_cast<ExtrusionType>(currentExtrusion);
        }

        // Mode de rendu 3D
        const char* renderModes[] = { "Filaire", "Plein sans éclairage", "Plein avec éclairage" };
        int currentRender = static_cast<int>(renderMode3D);
        if (ImGui::Combo("Mode de rendu", &currentRender, renderModes, 3)) {
            renderMode3D = static_cast<RenderMode3D>(currentRender);
        }

        // Paramètres d'extrusion
        if (ImGui::SliderFloat("Hauteur", &extrusionHeight, 0.1f, 5.0f)) {
            if (currentExtrusionType == ExtrusionType::LINEAR && surfaceGenerated) {
                generateLinearExtrusion();
            }
        }

        if (ImGui::SliderFloat("Facteur d'échelle", &extrusionScale, 0.1f, 3.0f)) {
            if (currentExtrusionType == ExtrusionType::LINEAR && surfaceGenerated) {
                generateLinearExtrusion();
            }
        }

        // Boutons d'action
        if (ImGui::Button("Générer extrusion")) {
            switch (currentExtrusionType) {
                case ExtrusionType::LINEAR:
                    generateLinearExtrusion();
                    break;
                case ExtrusionType::REVOLUTION:
                    generateRevolutionExtrusion();
                    break;
                case ExtrusionType::GENERALIZED:
                    generateGeneralizedExtrusion();
                    break;
            }
        }

        ImGui::Text("Surface: %s", surfaceGenerated ? "Générée" : "Non générée");
        if (surfaceGenerated) {
            ImGui::Text("Vertices: %zu", currentSurface.vertices.size());
            ImGui::Text("Triangles: %zu", currentSurface.indices.size() / 3);
        }

        // État des shaders 3D
        ImGui::Text("Shaders 3D: %s", shader3D ? "OK" : "Erreur");
    }
    ImGui::End();
}

void BezierApp::mouseButtonCallback(int button, int action, int mods) {
    // Ignorer les événements de souris si ImGui a le focus
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        switch (currentMode) {
            case Mode::ADD_CONTROL_POINTS:
                if (selectedCurveIterator != curves.end()) {
                    selectedCurveIterator->addControlPoint(mouseX, mouseY);
                    std::cout << "Point de contrôle ajouté: (" << mouseX << ", " << mouseY << ")" << std::endl;
                }
                break;

            case Mode::EDIT_CONTROL_POINTS:
                if (isPointHovered && hoveredPointIndex != -1) {
                    selectedPointIndex = hoveredPointIndex;
                    std::cout << "Point de contrôle sélectionné: " << selectedPointIndex << std::endl;
                } else {
                    selectNearestControlPoint(mouseX, mouseY);
                }
                break;

            case Mode::CREATE_CLIP_WINDOW:
                clipWindow.emplace_back(mouseX, mouseY);
                std::cout << "Point de fenêtre ajouté: (" << mouseX << ", " << mouseY << ")" << std::endl;

                if (clipWindow.size() >= 3) {
                    if (CyrusBeck::isPolygonConvex(clipWindow)) {
                        std::cout << "La fenêtre est convexe." << std::endl;
                    } else {
                        std::cout << "Attention: La fenêtre n'est pas convexe!" << std::endl;
                    }
                }
                break;

            case Mode::EDIT_CLIP_WINDOW:
                if (hoveredClipPointIndex != -1) {
                    selectedClipPointIndex = hoveredClipPointIndex;
                    std::cout << "Point de fenêtre sélectionné: " << selectedClipPointIndex << std::endl;
                } else {
                    selectNearestClipPoint(mouseX, mouseY);
                }
                break;
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
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
        switch (key) {
            // === NOUVELLES TOUCHES 3D ===
            case GLFW_KEY_V:
                currentViewMode = static_cast<ViewMode>((static_cast<int>(currentViewMode) + 1) % 3);
                std::cout << "Mode de vue: " << getViewModeString() << std::endl;
                break;

            case GLFW_KEY_L:
                currentExtrusionType = ExtrusionType::LINEAR;
                generateLinearExtrusion();
                break;

            case GLFW_KEY_M:
                renderMode3D = static_cast<RenderMode3D>((static_cast<int>(renderMode3D) + 1) % 3);
                std::cout << "Mode de rendu 3D: " << getRenderMode3DString() << std::endl;
                break;

            case GLFW_KEY_H:
                extrusionHeight += 0.2f;
                std::cout << "Hauteur d'extrusion: " << extrusionHeight << std::endl;
                if (surfaceGenerated) generateLinearExtrusion();
                break;

            case GLFW_KEY_U:
                extrusionHeight = std::max(0.1f, extrusionHeight - 0.2f);
                std::cout << "Hauteur d'extrusion: " << extrusionHeight << std::endl;
                if (surfaceGenerated) generateLinearExtrusion();
                break;

                // === TOUCHES EXISTANTES ===
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
                if (selectedCurveIterator != curves.end()) {
                    float dx = 0.1f;
                    float dy = 0.1f;
                    selectedCurveIterator->translate(dx, dy);
                }
                break;

            case GLFW_KEY_S:
                if (selectedCurveIterator != curves.end()) {
                    float sx = 1.1f;
                    float sy = 1.1f;
                    selectedCurveIterator->scale(sx, sy);
                }
                break;

            case GLFW_KEY_R:
                if (selectedCurveIterator != curves.end()) {
                    float angle = 15.0f;
                    selectedCurveIterator->rotate(angle);
                }
                break;

            case GLFW_KEY_F:
                currentMode = Mode::CREATE_CLIP_WINDOW;
                std::cout << "Mode: Création de fenêtre de découpage" << std::endl;
                break;

            case GLFW_KEY_G:
                currentMode = Mode::EDIT_CLIP_WINDOW;
                std::cout << "Mode: Édition de la fenêtre de découpage" << std::endl;
                break;

            case GLFW_KEY_X:
                enableClipping = !enableClipping;
                std::cout << "Découpage: " << (enableClipping ? "Activé" : "Désactivé") << std::endl;
                break;

            case GLFW_KEY_Z:
                toggleClippingAlgorithm();
                break;

            case GLFW_KEY_DELETE:
                if (currentMode == Mode::EDIT_CLIP_WINDOW && selectedClipPointIndex != -1) {
                    clipWindow.erase(clipWindow.begin() + selectedClipPointIndex);
                    selectedClipPointIndex = -1;
                    std::cout << "Point de fenêtre supprimé" << std::endl;
                } else if (currentMode == Mode::EDIT_CONTROL_POINTS && selectedPointIndex != -1 && selectedCurveIterator != curves.end()) {
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

void BezierApp::generateLinearExtrusion() {
    if (selectedCurveIterator == curves.end() ||
        selectedCurveIterator->getControlPointCount() < 2) {
        std::cout << "Pas assez de points de contrôle pour l'extrusion" << std::endl;
        return;
    }

    currentSurface.vertices.clear();
    currentSurface.indices.clear();

    // Obtenir les points de la courbe 2D
    std::vector<Point> curvePoints = getCurvePoints(*selectedCurveIterator);

    if (curvePoints.empty()) {
        std::cout << "Aucun point de courbe calculé" << std::endl;
        return;
    }

    // Générer les vertices pour l'extrusion linéaire
    int curvePointCount = curvePoints.size();
    int heightSteps = 20;

    for (int h = 0; h <= heightSteps; h++) {
        float t = (float)h / heightSteps;
        float z = extrusionHeight * t;
        float scale = 1.0f + (extrusionScale - 1.0f) * t;

        for (int i = 0; i < curvePointCount; i++) {
            Vertex3D vertex;
            vertex.position = glm::vec3(
                    curvePoints[i].x * scale,
                    curvePoints[i].y * scale,
                    z
            );
            vertex.color = objectColor;
            vertex.texCoord = glm::vec2((float)i / curvePointCount, t);

            currentSurface.vertices.push_back(vertex);
        }
    }

    // Générer les indices pour les triangles
    for (int h = 0; h < heightSteps; h++) {
        for (int i = 0; i < curvePointCount - 1; i++) {
            int base = h * curvePointCount + i;
            int next = base + curvePointCount;

            // Premier triangle
            currentSurface.indices.push_back(base);
            currentSurface.indices.push_back(next);
            currentSurface.indices.push_back(base + 1);

            // Deuxième triangle
            currentSurface.indices.push_back(base + 1);
            currentSurface.indices.push_back(next);
            currentSurface.indices.push_back(next + 1);
        }
    }

    calculateSurfaceNormals();
    currentSurface.updateBuffers();
    surfaceGenerated = true;

    std::cout << "Extrusion linéaire générée: " << currentSurface.vertices.size()
              << " vertices, " << currentSurface.indices.size() / 3 << " triangles" << std::endl;
}

void BezierApp::generateRevolutionExtrusion() {
    std::cout << "Extrusion par révolution - En développement" << std::endl;
}

void BezierApp::generateGeneralizedExtrusion() {
    std::cout << "Extrusion généralisée - En développement" << std::endl;
}

std::vector<Point> BezierApp::getCurvePoints(const BezierCurve& curve) const {
    std::vector<Point> points;

    // Utiliser les points de contrôle pour créer une courbe simple
    int n = curve.getControlPointCount();
    if (n < 2) return points;

    // Pour l'instant, utilisons juste les points de contrôle
    // Plus tard, nous utiliserons les points calculés par Bézier
    for (int i = 0; i < n; i++) {
        points.push_back(curve.getControlPoint(i));
    }

    return points;
}

void BezierApp::calculateSurfaceNormals() {
    // Initialiser toutes les normales à zéro
    for (auto& vertex : currentSurface.vertices) {
        vertex.normal = glm::vec3(0.0f);
    }

    // Calculer les normales de face et les accumuler
    for (size_t i = 0; i < currentSurface.indices.size(); i += 3) {
        unsigned int i0 = currentSurface.indices[i];
        unsigned int i1 = currentSurface.indices[i + 1];
        unsigned int i2 = currentSurface.indices[i + 2];

        glm::vec3 v0 = currentSurface.vertices[i0].position;
        glm::vec3 v1 = currentSurface.vertices[i1].position;
        glm::vec3 v2 = currentSurface.vertices[i2].position;

        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

        currentSurface.vertices[i0].normal += normal;
        currentSurface.vertices[i1].normal += normal;
        currentSurface.vertices[i2].normal += normal;
    }

    // Normaliser toutes les normales
    for (auto& vertex : currentSurface.vertices) {
        if (glm::length(vertex.normal) > 0.0f) {
            vertex.normal = glm::normalize(vertex.normal);
        }
    }
}

void BezierApp::createNewCurve() {
    curves.emplace_back();

    curves.back().setClippingAlgorithm(usesSutherlandHodgman ?
                                       BezierCurve::ClippingAlgorithm::SUTHERLAND_HODGMAN :
                                       BezierCurve::ClippingAlgorithm::CYRUS_BECK);

    selectedCurveIterator = std::prev(curves.end());
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

    float minDistance = selectionPadding * 2;
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

    float minDistance = selectionPadding * 2;
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