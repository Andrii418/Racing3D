// src/Camera.cpp
#include "Camera.h"
#include <iostream>

// --- KONIECZNA DYREKTYWA DLA UŻYWANIA MODUŁÓW GTX (NAPRAWA BŁĘDU #error) ---
#define GLM_ENABLE_EXPERIMENTAL 
// -------------------------------------------------------------------------

// Dołączanie nagłówków GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp> 


Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), Zoom(ZOOM) {
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

void Camera::updateCameraVectors() {
    // Obliczanie nowego wektora Front na podstawie kątów Eulera
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);

    // Obliczanie wektorów Right i Up
    glm::vec3 Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}

glm::mat4 Camera::GetViewMatrix() const {
    // Funckja LookAt: pozycja kamery, punkt na który patrzy (cel), wektor "w górę"
    return glm::lookAt(Position, Position + Front, Up);
}

glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(Zoom), aspectRatio, 0.1f, 100.0f);
}

// Nowa metoda dla menu (symuluje obrót tła)
void Camera::LookAt(const glm::vec3& centerPoint, float yawRotation) {
    // W tej metodzie ustawiamy kamerę tak, aby obracała się na stałej pozycji,
    // tworząc iluzję ruchu tła.

    // 1. Ustawienie kąta YAW na podstawie rotacji tła
    this->Yaw = yawRotation - 90.0f; // -90.0f by dopasować do standardu OpenGL

    // 2. Utrzymanie stałego kąta PITCH (patrzenie lekko w dół)
    this->Pitch = -15.0f;

    // 3. Przerysowanie wektora Front (kierunku, na który patrzy kamera)
    updateCameraVectors();
}

void Camera::FollowCar(const glm::vec3& carPosition, const glm::vec3& carFrontVector) {
    float distance = 4.0f; // Odległość kamery od samochodu
    float height = 2.0f;    // Wysokość kamery

    // 1. Obliczanie docelowej pozycji kamery (za samochodem i nieco wyżej)
    glm::vec3 desiredPosition = carPosition - glm::normalize(carFrontVector) * distance;
    desiredPosition.y += height;

    // 2. Płynne przejście kamery (dla profesjonalnego efektu)
    float smoothness = 0.1f; // Im mniejsza, tym płynniejsza
    Position = glm::mix(Position, desiredPosition, smoothness);

    // 3. Ustawienie kierunku, na który kamera patrzy (patrzymy na samochód)
    glm::vec3 lookAtTarget = carPosition + glm::vec3(0.0f, 0.5f, 0.0f);
    Front = glm::normalize(lookAtTarget - Position);

    // Ponieważ Front jest teraz ustawiony, GetViewMatrix użyje go poprawnie.
    // Nie musimy aktualizować YAW/PITCH, ponieważ nie używamy sterowania myszą.
}