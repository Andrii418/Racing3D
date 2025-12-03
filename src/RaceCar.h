#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <vector>
#include <string>

class Shader;

// Helper struct to keep Body and Wheels separate
struct CarMesh {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<unsigned int> indices;
    unsigned int VAO, VBO, EBO;

    void setupMesh();
};

class RaceCar {
public:
    glm::vec3 Position;
    glm::vec3 Velocity;
    glm::vec3 FrontVector;
    float Yaw;

    // Physics settings
    float MaxSpeed = 20.0f;
    float Acceleration = 15.0f;
    float Braking = 10.0f;
    float TurnRate = 2.0f;
    float WheelRotation = 0.0f; // To spin wheels

    RaceCar(glm::vec3 startPosition = glm::vec3(0.0f, 0.2f, 0.0f));

    bool loadAssets();

    void Update(float deltaTime);

    // Zaktualizowana deklaracja Draw, aby pasowała do wywołań w main.cpp (Menu i Gra)
    void Draw(const Shader& shader, glm::vec3 pos = glm::vec3(0.0f), float yaw = 0.0f) const;

    glm::mat4 GetModelMatrix() const;

private:
    CarMesh bodyMesh;
    CarMesh wheelMesh;
    unsigned int textureID;

    bool loadObj(const std::string& path, CarMesh& mesh);
    unsigned int loadTexture(const char* path);
    void calculateNormals(CarMesh& mesh);
};