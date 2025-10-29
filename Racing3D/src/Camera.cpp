// src/Camera.cpp
#include "Camera.h"
#include <iostream>

// --- KONIECZNA DYREKTYWA DLA U�YWANIA MODU��W GTX (NAPRAWA B��DU #error) ---
#define GLM_ENABLE_EXPERIMENTAL 
// -------------------------------------------------------------------------

// Do��czanie nag��wk�w GLM (musz� by� pod #define)
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
    // Obliczanie nowego wektora Front na podstawie k�t�w Eulera
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);

    // Obliczanie wektor�w Right i Up
    // UWAGA: Right powinno by� zmienn� sk�adow� w Camera.h.
    // U�ywamy lokalnej kopii dla bezpiecze�stwa kompilacji.
    glm::vec3 Right = glm::normalize(glm::cross(Front, WorldUp));

    // Obliczenie wektora Up:
    Up = glm::normalize(glm::cross(Right, Front));
}

glm::mat4 Camera::GetViewMatrix() const {
    // Funckja LookAt: pozycja kamery, punkt na kt�ry patrzy (cel), wektor "w g�r�"
    return glm::lookAt(Position, Position + Front, Up);
}

glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(Zoom), aspectRatio, 0.1f, 100.0f);
}

void Camera::FollowCar(const glm::vec3& carPosition, const glm::vec3& carFrontVector) {
    float distance = 4.0f; // Odleg�o�� kamery od samochodu
    float height = 2.0f;    // Wysoko�� kamery

    // 1. Obliczanie docelowej pozycji kamery (za samochodem i nieco wy�ej)
    glm::vec3 desiredPosition = carPosition - glm::normalize(carFrontVector) * distance;
    desiredPosition.y += height;

    // 2. P�ynne przej�cie kamery (dla profesjonalnego efektu)
    float smoothness = 0.1f; // Im mniejsza, tym p�ynniejsza
    Position = glm::mix(Position, desiredPosition, smoothness);

    // 3. Ustawienie celu, na kt�ry kamera patrzy (celujemy w samoch�d)
    Front = glm::normalize(carPosition - Position);

    // updateCameraVectors() nie jest konieczne, je�li polegasz na glm::lookAt
    // w GetViewMatrix, ale zostawiamy opcjonalnie, je�li to wp�ywa na logik�.
    // updateCameraVectors(); 
}