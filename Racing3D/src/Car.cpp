// src/Car.cpp
#include "Car.h"
#include "Shader.h"
#include <glm/gtc/matrix_transform.hpp>

// Wierzcho³ki szeœcianu z NORMALNYMI (6 floatów na wierzcho³ek: Pos X,Y,Z | Norm X,Y,Z)
float carVertices[] = {
    // ---- TY£ (-Z) Normalna: 0, 0, -1 ----
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

    // ---- PRZÓD (+Z) Normalna: 0, 0, 1 ----
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

    // ---- LEWA STRONA (-X) Normalna: -1, 0, 0 ----
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

    // ---- PRAWA STRONA (+X) Normalna: 1, 0, 0 ----
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

     // ---- GÓRA (+Y) Normalna: 0, 1, 0 ----
     -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
      0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
      0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
      0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,

     // ---- DÓ£ (-Y) Normalna: 0, -1, 0 ----
     -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
      0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
      0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
      0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f
};


Car::Car(glm::vec3 startPosition)
    : Position(startPosition), Velocity(glm::vec3(0.0f)), Yaw(0.0f) {

    FrontVector = glm::vec3(0.0f, 0.0f, -1.0f);
    setupMesh();
    // Inicjalizacja sta³ych
    Acceleration = 5.0f;
    Braking = 10.0f;
    MaxSpeed = 20.0f;
    TurnRate = 1.0f;
}

void Car::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(carVertices), carVertices, GL_STATIC_DRAW);

    // Krok to 6 floatów: 3x pozycja + 3x normalna
    GLsizei stride = 6 * sizeof(float);

    // Atrybut 0: Pozycja (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    // Atrybut 1: Normalna (location 1) - zaczyna siê po 3 floatach
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Car::Update(float deltaTime) {
    // 1. Aktualizacja pozycji
    Position += Velocity * deltaTime;

    // 2. Proste t³umienie ruchu (symulacja oporu)
    Velocity *= 0.98f;
}

glm::mat4 Car::GetModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);

    // 1. Translacja (przesuniêcie)
    model = glm::translate(model, Position);

    // 2. Rotacja (wokó³ osi Y na podstawie k¹ta Yaw)
    model = glm::rotate(model, glm::radians(Yaw), glm::vec3(0.0f, 1.0f, 0.0f));

    // 3. Skalowanie (Samochód)
    model = glm::scale(model, glm::vec3(1.0f, 0.5f, 2.0f));

    return model;
}

void Car::Draw(const Shader& shader) {
    shader.setMat4("model", GetModelMatrix());

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}