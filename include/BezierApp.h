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

// MOVE TriangularSurface inside BezierApp class - remove from here

class Camera3D {
    // ... existing Camera3D implementation stays the same ...
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
    // === ENUMS (unchanged) ===
    enum class Mode {
        ADD_CONTROL_POINTS = 0,
        EDIT_CONTROL_POINTS = 1,
        CREATE_CLIP_WINDOW = 2,
        EDIT_CLIP_WINDOW = 3
    };

    enum class CursorMode {
        NORMAL = 0,
        HIDDEN = 1,
        DISABLED = 2
    };

    enum class ViewMode {
        VIEW_2D = 0,
        VIEW_3D = 1,
        VIEW_DUAL = 2
    };

    enum class ExtrusionType {
        LINEAR = 0,
        REVOLUTION = 1,
        GENERALIZED = 2
    };

    enum class RenderMode3D {
        WIREFRAME = 0,
        SOLID_NO_LIGHTING = 1,
        SOLID_WITH_LIGHTING = 2,
        TEXTURED_NO_LIGHTING = 3,
        TEXTURED_WITH_LIGHTING = 4
    };

    // === CONSTRUCTOR/DESTRUCTOR ===
    BezierApp(const char* title, int width, int height);
    virtual ~BezierApp();

    // === PUBLIC METHODS ===
    void toggleClippingAlgorithm();
    virtual void run();

private:
    // === MOVE TriangularSurface HERE ===
    struct TriangularSurface {
        std::vector<Vertex3D> vertices;
        std::vector<unsigned int> indices;
        GLuint VAO = 0, VBO = 0, EBO = 0;

        void generateTriangularPatch(const BezierCurve& curve1,
                                     const BezierCurve& curve2,
                                     const BezierCurve& curve3);
        void setupBuffers();
        void cleanup();
        Point interpolateCurve(const std::vector<Point>& curve, float t);
    };

    // === EXISTING MEMBERS ===
    int width, height;
    GLFWwindow* window;
    GLShader* shader;

    ViewMode currentViewMode = ViewMode::VIEW_2D;
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    // Cursor
    float mouseX, mouseY;
    double screenMouseX, screenMouseY;
    GLuint cursorVAO, cursorVBO;
    bool isPointHovered;
    int hoveredPointIndex;
    CursorMode cursorMode;

    std::list<BezierCurve> curves;
    std::list<BezierCurve>::iterator selectedCurveIterator;

    Mode currentMode;
    int selectedPointIndex;
    bool menuNeedsUpdate = true;

    // Clipping
    std::vector<Point> clipWindow;
    int selectedClipPointIndex = -1;
    bool enableClipping = false;
    int hoveredClipPointIndex = -1;

    ImGuiManager imguiManager;
    float selectionPadding = 0.03f;
    std::map<std::string, std::string> commandDescriptions;
    bool usesSutherlandHodgman;

    // === 3D MEMBERS ===
    RenderMode3D renderMode3D = RenderMode3D::SOLID_WITH_LIGHTING;
    ExtrusionType currentExtrusionType = ExtrusionType::LINEAR;

    Camera3D camera3D;
    GLShader* shader3D = nullptr;

    // Extrusion parameters
    float extrusionHeight = 1.0f;
    float extrusionScale = 1.0f;
    float revolutionAngle = 360.0f;
    int revolutionSegments = 16;

    // Surfaces
    Surface3D currentSurface;
    TriangularSurface triangularSurface; // MOVED HERE
    bool surfaceGenerated = false;

    // Lighting
    glm::vec3 lightPos = glm::vec3(2.0f, 2.0f, 2.0f);
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 objectColor = glm::vec3(0.8f, 0.6f, 0.4f);

    // Camera controls
    bool cameraControlEnabled = false;
    bool firstMouse = true;
    float lastMouseX = 400.0f, lastMouseY = 300.0f;

    // Real-time editing
    bool realTimeExtrusion = true;
    bool needsExtrusionUpdate = false;

    // === EXISTING METHODS ===
    virtual void processInput();
    virtual void render();
    void renderCursor();
    virtual void renderMenu();
    void framebufferSizeCallback(int width, int height);
    void setMode(Mode newMode);

    void cursorPositionCallback(double xpos, double ypos);
    virtual void mouseButtonCallback(int button, int action, int mods);
    virtual void keyCallback(int key, int scancode, int action, int mods);

    void createNewCurve();
    void deleteCurve();
    void nextCurve();
    void selectNearestControlPoint(float x, float y);
    bool checkPointHover(float x, float y);
    void selectNearestClipPoint(float x, float y);
    bool checkClipPointHover(float x, float y);
    void clearClipWindow();

    void saveCurvesToFile();
    void loadCurvesFromFile();

    // Initialiser les descriptions des commandes
    void initCommandDescriptions();
    std::string getModeString() const;
    void mouseToOpenGL(double mouseX, double mouseY, float& oglX, float& oglY) const;
    void setupCursorBuffers();

    // === 3D METHODS ===
    void setupShaders3D();
    void render3D();
    void renderDual();
    void processCamera3D();

    // Extrusion methods
    void generateLinearExtrusion();
    void generateRevolutionExtrusion();
    void generateGeneralizedExtrusion();
    void calculateSurfaceNormals();
    std::vector<Point> getCurvePoints(const BezierCurve& curve) const;

    // ADD THESE MISSING METHODS:
    std::vector<Point> getCurvePointsFromCurve(const BezierCurve& curve) const;
    void bridgeCurvesSequentially();
    void bridgeMultipleCurves(); // ADD THIS
    void generateRuledSurfaceBridge(); // MOVED HERE

    // UI methods
    void renderExtrusionControls();
    void renderCameraControls();
    void renderLightingControls();
    void render3DViewport(float x, float y, float width, float height);
    void render2DViewport(float x, float y, float width, float height);

    // Mode setters
    void setViewMode(ViewMode mode);
    void setRenderMode3D(RenderMode3D mode);
    void setExtrusionType(ExtrusionType type);

    // Utility methods
    std::string getViewModeString() const;
    std::string getRenderMode3DString() const;
    std::string getExtrusionTypeString() const;
};

// === INLINE IMPLEMENTATIONS ===
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

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex3D),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                          (void*)offsetof(Vertex3D, normal));
    glEnableVertexAttribArray(1);

    // Texture coordinates
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D),
                          (void*)offsetof(Vertex3D, texCoord));
    glEnableVertexAttribArray(2);

    // Color
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