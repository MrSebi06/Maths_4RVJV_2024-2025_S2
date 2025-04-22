#include "include/BezierApp.h"
#include <iostream>

int main() {
    try {
        // Créer l'application avec une fenêtre de 800x600
        BezierApp app("Courbes de Bézier", 800, 600);

        // Afficher les instructions
        std::cout << "=== Programme de manipulation de courbes de Bézier ===" << std::endl;
        std::cout << "Cliquez pour ajouter des points de contrôle." << std::endl;
        std::cout << "Utilisez les touches du clavier pour les différentes fonctionnalités." << std::endl;

        // Lancer l'application
        app.run();

        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << "Erreur: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}