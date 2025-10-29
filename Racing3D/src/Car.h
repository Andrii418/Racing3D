#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include <iostream>

class Shader; // Deklaracja wsteczna (forward declaration), unikamy p�tli zale�no�ci

class Car {
public:
    // Stan fizyczny/geometria
    glm::vec3 Position;      // Pozycja w �wiecie 3D (x, y, z)
    glm::vec3 Velocity;      // Wektor pr�dko�ci
    glm::vec3 FrontVector;   // Wektor kierunku, w kt�rym "patrzy" samoch�d (wa�ne dla fizyki i kamery)
    float Yaw;               // K�t obrotu wok� osi Y (horyzontalnie)

    // Sta�e fizyczne (wst�pnie)
    float MaxSpeed = 10.0f; // Maksymalna pr�dko��
    float Acceleration = 5.0f; // Przyspieszenie
    float Braking = 10.0f;     // Si�a hamowania
    float TurnRate = 1.5f;   // Szybko�� skr�tu (w radianach/sekund�)

    // Dane do renderowania (VAO/VBO)
    unsigned int VAO;
    unsigned int VBO;

    // Konstruktor
    Car(glm::vec3 startPosition);

    // Metody
    void ProcessInput(float deltaTime); // Obs�uga wci�ni�tych klawiszy (W/S/A/D)
    void Update(float deltaTime);       // Obliczenia fizyczne i aktualizacja pozycji
    void Draw(const Shader& shader);    // Renderowanie geometrii
    glm::mat4 GetModelMatrix() const;   // Zwraca macierz Modelu

private:
    void setupMesh(); // Inicjalizacja VAO/VBO i wierzcho�k�w
    void processMovement(float deltaTime);
};