#include "../../include/ui/ImGuiManager.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>
#include <map>

ImGuiManager::ImGuiManager() : window(nullptr), initialized(false) {
}

ImGuiManager::~ImGuiManager() {
    if (initialized) {
        shutdown();
    }
}

void ImGuiManager::init(GLFWwindow* window) {
    this->window = window;
    
    // Configurer ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Activer la navigation clavier
    
    // Style
    ImGui::StyleColorsDark();
    
    // Initialiser les implémentations backend
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    initialized = true;
}

void ImGuiManager::beginFrame() {
    if (!initialized) return;
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::endFrame() {
    if (!initialized) return;
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiManager::shutdown() {
    if (!initialized) return;
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    initialized = false;
}

void ImGuiManager::renderCommandsUI(const std::map<std::string, std::string>& commands, 
                                    const std::string& currentMode) {
    if (!initialized) return;
    
    // Définir la position et la taille de départ (uniquement lors de la première utilisation)
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);

    // Créer une fenêtre avec le titre - Pas de flags qui empêcheraient le déplacement
    if (ImGui::Begin("Commandes disponibles", nullptr)) {
        // Afficher le mode actuel
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Mode actuel: %s", currentMode.c_str());
        ImGui::Separator();

        // Section pour les raccourcis clavier
        if (ImGui::CollapsingHeader("Commandes de l'application", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (const auto& [key, desc] : commands) {
                ImGui::BulletText("%s: %s", key.c_str(), desc.c_str());
            }
        }

        // Informations supplémentaires
        if (ImGui::CollapsingHeader("Aide")) {
            ImGui::TextWrapped("Utilisez les touches pour les différentes fonctionnalités. "
                              "Utilisez la souris pour ajouter ou éditer des points.");
            ImGui::TextWrapped("Vous pouvez déplacer cette fenêtre en cliquant et glissant sa barre de titre.");
        }
    }
    ImGui::End();
}