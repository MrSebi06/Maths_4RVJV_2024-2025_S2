#include "include/BezierApp.h"
#include <iostream>

int main() {
    try {
        // Créer l'application avec une fenêtre plus grande pour la 3D
        BezierApp app("Courbes de Bézier 2D/3D - Interface Graphique", 1400, 900);

        // Afficher les instructions dans la console
        std::cout << "=== Programme de manipulation de courbes de Bézier 2D/3D ===" << std::endl;
        std::cout << "Interface graphique disponible avec support 3D." << std::endl;
        std::cout << "Nouvelles fonctionnalités:" << std::endl;
        std::cout << "- V: Basculer entre vue 2D/3D/Dual" << std::endl;
        std::cout << "- L: Extrusion linéaire" << std::endl;
        std::cout << "- K: Extrusion par révolution" << std::endl;
        std::cout << "- WASD: Navigation caméra 3D" << std::endl;

        // Lancer l'application
        app.run();

        return EXIT_SUCCESS;
    } catch (const std::exception& e) {
        std::cerr << "Erreur: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}