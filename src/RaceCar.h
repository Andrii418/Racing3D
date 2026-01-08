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

    // Reasonable defaults: MaxSpeed in m/s
    float MaxSpeed = 18.0f;
    // Acceleration in m/s^2 (how quickly speed increases)
    float Acceleration = 18.0f; // increased to make forward acceleration stronger
    float Braking = 8.0f; // reduced so reverse/braking isn't stronger than throttle
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

    // Throttle smoothing: current smoothed throttle (0..1 forward, -1..0 reverse)
    float Throttle = 0.0f;
    // Raw input from player: -1..1
    float ThrottleInput = 0.0f;
    // How fast the throttle responds (higher = snappier)
    float ThrottleResponse = 3.5f;

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

