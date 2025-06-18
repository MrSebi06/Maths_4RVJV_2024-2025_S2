#include "../include/BezierApp.h"
#include "../include/bezier/BezierCurve.h"
#include <iostream>
#include <cmath>
#include "../include/clipping/CyriusBeck.h"
#include "../include/clipping/SutherlandHodgman.h"
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <sstream>

#include "imgui.h"
#include "libs/imfilebrowser.h"

#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb_image.h"

// Définition de M_PI si nécessaire
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void BezierApp::render() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Disable depth testing for 2D
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Set projection matrix for 2D
    shader->Begin();
    glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
    GLint projLoc = glGetUniformLocation(shader->GetProgram(), "projection");
    if (projLoc != -1) {
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    }
    shader->End();

    // Draw all curves
    for (auto& curve : curves) {
        if (enableClipping && clipWindow.size() >= 3) {
            curve.draw(*shader, &clipWindow);
        } else {
            curve.draw(*shader);
        }
    }

    // Create buffers for points
    static GLuint pointVAO = 0, pointVBO = 0;
    if (pointVAO == 0) {
        glGenVertexArrays(1, &pointVAO);
        glGenBuffers(1, &pointVBO);

        glBindVertexArray(pointVAO);
        glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    shader->Begin();
    glBindVertexArray(pointVAO);

    // === DRAW CONTROL POINTS AS SMALL TRIANGLES (more visible than points) ===
    if (selectedCurveIterator != curves.end()) {
        for (int i = 0; i < selectedCurveIterator->getControlPointCount(); i++) {
            Point p = selectedCurveIterator->getControlPoint(i);

            // Set color
            if (i == hoveredPointIndex) {
                shader->SetUniform("color", 1.0f, 0.6f, 0.0f); // Orange
            } else if (i == selectedPointIndex) {
                shader->SetUniform("color", 0.0f, 1.0f, 0.0f); // Green
            } else {
                shader->SetUniform("color", 1.0f, 0.0f, 0.0f); // Red
            }

            // Create a small triangle around the point (instead of a point)
            float size = 0.02f; // Triangle size
            float triangle[] = {
                    p.x - size, p.y - size,  // Bottom left
                    p.x + size, p.y - size,  // Bottom right
                    p.x,        p.y + size   // Top
            };

            glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STREAM_DRAW);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
    }

    // Draw clipping window points as triangles too
    if (!clipWindow.empty()) {
        for (int i = 0; i < clipWindow.size(); i++) {
            if (i == hoveredClipPointIndex) {
                shader->SetUniform("color", 1.0f, 1.0f, 0.0f); // Yellow
            } else if (i == selectedClipPointIndex) {
                shader->SetUniform("color", 1.0f, 1.0f, 0.4f); // Light yellow
            } else {
                shader->SetUniform("color", 0.7f, 0.7f, 0.0f); // Dark yellow
            }

            float size = 0.015f;
            float triangle[] = {
                    clipWindow[i].x - size, clipWindow[i].y - size,
                    clipWindow[i].x + size, clipWindow[i].y - size,
                    clipWindow[i].x,        clipWindow[i].y + size
            };

            glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STREAM_DRAW);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // Draw clipping window lines
        if (clipWindow.size() >= 2) {
            static GLuint lineVAO = 0, lineVBO = 0;
            if (lineVAO == 0) {
                glGenVertexArrays(1, &lineVAO);
                glGenBuffers(1, &lineVBO);

                glBindVertexArray(lineVAO);
                glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
                glEnableVertexAttribArray(0);
            }

            if (CyrusBeck::isPolygonConvex(clipWindow)) {
                shader->SetUniform("color", 0.7f, 0.7f, 0.0f);
            } else {
                shader->SetUniform("color", 1.0f, 0.0f, 0.0f);
            }

            glBindVertexArray(lineVAO);
            glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
            glBufferData(GL_ARRAY_BUFFER, clipWindow.size() * sizeof(Point),
                         clipWindow.data(), GL_STREAM_DRAW);
            glDrawArrays(GL_LINE_LOOP, 0, clipWindow.size());
        }
    }

    glBindVertexArray(0);
    shader->End();
}

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
          // objectColor(0.8f, 0.6f, 0.4f),
          objectColor(1.f,1.f,1.f), // White for better visibility
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

    // === ADD THE DEBUG CODE HERE ===
    GLfloat pointSizeRange[2];
    glGetFloatv(GL_POINT_SIZE_RANGE, pointSizeRange);
    std::cout << "Point size range: " << pointSizeRange[0] << " to " << pointSizeRange[1] << std::endl;

    GLfloat currentPointSize;
    glGetFloatv(GL_POINT_SIZE, &currentPointSize);
    std::cout << "Current point size: " << currentPointSize << std::endl;
    // === END DEBUG CODE ===

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
    setupDefaultTexture();
    currentTexture = defaultTexture;
    selectedTextureIndex = 0;
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
    // Nettoyage des shaders
    if (shader3D) {
        delete shader3D;
        shader3D = nullptr;
    }

    if (shader) {
        delete shader;
        shader = nullptr;
    }

    // Nettoyage 3D
    currentSurface.cleanup();

    // Nettoyer les ressources du curseur
    if (cursorVAO != 0) {
        glDeleteVertexArrays(1, &cursorVAO);
        cursorVAO = 0;
    }
    if (cursorVBO != 0) {
        glDeleteBuffers(1, &cursorVBO);
        cursorVBO = 0;
    }

    // Nettoyage ImGui
    imguiManager.shutdown();

    // Nettoyage GLFW (en dernier)
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }

    glDeleteTextures(1, &defaultTexture);
    for (auto& tex : loadedTextures) {
        if (tex.second != defaultTexture) {
            glDeleteTextures(1, &tex.second);
        }
    }

    glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset) {
        BezierApp* app = static_cast<BezierApp*>(glfwGetWindowUserPointer(window));

        // Vérifier si ImGui veut capturer la molette
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) return;

        if (app->currentViewMode != ViewMode::VIEW_2D) {
            app->camera3D.processMouseScroll(static_cast<float>(yoffset));
        }
    });

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
    if (currentViewMode == ViewMode::VIEW_DUAL) {
        if (mouseX > width / 2.0) {
            oglX = 999.0f; // Invalid coordinate to indicate right half
            oglY = 999.0f;
            return;
        }

        oglX = (2.0f * static_cast<float>(mouseX) / static_cast<float>(width / 2)) - 1.0f;
        oglY = 1.0f - (2.0f * static_cast<float>(mouseY) / static_cast<float>(height));
    } else {
        oglX = (2.0f * static_cast<float>(mouseX) / static_cast<float>(width)) - 1.0f;
        oglY = 1.0f - (2.0f * static_cast<float>(mouseY) / static_cast<float>(height));
    }
}

void BezierApp::cursorPositionCallback(double xpos, double ypos) {
    if (cameraControlEnabled && currentViewMode != ViewMode::VIEW_2D) {
        if (firstMouse) {
            lastMouseX = xpos;
            lastMouseY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastMouseX;
        float yoffset = lastMouseY - ypos; // Inversé car y va de bas en haut

        lastMouseX = xpos;
        lastMouseY = ypos;

        camera3D.processMouseMovement(xoffset, yoffset);
        return; // Ne pas traiter le reste si on contrôle la caméra
    }

    // Stocker les coordonnées écran
    screenMouseX = xpos;
    screenMouseY = ypos;

    // Convertir en coordonnées OpenGL
    mouseToOpenGL(xpos, ypos, mouseX, mouseY);

    // In dual view, skip hover detection if mouse is in right half
    if (currentViewMode == ViewMode::VIEW_DUAL && mouseX > 900.0f) { // Using our invalid coordinate flag
        isPointHovered = false;
        hoveredPointIndex = -1;
        hoveredClipPointIndex = -1;
        return;
    }

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
    commandDescriptions["P/O"] = "Sauvegarder/Charger";
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
    commandDescriptions["Clic droit"] = "Activer/désactiver contrôle caméra 3D";
    commandDescriptions["WASD"] = "Déplacer la caméra (quand activée)";
    commandDescriptions["Q/E"] = "Monter/Descendre la caméra";
    commandDescriptions["Molette"] = "Zoom caméra";
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

        if (needsExtrusionUpdate) {
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
            needsExtrusionUpdate = false;
        }

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

    bool useTexture = (renderMode3D == RenderMode3D::TEXTURED_NO_LIGHTING ||
                  renderMode3D == RenderMode3D::TEXTURED_WITH_LIGHTING);
    glUniform1i(glGetUniformLocation(shader3D->GetProgram(), "useTexture"), useTexture);

    if (useTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentTexture);
        glUniform1i(glGetUniformLocation(shader3D->GetProgram(), "texture1"), 0);
    }

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
    // Clear entire screen first
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_SCISSOR_TEST);

    // === LEFT HALF: 2D View ===
    glScissor(0, 0, width / 2, height);
    glViewport(0, 0, width / 2, height);

    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Draw curves
    for (auto& curve : curves) {
        if (enableClipping && clipWindow.size() >= 3) {
            curve.draw(*shader, &clipWindow);
        } else {
            curve.draw(*shader);
        }
    }

    // Draw control points
    static GLuint pointVAO = 0, pointVBO = 0;
    if (pointVAO == 0) {
        glGenVertexArrays(1, &pointVAO);
        glGenBuffers(1, &pointVBO);
        glBindVertexArray(pointVAO);
        glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    shader->Begin();
    glBindVertexArray(pointVAO);

    if (selectedCurveIterator != curves.end()) {
        for (int i = 0; i < selectedCurveIterator->getControlPointCount(); i++) {
            Point p = selectedCurveIterator->getControlPoint(i);

            if (i == hoveredPointIndex) {
                shader->SetUniform("color", 1.0f, 0.6f, 0.0f);
            } else if (i == selectedPointIndex) {
                shader->SetUniform("color", 0.0f, 1.0f, 0.0f);
            } else {
                shader->SetUniform("color", 1.0f, 0.0f, 0.0f);
            }

            float size = 0.02f;
            float triangle[] = {
                    p.x - size, p.y - size,
                    p.x + size, p.y - size,
                    p.x,        p.y + size
            };

            glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STREAM_DRAW);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
    }

    // Draw clipping window if exists
    if (!clipWindow.empty()) {
        for (int i = 0; i < clipWindow.size(); i++) {
            if (i == hoveredClipPointIndex) {
                shader->SetUniform("color", 1.0f, 1.0f, 0.0f);
            } else if (i == selectedClipPointIndex) {
                shader->SetUniform("color", 1.0f, 1.0f, 0.4f);
            } else {
                shader->SetUniform("color", 0.7f, 0.7f, 0.0f);
            }

            float size = 0.015f;
            float triangle[] = {
                    clipWindow[i].x - size, clipWindow[i].y - size,
                    clipWindow[i].x + size, clipWindow[i].y - size,
                    clipWindow[i].x,        clipWindow[i].y + size
            };

            glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STREAM_DRAW);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
    }

    glBindVertexArray(0);
    shader->End();

    // === RIGHT HALF: 3D View ===
    glScissor(width / 2, 0, width / 2, height);
    glViewport(width / 2, 0, width / 2, height);

    // Enable depth testing for 3D
    glEnable(GL_DEPTH_TEST);

    render3D();

    // === Restore settings ===
    glDisable(GL_SCISSOR_TEST);
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

        // ADD THIS: Trigger real-time extrusion update
        if (realTimeExtrusion && surfaceGenerated && currentViewMode != ViewMode::VIEW_2D) {
            needsExtrusionUpdate = true;
        }
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
    if (currentViewMode != ViewMode::VIEW_2D && cameraControlEnabled) {
        // Déplacement WASD
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera3D.processKeyboard(0, deltaTime); // Forward
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera3D.processKeyboard(1, deltaTime); // Backward
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera3D.processKeyboard(2, deltaTime); // Left
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera3D.processKeyboard(3, deltaTime); // Right

        // Déplacement vertical
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            camera3D.position.y += camera3D.movementSpeed * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            camera3D.position.y -= camera3D.movementSpeed * deltaTime;
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

    // In dual view, don't render cursor if mouse is in right half or coordinates are invalid
    if (currentViewMode == ViewMode::VIEW_DUAL) {
        // Check if mouse is in right half using screen coordinates
        if (screenMouseX > width / 2.0) {
            return; // Don't render cursor in 3D view
        }

        // Also check if mouseX/mouseY are invalid (set by mouseToOpenGL when in right half)
        if (mouseX > 900.0f || mouseY > 900.0f) {
            return;
        }

        // Set up scissor test and viewport for left half only
        glEnable(GL_SCISSOR_TEST);
        glScissor(0, 0, width / 2, height);
        glViewport(0, 0, width / 2, height);
    }

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

    // Create/bind VAO and VBO for cursor
    static GLuint cursorVAO = 0, cursorVBO = 0;
    if (cursorVAO == 0) {
        glGenVertexArrays(1, &cursorVAO);
        glGenBuffers(1, &cursorVBO);
        glBindVertexArray(cursorVAO);
        glBindBuffer(GL_ARRAY_BUFFER, cursorVBO);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    glBindVertexArray(cursorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cursorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(crossLines), crossLines, GL_STREAM_DRAW);
    glDrawArrays(GL_LINES, 0, 4);
    glBindVertexArray(0);

    shader->End();

    // Restore viewport and disable scissor test if we enabled it
    if (currentViewMode == ViewMode::VIEW_DUAL) {
        glDisable(GL_SCISSOR_TEST);
        glViewport(0, 0, width, height);
    }
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
    renderTextureControls();

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

        ImGui::Separator();
        ImGui::Checkbox("Édition temps réel", &realTimeExtrusion);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Met à jour l'extrusion 3D en temps réel lors de l'édition des points");
        }

        ImGui::Separator();
        ImGui::Text("Contrôles caméra 3D:");
        ImGui::BulletText("Clic droit: %s", cameraControlEnabled ? "Désactiver" : "Activer");

        if (cameraControlEnabled) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Caméra active");
            ImGui::BulletText("WASD: Déplacement");
            ImGui::BulletText("Q/E: Monter/Descendre");
            ImGui::BulletText("Souris: Rotation");
            ImGui::BulletText("Molette: Zoom");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Clic droit pour activer");
        }
    }
    ImGui::End();
}

void BezierApp::generateRuledSurfaceBridge() {
    if (curves.size() < 2) return;

    auto curve1 = curves.begin();
    auto curve2 = std::next(curve1);

    curve1->calculateDirectMethod();
    curve2->calculateDirectMethod();

    std::vector<Point> points1 = curve1->getDirectMethodPoints();
    std::vector<Point> points2 = curve2->getDirectMethodPoints();

    if (points1.empty() || points2.empty()) return;

    currentSurface.vertices.clear();
    currentSurface.indices.clear();

    // Simple approach: just connect corresponding points with straight lines
    int minSize = std::min(points1.size(), points2.size());

    for (int i = 0; i < minSize; i++) {
        // Add both curve points
        Vertex3D v1, v2;
        v1.position = glm::vec3(points1[i].x, points1[i].y, 0.0f);
        v1.color = glm::vec3(1.0f, 0.0f, 0.0f); // Red for first curve

        v2.position = glm::vec3(points2[i].x, points2[i].y, extrusionHeight);
        v2.color = glm::vec3(0.0f, 0.0f, 1.0f); // Blue for second curve

        currentSurface.vertices.push_back(v1);
        currentSurface.vertices.push_back(v2);
    }

    // Create triangular strips
    for (int i = 0; i < minSize - 1; i++) {
        int base = i * 2;

        // Triangle 1
        currentSurface.indices.push_back(base);
        currentSurface.indices.push_back(base + 2);
        currentSurface.indices.push_back(base + 1);

        // Triangle 2
        currentSurface.indices.push_back(base + 1);
        currentSurface.indices.push_back(base + 2);
        currentSurface.indices.push_back(base + 3);
    }

    calculateSurfaceNormals();
    currentSurface.updateBuffers();
    surfaceGenerated = true;

    std::cout << "Ruled surface bridge created!" << std::endl;
}

// Helper method to get curve points from a specific curve
std::vector<Point> BezierApp::getCurvePointsFromCurve(const BezierCurve& curve) const {
    // Force calculation if needed
    if (curve.getDirectMethodPoints().empty()) {
        const_cast<BezierCurve&>(curve).calculateDirectMethod();
    }
    return curve.getDirectMethodPoints();
}

void BezierApp::bridgeCurvesSequentially() {
    if (curves.size() < 3) {
        std::cout << "Need at least 3 curves to bridge" << std::endl;
        return;
    }

    // Get first 3 curves
    auto it1 = curves.begin();
    auto it2 = std::next(it1);
    auto it3 = std::next(it2);

    // Connect curve1 -> curve2 with C1 continuity
    it1->joinC1(*it2);

    // Connect curve2 -> curve3 with C1 continuity
    it2->joinC1(*it3);

    std::cout << "Bridged 3 curves sequentially with C1 continuity" << std::endl;
}

void BezierApp::mouseButtonCallback(int button, int action, int mods) {
    // Ignorer les événements de souris si ImGui a le focus
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS && currentViewMode != ViewMode::VIEW_2D) {
        cameraControlEnabled = !cameraControlEnabled;

        if (cameraControlEnabled) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true;
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        }

        std::cout << "Contrôle caméra: " << (cameraControlEnabled ? "Activé" : "Désactivé") << std::endl;
        return;
    }

    if (currentViewMode == ViewMode::VIEW_DUAL && mouseX > 900.0f) {
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        switch (currentMode) {
            case Mode::ADD_CONTROL_POINTS:
                if (selectedCurveIterator != curves.end()) {
                    if (isPointHovered && hoveredPointIndex == 0) {
                        selectedCurveIterator->closeCurve();
                        std::cout << "Courbe fermée" << std::endl;
                    } else {
                        selectedCurveIterator->addControlPoint(mouseX, mouseY);
                        std::cout << "Point de contrôle ajouté: (" << mouseX << ", " << mouseY << ")" << std::endl;
                    }
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
    if (cameraControlEnabled) {
        return;
    }
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

            case GLFW_KEY_B: // B for Bridge
                generateRuledSurfaceBridge();
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
                    std::cout << "=== PRESSING KEY 1 DEBUG ===" << std::endl;
                    std::cout << "Control points: " << selectedCurveIterator->getControlPointCount() << std::endl;

                    selectedCurveIterator->toggleDirectMethod();

                    std::cout << "After toggle - showing direct: " << selectedCurveIterator->isShowingDirectMethod() << std::endl;

                    // Check if points were calculated
                    const auto& directPoints = selectedCurveIterator->getDirectMethodPoints();
                    std::cout << "Direct method points count: " << directPoints.size() << std::endl;

                    if (directPoints.size() > 0) {
                        std::cout << "First point: (" << directPoints[0].x << ", " << directPoints[0].y << ")" << std::endl;
                        std::cout << "Last point: (" << directPoints.back().x << ", " << directPoints.back().y << ")" << std::endl;
                    }
                    std::cout << "============================" << std::endl;
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
            case GLFW_KEY_P:
                saveCurvesToFile();
                break;

            case GLFW_KEY_O:
                loadCurvesFromFile();
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

    int curvePointCount = curvePoints.size();

    // Générer les vertices pour la révolution
    for (int j = 0; j <= revolutionSegments; j++) {
        float t = (float)j / revolutionSegments;
        float angle = (revolutionAngle * M_PI / 180.0f) * t;
        float cosA = cos(angle);
        float sinA = sin(angle);

        for (int i = 0; i < curvePointCount; i++) {
            Vertex3D vertex;

            // Révolution autour de l'axe Y
            // x devient le rayon, y reste la hauteur
            float radius = curvePoints[i].x;
            float height = curvePoints[i].y;

            vertex.position = glm::vec3(
                radius * cosA,     // x = r * cos(θ)
                height,            // y reste y
                radius * sinA      // z = r * sin(θ)
            );

            // Couleur basée sur la position
            vertex.color = objectColor;

            // Coordonnées de texture
            vertex.texCoord = glm::vec2(t, (float)i / (curvePointCount - 1));

            currentSurface.vertices.push_back(vertex);
        }
    }

    // Générer les indices pour les triangles
    for (int j = 0; j < revolutionSegments; j++) {
        for (int i = 0; i < curvePointCount - 1; i++) {
            int base = j * curvePointCount + i;
            int nextRing = ((j + 1) % (revolutionSegments + 1)) * curvePointCount + i;

            // Premier triangle
            currentSurface.indices.push_back(base);
            currentSurface.indices.push_back(nextRing);
            currentSurface.indices.push_back(base + 1);

            // Deuxième triangle
            currentSurface.indices.push_back(base + 1);
            currentSurface.indices.push_back(nextRing);
            currentSurface.indices.push_back(nextRing + 1);
        }
    }

    calculateSurfaceNormals();
    currentSurface.updateBuffers();
    surfaceGenerated = true;

    std::cout << "Extrusion par révolution générée: " << currentSurface.vertices.size()
              << " vertices, " << currentSurface.indices.size() / 3 << " triangles" << std::endl;
}

void BezierApp::generateGeneralizedExtrusion() {
    if (curves.size() < 2) {
        std::cout << "Besoin d'au moins 2 courbes: une forme (profil) et une trajectoire (âme)" << std::endl;
        return;
    }

    currentSurface.vertices.clear();
    currentSurface.indices.clear();

    // Première courbe = forme (profil 2D)
    // Deuxième courbe = trajectoire (âme 3D dans le plan z=0)
    auto formeCurve = curves.begin();
    auto trajectoireCurve = std::next(formeCurve);

    std::vector<Point> forme = getCurvePoints(*formeCurve);
    std::vector<Point> trajectoire = getCurvePoints(*trajectoireCurve);

    if (forme.empty() || trajectoire.empty()) {
        std::cout << "Les courbes doivent avoir des points calculés" << std::endl;
        return;
    }

    int formeSize = forme.size();
    int trajectoireSize = trajectoire.size();

    // Pour chaque point de la trajectoire
    for (int s = 0; s < trajectoireSize; s++) {
        // Point actuel sur la trajectoire (A(s))
        glm::vec3 A(trajectoire[s].x, trajectoire[s].y, 0.0f);

        // Calculer le vecteur tangent V = dA/ds
        glm::vec3 V;
        if (s == 0) {
            // Premier point: utiliser la différence avant
            glm::vec3 A_next(trajectoire[s+1].x, trajectoire[s+1].y, 0.0f);
            V = glm::normalize(A_next - A);
        } else if (s == trajectoireSize - 1) {
            // Dernier point: utiliser la différence arrière
            glm::vec3 A_prev(trajectoire[s-1].x, trajectoire[s-1].y, 0.0f);
            V = glm::normalize(A - A_prev);
        } else {
            // Points intermédiaires: utiliser la différence centrée
            glm::vec3 A_next(trajectoire[s+1].x, trajectoire[s+1].y, 0.0f);
            glm::vec3 A_prev(trajectoire[s-1].x, trajectoire[s-1].y, 0.0f);
            V = glm::normalize(A_next - A_prev);
        }

        // N est perpendiculaire au plan z=0, donc N = (0, 0, 1)
        glm::vec3 N(0.0f, 0.0f, 1.0f);

        // Calculer U = V × N
        glm::vec3 U = glm::normalize(glm::cross(V, N));

        // Recalculer N = U × V pour assurer l'orthogonalité
        N = glm::normalize(glm::cross(U, V));

        // Pour chaque point de la forme
        for (int t = 0; t < formeSize; t++) {
            Vertex3D vertex;

            // Position du point dans le repère local (U, N)
            // σ(t,s) = A(s) + x_f(t) * U + y_f(t) * N
            vertex.position = A + forme[t].x * U + forme[t].y * N;

            // Coordonnées de texture
            vertex.texCoord = glm::vec2(
                (float)t / (formeSize - 1),
                (float)s / (trajectoireSize - 1)
            );

            // Couleur
            vertex.color = objectColor;

            currentSurface.vertices.push_back(vertex);
        }
    }

    // Générer les indices pour créer les triangles
    for (int s = 0; s < trajectoireSize - 1; s++) {
        for (int t = 0; t < formeSize - 1; t++) {
            int base = s * formeSize + t;
            int next = base + formeSize;

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

    std::cout << "Extrusion généralisée créée: " << currentSurface.vertices.size()
              << " vertices, " << currentSurface.indices.size() / 3 << " triangles" << std::endl;
}

void BezierApp::saveCurvesToFile() {
    std::vector<std::vector<std::tuple<float, float>>> allCurvesData;
    for (const auto& curve : curves) {
        std::vector<std::tuple<float, float>> curveData;
        for (int i = 0; i < curve.getControlPointCount(); i++) {
            Point p = curve.getControlPoint(i);
            curveData.emplace_back(p.x, p.y);
        }
        allCurvesData.push_back(curveData);
    }
    std::ofstream
            file("curves.crv");
    if (file.is_open()) {
        for (const auto& curveData : allCurvesData) {
            for (const auto& point : curveData) {
                file << std::get<0>(point) << " " << std::get<1>(point) << "\n";
            }
            file << ";\n"; // Séparateur entre les courbes
        }
        file.close();
        std::cout << "Courbes sauvegardées dans le fichier: " << "curves.crv" << std::endl;
    } else {
        std::cerr << "Erreur lors de l'ouverture du fichier pour sauvegarde" << std::endl;
    }
}
void BezierApp::loadCurvesFromFile() {
    std::ifstream file("curves.crv");
    if (file.is_open()) {
        std::string line;
        std::vector<std::tuple<float, float>> curveData;
        curves.clear();

        while (std::getline(file, line)) {
            if (line == ";") {
                // Create a new curve and add points to it
                curves.emplace_back();  // Add a new curve to the container
                auto curveIter = --curves.end();  // Get iterator to the newly added curve

                for (const auto& point : curveData) {
                    curveIter->addControlPoint(std::get<0>(point), std::get<1>(point));
                }
                curveData.clear();
            } else {
                std::istringstream iss(line);
                float x, y;
                if (iss >> x >> y) {
                    curveData.emplace_back(x, y);
                }
            }
        }

        // Handle any remaining points (for the last curve if no ";" at the end)
        if (!curveData.empty()) {
            curves.emplace_back();  // Add a new curve
            auto curveIter = --curves.end();

            for (const auto& point : curveData) {
                curveIter->addControlPoint(std::get<0>(point), std::get<1>(point));
            }
        }

        // Set the selected curve to the first one if any were loaded
        if (!curves.empty()) {
            selectedCurveIterator = curves.begin();
        }

        file.close();
        std::cout << "Courbes chargées depuis le fichier: " << "curves.crv" << std::endl;
    } else {
        std::cerr << "Erreur lors de l'ouverture du fichier pour chargement" << std::endl;
    }
}

std::vector<Point> BezierApp::getCurvePoints(const BezierCurve& curve) const {
    std::vector<Point> points;

    int n = curve.getControlPointCount();
    std::cout << "=== getCurvePoints DEBUG ===" << std::endl;
    std::cout << "Control points count: " << n << std::endl;


    if (n < 2) {
        std::cout << "Not enough control points" << std::endl;
        return points;
    }

    // Check if methods are showing
    bool showingDirect = curve.isShowingDirectMethod();
    bool showingDeCasteljau = curve.isShowingDeCasteljau();

    std::cout << "Showing direct method: " << showingDirect << std::endl;
    std::cout << "Showing De Casteljau: " << showingDeCasteljau << std::endl;

    // Get the calculated Bézier curve points
    if (showingDirect) {
        const auto& directPoints = curve.getDirectMethodPoints();
        std::cout << "Direct method points count: " << directPoints.size() << std::endl;
        if (!directPoints.empty()) {
            points = directPoints;
            std::cout << "Using direct method points" << std::endl;
            std::cout << "=========================" << std::endl;
            return points;
        }
    }

    if (showingDeCasteljau) {
        const auto& deCasteljauPoints = curve.getDeCasteljauPoints();
        std::cout << "De Casteljau points count: " << deCasteljauPoints.size() << std::endl;
        if (!deCasteljauPoints.empty()) {
            points = deCasteljauPoints;
            std::cout << "Using De Casteljau points" << std::endl;
            std::cout << "=========================" << std::endl;
            return points;
        }
    }

    // If no curve is calculated, force calculation
    std::cout << "No curve calculated, forcing direct method calculation..." << std::endl;
    BezierCurve& nonConstCurve = const_cast<BezierCurve&>(curve);
    nonConstCurve.calculateDirectMethod();

    // Now get the calculated points
    const auto& directPoints = curve.getDirectMethodPoints();
    std::cout << "After forced calculation, points count: " << directPoints.size() << std::endl;

    if (!directPoints.empty()) {
        points = directPoints;
        std::cout << "Using forced calculated points" << std::endl;

        // Print first few points for verification
        for (size_t i = 0; i < std::min(size_t(5), points.size()); i++) {
            std::cout << "  Point " << i << ": (" << points[i].x << ", " << points[i].y << ")" << std::endl;
        }
    } else {
        std::cout << "ERROR: Still no points after forced calculation!" << std::endl;
    }

    std::cout << "=========================" << std::endl;
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

void BezierApp::setupDefaultTexture() {
    // Créer une texture procédurale en damier
    const int size = 256;
    unsigned char* data = new unsigned char[size * size * 3];

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int index = (y * size + x) * 3;
            bool isDark = ((x / 32) + (y / 32)) % 2;
            unsigned char color = isDark ? 64 : 192;
            data[index] = color;
            data[index + 1] = color;
            data[index + 2] = color;
        }
    }

    glGenTextures(1, &defaultTexture);
    glBindTexture(GL_TEXTURE_2D, defaultTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    delete[] data;

    // Ajouter à la liste
    loadedTextures.push_back({"Damier par défaut", defaultTexture});
}

#ifdef _WIN32
#include <windows.h>
#include <locale>
#include <codecvt>
#endif

GLuint BezierApp::loadTexture(const std::string& path) {
    std::cout << "Attempting to load texture: " << path << std::endl;

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Enable vertical flip for OpenGL
    stbi_set_flip_vertically_on_load(true);

    int width, height, channels;
    unsigned char* data = nullptr;

#ifdef _WIN32
    // Convert UTF-8 path to wide string for Windows
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::wstring wpath = converter.from_bytes(path);

        // Use _wfopen for Unicode support on Windows
        FILE* file = _wfopen(wpath.c_str(), L"rb");
        if (file) {
            fclose(file);
            // Convert back to system locale for stbi
            int needed = WideCharToMultiByte(CP_ACP, 0, wpath.c_str(), -1, nullptr, 0, nullptr, nullptr);
            if (needed > 0) {
                std::vector<char> buffer(needed);
                WideCharToMultiByte(CP_ACP, 0, wpath.c_str(), -1, buffer.data(), needed, nullptr, nullptr);
                data = stbi_load(buffer.data(), &width, &height, &channels, 0);
            }
        }

        if (!data) {
            // Fallback: try original path
            data = stbi_load(path.c_str(), &width, &height, &channels, 0);
        }
    } catch (const std::exception& e) {
        std::cerr << "Unicode conversion error: " << e.what() << std::endl;
        data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    }
#else
    // Unix/Linux - should handle UTF-8 natively
    data = stbi_load(path.c_str(), &width, &height, &channels, 0);
#endif

    if (data) {
        std::cout << "Texture loaded successfully: " << width << "x" << height << " with " << channels << " channels" << std::endl;

        GLenum format;
        GLenum internalFormat;

        switch (channels) {
            case 1:
                format = GL_RED;
                internalFormat = GL_R8;
                break;
            case 3:
                format = GL_RGB;
                internalFormat = GL_RGB8;
                break;
            case 4:
                format = GL_RGBA;
                internalFormat = GL_RGBA8;
                break;
            default:
                std::cerr << "Unsupported number of channels: " << channels << std::endl;
                stbi_image_free(data);
                glDeleteTextures(1, &textureID);
                return 0;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);

        // Check for OpenGL errors
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::cerr << "OpenGL error after texture creation: " << error << std::endl;
            glDeleteTextures(1, &textureID);
            return 0;
        }

        std::cout << "Texture created successfully with ID: " << textureID << std::endl;
    } else {
        std::cerr << "Failed to load texture: " << path << std::endl;
        std::cerr << "STB Error: " << stbi_failure_reason() << std::endl;

        // Additional debugging: check if file exists
#ifdef _WIN32
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::wstring wpath = converter.from_bytes(path);
        DWORD attributes = GetFileAttributesW(wpath.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES) {
            std::cerr << "File does not exist or cannot be accessed" << std::endl;
        } else {
            std::cerr << "File exists but cannot be opened by stbi_load" << std::endl;
        }
#endif

        glDeleteTextures(1, &textureID);
        return 0;
    }

    return textureID;
}

void BezierApp::renderTextureControls() {
    ImGui::SetNextWindowPos(ImVec2(width - 310, height - 200), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 180), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Contrôles de texture")) {
        // Sélection de la texture
        if (ImGui::BeginCombo("Texture", loadedTextures[selectedTextureIndex].first.c_str())) {
            for (int i = 0; i < loadedTextures.size(); i++) {
                bool isSelected = (selectedTextureIndex == i);
                if (ImGui::Selectable(loadedTextures[i].first.c_str(), isSelected)) {
                    selectedTextureIndex = i;
                    currentTexture = loadedTextures[i].second;
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        // Bouton pour charger une nouvelle texture
        if (ImGui::Button("Charger texture...")) {
            fileDialog.Open();
        }
        fileDialog.Display();
        if (fileDialog.HasSelected()) {
            std::cout << "Selected filename" << fileDialog.GetSelected().string() << std::endl;
            std::string filename = fileDialog.GetSelected().string();
            GLuint newTexture = loadTexture(filename);
            if (newTexture != 0) {
                loadedTextures.push_back({filename, newTexture});
                selectedTextureIndex = loadedTextures.size() - 1;
                currentTexture = newTexture;
                std::cout << "Texture chargée: " << filename << std::endl;
            } else {
                std::cerr << "Erreur lors du chargement de la texture: " << filename << std::endl;
            }
            fileDialog.ClearSelected();
        }

        ImGui::Text("Modes de rendu avec texture:");
        bool useTexture = (renderMode3D == RenderMode3D::TEXTURED_NO_LIGHTING ||
                          renderMode3D == RenderMode3D::TEXTURED_WITH_LIGHTING);
        if (ImGui::Checkbox("Utiliser texture", &useTexture)) {
            if (useTexture) {
                renderMode3D = RenderMode3D::TEXTURED_WITH_LIGHTING;
            } else {
                renderMode3D = RenderMode3D::SOLID_WITH_LIGHTING;
            }
        }
    }
    ImGui::End();
}
