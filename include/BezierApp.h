#ifndef BEZIER_APP_H
#define BEZIER_APP_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <list>
#include <map>
#include <string>

// === NOUVEAUX INCLUDES POUR LA 3D ===
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../include/GLShader.h"
#include "../include/BezierCurve.h"
#include "../include/ImGuiManager.h"

// === NOUVELLES STRUCTURES 3D ===
struct Vertex3D {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec3 color;

    Vertex3D() : position(0.0f), normal(0.0f, 0.0f, 1.0f),
                 texCoord(0.0f), color(1.0f) {}
    Vertex3D(glm::vec3 pos) : position(pos), normal(0.0f, 0.0f, 1.0f),
                              texCoord(0.0f), color(0.8f, 0.6f, 0.4f) {}
};

struct Surface3D {
    std::vector<Vertex3D> vertices;
    std::vector<unsigned int> indices;
    GLuint VAO, VBO, EBO;
    bool isSetup;

    Surface3D() : VAO(0), VBO(0), EBO(0), isSetup(false) {}
    ~Surface3D() { cleanup(); }

    void setupBuffers();
    void updateBuffers();
    void cleanup();
};

class Camera3D {
private:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw, pitch;
    float movementSpeed;
    float mouseSensitivity;
    float zoom;

public:
    Camera3D(glm::vec3 pos = glm::vec3(0.0f, 0.0f, 3.0f))
            : position(pos), worldUp(0.0f, 1.0f, 0.0f), yaw(-90.0f), pitch(0.0f),
              movementSpeed(2.5f), mouseSensitivity(0.1f), zoom(45.0f) {
        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() {
        return glm::lookAt(position, position + front, up);
    }

    glm::mat4 getProjectionMatrix(float aspect) {
        return glm::perspective(glm::radians(zoom), aspect, 0.1f, 100.0f);
    }

    void processKeyboard(int direction, float deltaTime) {
        float velocity = movementSpeed * deltaTime;
        switch(direction) {
            case 0: position += front * velocity; break;      // W
            case 1: position -= front * velocity; break;      // S
            case 2: position -= right * velocity; break;      // A
            case 3: position += right * velocity; break;      // D
        }
    }

    void processMouseMovement(float xoffset, float yoffset) {
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        yaw += xoffset;
        pitch += yoffset;

        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        updateCameraVectors();
    }

    void processMouseScroll(float yoffset) {
        zoom -= yoffset;
        if (zoom < 1.0f) zoom = 1.0f;
        if (zoom > 45.0f) zoom = 45.0f;
    }

    glm::vec3 getPosition() const { return position; }

private:
    void updateCameraVectors() {
        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(newFront);

        right = glm::normalize(glm::cross(front, worldUp));
        up = glm::normalize(glm::cross(right, front));
    }
};

class BezierApp {
public:
    // === ENUMS EXISTANTS ===
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

    // === NOUVEAUX ENUMS 3D ===
    enum class ViewMode {
        VIEW_2D = 0,     // Vue 2D uniquement (mode actuel)
        VIEW_3D = 1,     // Vue 3D uniquement
        VIEW_DUAL = 2    // Vue 2D + 3D côte à côte
    };

    enum class ExtrusionType {
        LINEAR = 0,      // Extrusion linéaire avec hauteur
        REVOLUTION = 1,  // Extrusion par révolution
        GENERALIZED = 2  // Extrusion le long d'une trajectoire
    };

    enum class RenderMode3D {
        WIREFRAME = 0,           // Mode filaire
        SOLID_NO_LIGHTING = 1,   // Plein sans éclairage
        SOLID_WITH_LIGHTING = 2, // Plein avec éclairage
        TEXTURED_NO_LIGHTING = 3,// Texturé sans éclairage
        TEXTURED_WITH_LIGHTING = 4 // Texturé avec éclairage
    };

    // === CONSTRUCTEUR/DESTRUCTEUR ===
    BezierApp(const char* title, int width, int height);
    virtual ~BezierApp();

    // === MÉTHODES EXISTANTES ===
    void toggleClippingAlgorithm();
    virtual void run();

private:
    // === MEMBRES EXISTANTS (inchangés) ===
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

    // === NOUVEAUX MEMBRES 3D ===
    ViewMode currentViewMode;
    RenderMode3D renderMode3D;
    ExtrusionType currentExtrusionType;

    Camera3D camera3D;
    GLShader* shader3D;

    // Paramètres d'extrusion
    float extrusionHeight;
    float extrusionScale;
    float revolutionAngle;
    int revolutionSegments;

    // Surface 3D courante
    Surface3D currentSurface;
    bool surfaceGenerated;

    // Éclairage
    glm::vec3 lightPos;
    glm::vec3 lightColor;
    glm::vec3 objectColor;

    // Gestion du temps pour l'animation
    float deltaTime;
    float lastFrame;

    // Contrôles caméra 3D
    bool cameraControlEnabled;
    bool firstMouse;
    float lastMouseX, lastMouseY;

    // === MÉTHODES EXISTANTES (inchangées) ===
    virtual void processInput();
    virtual void render();
    void renderCursor(); // Méthode pour dessiner le curseur
    virtual void renderMenu();
    void framebufferSizeCallback(int width, int height);
    void setMode(Mode newMode);

    void cursorPositionCallback(double xpos, double ypos); // Callback pour la position du curseur
    virtual void mouseButtonCallback(int button, int action, int mods);
    virtual void keyCallback(int key, int scancode, int action, int mods);

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

    // === NOUVELLES MÉTHODES 3D ===
    void setupShaders3D();
    void render3D();
    void renderDual();
    void processCamera3D();

    // Méthodes d'extrusion
    void generateLinearExtrusion();
    void generateRevolutionExtrusion();
    void generateGeneralizedExtrusion();
    void calculateSurfaceNormals();
    std::vector<Point> getCurvePoints(const BezierCurve& curve) const;

    // Interface utilisateur 3D
    void renderExtrusionControls();
    void renderCameraControls();
    void renderLightingControls();
    void render3DViewport(float x, float y, float width, float height);
    void render2DViewport(float x, float y, float width, float height);

    // Gestion des modes
    void setViewMode(ViewMode mode);
    void setRenderMode3D(RenderMode3D mode);
    void setExtrusionType(ExtrusionType type);

    // Utilitaires 3D
    std::string getViewModeString() const;
    std::string getRenderMode3DString() const;
    std::string getExtrusionTypeString() const;
};

// === IMPLÉMENTATIONS INLINE POUR LES UTILITAIRES ===
inline void Surface3D::setupBuffers() {
    if (!isSetup) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        isSetup = true;
    }
}

inline void Surface3D::updateBuffers() {
    if (!isSetup) setupBuffers();

    glBindVertexArray(VAO);

    // Buffer des vertices
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex3D),
                 vertices.data(), GL_STATIC_DRAW);

    // Buffer des indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);

    // Attribut 0: Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)0);
    glEnableVertexAttribArray(0);

    // Attribut 1: Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                          (void*)offsetof(Vertex3D, normal));
    glEnableVertexAttribArray(1);

    // Attribut 2: Coordonnées de texture
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                          (void*)offsetof(Vertex3D, texCoord));
    glEnableVertexAttribArray(2);

    // Attribut 3: Couleur
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                          (void*)offsetof(Vertex3D, color));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
}

inline void Surface3D::cleanup() {
    if (isSetup) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        isSetup = false;
    }
}

#endif // BEZIER_APP_H