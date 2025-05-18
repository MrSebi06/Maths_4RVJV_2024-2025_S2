#include "include/BezierApp.h"
#include <iostream>

int main() {
    try {
        // Créer l'application avec une fenêtre de 800x600
        BezierApp app("Courbes de Bézier - Interface Graphique", 800, 600);

        // Afficher les instructions dans la console (en plus de l'interface graphique)
        std::cout << "=== Programme de manipulation de courbes de Bézier ===" << std::endl;
        std::cout << "Une interface graphique est maintenant disponible dans l'application." << std::endl;
        std::cout << "Utilisez les panneaux d'aide à l'écran pour voir les commandes disponibles." << std::endl;

        // Lancer l'application
        app.run();

        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << "Erreur: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}