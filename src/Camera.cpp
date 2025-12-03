#include "Camera.h"
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL

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
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);

    glm::vec3 Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(Position, Position + Front, Up);
}

glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(Zoom), aspectRatio, 0.1f, 100.0f);
}

void Camera::LookAt(const glm::vec3& centerPoint, float yawRotation) {
    this->Yaw = yawRotation - 90.0f;
    this->Pitch = -15.0f;
    updateCameraVectors();
}

void Camera::FollowCar(const glm::vec3& carPosition, const glm::vec3& carFrontVector) {
    float distance = 4.0f;
    float height = 2.0f;

    glm::vec3 desiredPosition = carPosition - glm::normalize(carFrontVector) * distance;
    desiredPosition.y += height;

    float smoothness = 0.1f;
    Position = glm::mix(Position, desiredPosition, smoothness);

    glm::vec3 lookAtTarget = carPosition + glm::vec3(0.0f, 0.5f, 0.0f);
    Front = glm::normalize(lookAtTarget - Position);
}