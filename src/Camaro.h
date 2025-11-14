#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <vector>
#include <string>

class Shader;

class Camaro {
public:
    // Stan fizyczny/geometria
    glm::vec3 Position;
    glm::vec3 Velocity;
    glm::vec3 FrontVector;
    float Yaw;

    // Stałe fizyczne
    float MaxSpeed = 20.0f;
    float Acceleration = 5.0f;
    float Braking = 10.0f;
    float TurnRate = 1.5f;

    // Konstruktor
    Camaro(glm::vec3 startPosition = glm::vec3(0.0f, 0.5f, 0.0f));

    // Metody
    bool loadModel(const std::string& modelPath = "assets/models/75-chevrolet_camaro_ss/Chevrolet_Camaro_SS_Low.obj");
    void Update(float deltaTime);
    void Draw(const Shader& shader, glm::vec3 pos = glm::vec3(0.0f), float yaw = 0.0f) const;
    glm::mat4 GetModelMatrix() const;

private:
    void setupMesh();
    void calculateNormals();

    // Dane modelu OBJ
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<unsigned int> indices;

    // Buffery OpenGL
    unsigned int VAO, VBO, EBO;
};