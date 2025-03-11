#include <cassert>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#ifdef WIN32
#include <GL/wglew.h>
#endif

#include "GLShader.h"


struct Application {
    GLShader m_basicShader;
    int width{};
    int height{};

    bool Initialize(int width, int height);

    void Terminate();

    void Render();
};

struct Vertex {
    float position[2]{};
    float color[3]{};

    Vertex(const float x, const float y, const float r, const float g, const float b) {
        position[0] = x;
        position[1] = y;
        color[0] = r;
        color[1] = g;
        color[2] = b;
    }
};

bool Application::Initialize(const int width, const int height) {
    this->width = width;
    this->height = height;

    const GLenum glewOk = glewInit();
    assert(glewOk == GLEW_NO_ERROR);

    m_basicShader.LoadVertexShader("shader/basic.vs.glsl");
    m_basicShader.LoadFragmentShader("shader/basic.fs.glsl");
    m_basicShader.Create();

#ifdef WIN32
    // Enable VSync
    wglSwapIntervalEXT(1);
#endif

    return true;
}

void Application::Terminate() {
}

void Application::Render() {
    glViewport(0, 0, width, height);

    glClearColor(0.5f, 0.5f, 0.5f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    const auto basicProgram = m_basicShader.GetProgram();
    glUseProgram(basicProgram);

    static const Vertex triangle[] = {
        Vertex(-0.5f, -0.5f, 1.0f, 0.0f, 0.0f),
        Vertex(0.5f, -0.5f, 0.0f, 1.0f, 0.0f),
        Vertex(0.0f, 0.5f, 0.0f, 0.0f, 1.0f)
    };

    const int loc_position = glGetAttribLocation(basicProgram, "a_Position");
    const int loc_color = glGetAttribLocation(basicProgram, "a_Color");
    constexpr int stride = sizeof(Vertex);
    glEnableVertexAttribArray(loc_position);
    glEnableVertexAttribArray(loc_color);
    glVertexAttribPointer(loc_position, 2, GL_FLOAT, GL_FALSE, stride, &triangle[0].position);
    glVertexAttribPointer(loc_color, 3, GL_FLOAT, GL_FALSE, stride, &triangle[0].color);

    glDrawArrays(GL_TRIANGLES, 0, 3);
}


int main() {
    Application app;
    /* Initialize the library */
    if (!glfwInit())
        return -1;

    constexpr int width = 640, height = 480;
    /* Create a windowed mode window and its OpenGL context */
    GLFWwindow *window = glfwCreateWindow(width, height, "Hello World", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    app.Initialize(width, height);
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window)) {
        /* Render here */
        app.Render();

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }
    app.Terminate();

    return 0;
}
