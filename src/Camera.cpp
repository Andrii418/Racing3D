#include "Camera.h"
#include <iostream>
#include <cmath>

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

// Third-person smooth follow
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

// New First-person "Bumper/Eyes" camera
void Camera::SetFrontCamera(const glm::vec3& carPosition, float carYaw) {
    // "Eyes of the car" / Bumper view configuration
    // Height: Very low (0.35f), just above the road surface for maximum speed sensation.
    // Forward: Pushed forward (0.85f) to sit at the front bumper/headlights, 
    //          ensuring we don't clip inside the car geometry.

    float height = 0.35f;
    float forwardOffset = 0.85f;

    // Calculate position offset based on car rotation
    // Note: main.cpp uses sin(Yaw) for X and cos(Yaw) for Z
    float yawRad = glm::radians(carYaw);
    glm::vec3 offset(sin(yawRad) * forwardOffset, height, cos(yawRad) * forwardOffset);

    // Apply strict position lock (no smoothing) for precise control in first-person
    Position = carPosition + offset;

    // Orientation:
    // Sync Camera Yaw with Car Yaw. 
    // Logic: If Car faces +Z (Yaw 0), Camera must face +Z.
    // Standard GL Camera faces +Z when Yaw is 90. 
    Yaw = 90.0f - carYaw;

    // Pitch almost flat (-1.0f) to look at the horizon and road ahead
    Pitch = -1.0f;

    updateCameraVectors();
}