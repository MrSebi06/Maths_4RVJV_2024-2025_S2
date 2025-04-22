#include "../include/Shader.h"

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
    // 1. Récupérer le code source des fichiers
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    // S'assurer que les objets ifstream peuvent lancer des exceptions
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        // Ouvrir les fichiers
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);

        // Lire les buffers dans des flux
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();

        // Fermer les gestionnaires de fichiers
        vShaderFile.close();
        fShaderFile.close();

        // Convertir le flux en chaîne de caractères
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cerr << "ERREUR::SHADER::FICHIER_NON_LU_AVEC_SUCCES" << std::endl;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    // 2. Compiler les shaders
    unsigned int vertex, fragment;
    int success;
    char infoLog[512];

    // Shader de sommets
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    // Vérifier les erreurs de compilation
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cerr << "ERREUR::SHADER::VERTEX::COMPILATION_ECHOUEE\n" << infoLog << std::endl;
    }

    // Shader de fragments
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    // Vérifier les erreurs de compilation
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cerr << "ERREUR::SHADER::FRAGMENT::COMPILATION_ECHOUEE\n" << infoLog << std::endl;
    }

    // Programme de shader
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    // Vérifier les erreurs de liaison
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cerr << "ERREUR::SHADER::PROGRAMME::LIAISON_ECHOUEE\n" << infoLog << std::endl;
    }

    // Supprimer les shaders car ils sont maintenant liés au programme et ne sont plus nécessaires
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::use() {
    glUseProgram(ID);
}

void Shader::setBool(const std::string &name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::setInt(const std::string &name, int value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setFloat(const std::string &name, float value) const {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setVec2(const std::string &name, float x, float y) const {
    glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
}

void Shader::setVec3(const std::string &name, float x, float y, float z) const {
    glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
}

void Shader::setVec4(const std::string &name, float x, float y, float z, float w) const {
    glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
}