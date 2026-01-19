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
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    void setupMesh();
};

class RaceCar {
public:
    glm::vec3 Position;
    glm::vec3 PreviousPosition;
    glm::vec3 Velocity;
    glm::vec3 FrontVector;
    float Yaw;

    // PHYSICS TUNING (Target: 18 km/h = 5.0 m/s)
    float MaxSpeed = 5.0f;           // 5 m/s * 3.6 = 18 km/h
    float Acceleration = 8.0f;       // Snappy acceleration
    float Braking = 10.0f;           // Strong brakes
    float TurnRate = 2.5f;           // Slightly sharper turning
    float WheelRotation = 0.0f;

    // Resistance Tuning
    float Grip = 1.5f;               // High grip for tight controls
    float AerodynamicDrag = 0.02f;   // Low drag
    float RollingResistance = 0.5f;  // Moderate coasting friction
    float SteeringResponsiveness = 2.0f;

    bool Handbrake = false;
    float HandbrakeGripReduction = 0.6f;
    float HandbrakeDeceleration = 0.95f;

    float SteeringInput = 0.0f;
    float ThrottleInput = 0.0f;
    float Throttle = 0.0f;
    float ThrottleResponse = 5.0f;   // Faster throttle input reaction

    // Visual settings
    float WheelFrontX = 0.45f;
    float WheelBackX = 0.45f;
    float WheelZ = 0.42f;

    RaceCar(glm::vec3 startPosition = glm::vec3(0.0f, 0.2f, 0.0f));
    bool loadAssets(const std::string& bodyPath, const std::string& wheelFrontPath, const std::string& wheelBackPath = "");
    void cleanup();
    void Update(float deltaTime);
    void Draw(const Shader& shader, glm::vec3 pos = glm::vec3(0.0f), float yaw = 0.0f) const;
    glm::mat4 GetModelMatrix() const;

private:
    CarMesh bodyMesh;
    CarMesh wheelFrontMesh;
    CarMesh wheelBackMesh;
    unsigned int textureID = 0;
    void HandleCollision();
    bool loadObj(const std::string& path, CarMesh& mesh, bool isBody);
    unsigned int loadTexture(const char* path);
};