#include "Camera.h"
#include <iostream>
#include <cmath>

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>

/**
 * @file Camera.cpp
 * @brief Implementacja klasy `Camera` (obliczanie macierzy oraz trybów kamery).
 */

 /**
  * @brief Tworzy kamerę i inicjalizuje jej wektory kierunkowe.
  * @param position Pozycja startowa kamery.
  * @param up Wektor „góry” świata.
  * @param yaw Początkowy yaw w stopniach.
  * @param pitch Początkowy pitch w stopniach.
  */
Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), Zoom(ZOOM) {
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

/**
 * @brief Aktualizuje wektory `Front` i `Up` na podstawie kątów Eulera.
 */
void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);

    glm::vec3 Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}

/**
 * @brief Zwraca macierz widoku kamery.
 * @return Macierz widoku `lookAt(Position, Position + Front, Up)`.
 */
glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(Position, Position + Front, Up);
}

/**
 * @brief Zwraca macierz projekcji perspektywicznej.
 * @param aspectRatio Proporcje ekranu (szerokość / wysokość).
 * @return Macierz perspektywy z parametrami: near=0.1, far=100.0, FOV=`Zoom`.
 */
glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(Zoom), aspectRatio, 0.1f, 100.0f);
}

/**
 * @brief Ustawia orientację kamery na podstawie rotacji yaw.
 * @param centerPoint Punkt odniesienia (nieużywany w tej implementacji).
 * @param yawRotation Rotacja yaw w stopniach.
 */
void Camera::LookAt(const glm::vec3& centerPoint, float yawRotation) {
    this->Yaw = yawRotation - 90.0f;
    this->Pitch = -15.0f;
    updateCameraVectors();
}

/**
 * @brief Tryb trzeciej osoby: kamera płynnie podąża za samochodem.
 * @param carPosition Pozycja samochodu.
 * @param carFrontVector Wektor przodu samochodu.
 */
void Camera::FollowCar(const glm::vec3& carPosition, const glm::vec3& carFrontVector) {
    float distance = 1.6f;
    float height = 0.8f;

    glm::vec3 desiredPosition = carPosition - glm::normalize(carFrontVector) * distance;
    desiredPosition.y += height;

    float smoothness = 0.1f;
    Position = glm::mix(Position, desiredPosition, smoothness);

    glm::vec3 lookAtTarget = carPosition + glm::vec3(0.0f, 0.5f, 0.0f);
    Front = glm::normalize(lookAtTarget - Position);

    glm::vec3 Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}

/**
 * @brief Tryb pierwszoosobowy: kamera ustawiona nisko z przodu pojazdu.
 * @param carPosition Pozycja samochodu.
 * @param carYaw Yaw samochodu w stopniach.
 */
void Camera::SetFrontCamera(const glm::vec3& carPosition, float carYaw) {
    float height = 0.35f;
    float forwardOffset = 0.85f;

    float yawRad = glm::radians(carYaw);
    glm::vec3 offset(sin(yawRad) * forwardOffset, height, cos(yawRad) * forwardOffset);

    Position = carPosition + offset;

    Yaw = 90.0f - carYaw;

    Pitch = -1.0f;

    updateCameraVectors();
}