#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "Shader.h"

// Prosta struktura wierzcho³ka
struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

class Model {
public:
    Model(const std::string& path); // konstruktor wczytuj¹cy model
    ~Model();
    void Draw(Shader& shader);      // rysuje model

private:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    // Inicjalizacja do 0, aby zapobiec problemom w destruktorze
    unsigned int VAO = 0, VBO = 0, EBO = 0;

    void setupMesh();
    void loadModel(const std::string& path);
};