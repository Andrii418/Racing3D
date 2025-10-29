#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include <iostream>

class Shader; // Deklaracja wsteczna (forward declaration), unikamy pêtli zale¿noœci

class Car {
public:
    // Stan fizyczny/geometria
    glm::vec3 Position;      // Pozycja w œwiecie 3D (x, y, z)
    glm::vec3 Velocity;      // Wektor prêdkoœci
    glm::vec3 FrontVector;   // Wektor kierunku, w którym "patrzy" samochód (wa¿ne dla fizyki i kamery)
    float Yaw;               // K¹t obrotu wokó³ osi Y (horyzontalnie)

    // Sta³e fizyczne (wstêpnie)
    float MaxSpeed = 10.0f; // Maksymalna prêdkoœæ
    float Acceleration = 5.0f; // Przyspieszenie
    float Braking = 10.0f;     // Si³a hamowania
    float TurnRate = 1.5f;   // Szybkoœæ skrêtu (w radianach/sekundê)

    // Dane do renderowania (VAO/VBO)
    unsigned int VAO;
    unsigned int VBO;

    // Konstruktor
    Car(glm::vec3 startPosition);

    // Metody
    void ProcessInput(float deltaTime); // Obs³uga wciœniêtych klawiszy (W/S/A/D)
    void Update(float deltaTime);       // Obliczenia fizyczne i aktualizacja pozycji
    void Draw(const Shader& shader);    // Renderowanie geometrii
    glm::mat4 GetModelMatrix() const;   // Zwraca macierz Modelu

private:
    void setupMesh(); // Inicjalizacja VAO/VBO i wierzcho³ków
    void processMovement(float deltaTime);
};