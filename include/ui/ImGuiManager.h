#ifndef IMGUI_MANAGER_H
#define IMGUI_MANAGER_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <map>
#include <functional>

class ImGuiManager {
public:
    ImGuiManager();
    ~ImGuiManager();

    // Initialiser ImGui avec la fenêtre GLFW
    void init(GLFWwindow* window);
    
    // Commencer une nouvelle frame
    void beginFrame();
    
    // Terminer la frame et dessiner
    void endFrame();
    
    // Libérer les ressources
    void shutdown();
    
    // Afficher l'interface des commandes
    void renderCommandsUI(const std::map<std::string, std::string>& commands, 
                         const std::string& currentMode);

private:
    GLFWwindow* window;
    bool initialized;
};

#endif // IMGUI_MANAGER_H