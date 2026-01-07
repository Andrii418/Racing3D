#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <vector>
#include <string>

class Shader;

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
    glm::vec3 PreviousPosition;
    glm::vec3 Velocity;
    glm::vec3 FrontVector;
    float Yaw;

    float MaxSpeed = 85.0f;
    float Acceleration = 45.0f;
    float Braking = 28.0f;
    float TurnRate = 2.0f;
    float WheelRotation = 0.0f;

    // Small physics tuning params
    float Grip = 0.8f; // higher reduces lateral sliding
    float AerodynamicDrag = 0.008f;
    float RollingResistance = 0.003f;

    float SteeringResponsiveness = 1.5f; // how quickly velocity aligns to car heading

    // Handbrake / drift
    bool Handbrake = false;
    float HandbrakeGripReduction = 0.6f; // how much grip is reduced when handbrake held (0..1)
    float HandbrakeDeceleration = 0.95f; // exponential deceleration multiplier per second when handbrake held

    // Input state
    float SteeringInput = 0.0f; // -1 right, +1 left

    RaceCar(glm::vec3 startPosition = glm::vec3(0.0f, 0.2f, 0.0f));

    bool loadAssets();
    void Update(float deltaTime);
    void Draw(const Shader& shader, glm::vec3 pos = glm::vec3(0.0f), float yaw = 0.0f) const;

    glm::mat4 GetModelMatrix() const;

private:
    CarMesh bodyMesh;
    CarMesh wheelMesh;
    unsigned int textureID;

    void HandleCollision();

    bool loadObj(const std::string& path, CarMesh& mesh);
    unsigned int loadTexture(const char* path);
    void calculateNormals(CarMesh& mesh);
};

