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

    // Точна фізика з GitHub
    float MaxSpeed = 18.0f;
    float Acceleration = 18.0f;
    float Braking = 8.0f;
    float TurnRate = 2.0f;
    float WheelRotation = 0.0f;
    float Grip = 0.8f;
    float AerodynamicDrag = 0.008f;
    float RollingResistance = 0.003f;
    float SteeringResponsiveness = 1.5f;

    bool Handbrake = false;
    float HandbrakeGripReduction = 0.6f;
    float HandbrakeDeceleration = 0.95f;

    float SteeringInput = 0.0f;
    float ThrottleInput = 0.0f;
    float Throttle = 0.0f;
    float ThrottleResponse = 3.5f;

    // Візуальні налаштування коліс
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