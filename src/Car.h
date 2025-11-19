#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include <iostream>

class Shader;

class Car {
public:
    glm::vec3 Position;
    glm::vec3 Velocity;
    glm::vec3 FrontVector;
    float Yaw;

    float MaxSpeed = 10.0f;
    float Acceleration = 5.0f;
    float Braking = 10.0f;
    float TurnRate = 1.5f;

    unsigned int VAO;
    unsigned int VBO;

    Car(glm::vec3 startPosition);

    void ProcessInput(float deltaTime);
    void Update(float deltaTime);

    void Draw(const Shader& shader, glm::vec3 pos = glm::vec3(0.0f), float yaw = 0.0f) const;

    glm::mat4 GetModelMatrix() const;

private:
    void setupMesh();
    void processMovement(float deltaTime);
};