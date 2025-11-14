// src/Camera.h
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Domyślne wartości
const float YAW = -90.0f;
const float PITCH = -20.0f; // Patrzymy trochę w dół na samochód
const float ZOOM = 45.0f;

class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 WorldUp;
    float Yaw;
    float Pitch;
    float Zoom;
    // Potrzebujemy też Right dla updateCameraVectors, ale nie jest kluczowy dla tej logiki.
    // Jeśli go nie masz, używaj lokalnie w Camera.cpp.

    Camera(glm::vec3 position, glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH);

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspectRatio) const;

    // KLUCZOWA METODA: Zapewnia widok zza samochodu
    void FollowCar(const glm::vec3& carPosition, const glm::vec3& carFrontVector);

    // --- NOWA METODA DLA ANIMACJI TŁA W MENU ---
    void LookAt(const glm::vec3& centerPoint, float yawRotation);
    // ------------------------------------------

private:
    void updateCameraVectors();
};