#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/**
 * @file Camera.h
 * @brief Deklaracja klasy `Camera` odpowiedzialnej za macierze widoku i projekcji oraz tryby kamery.
 */

 /** @brief Domyślna wartość yaw (w stopniach). */
const float YAW = -90.0f;

/** @brief Domyślna wartość pitch (w stopniach). */
const float PITCH = -20.0f;

/** @brief Domyślna wartość pola widzenia (FOV) w stopniach. */
const float ZOOM = 45.0f;

/**
 * @brief Kamera sceny 3D (widok + projekcja) z trybem „chase” i trybem pierwszoosobowym.
 *
 * Klasa utrzymuje pozycję i orientację kamery (`Position`, `Front`, `Up`) oraz parametry Eulera (`Yaw`, `Pitch`).
 * Udostępnia macierz widoku (`GetViewMatrix`) i macierz projekcji perspektywicznej (`GetProjectionMatrix`).
 *
 * Dodatkowo implementuje:
 * - kamerę podążającą za pojazdem (trzecia osoba) przez `FollowCar`
 * - kamerę „z przodu” (pierwsza osoba / zderzak) przez `SetFrontCamera`
 */
class Camera {
public:
    /** @brief Pozycja kamery w świecie. */
    glm::vec3 Position;

    /** @brief Znormalizowany wektor kierunku patrzenia. */
    glm::vec3 Front;

    /** @brief Znormalizowany wektor „góry” kamery. */
    glm::vec3 Up;

    /** @brief Globalny wektor „góry” świata (najczęściej (0,1,0)). */
    glm::vec3 WorldUp;

    /** @brief Kąt yaw w stopniach (obrót wokół osi Y). */
    float Yaw;

    /** @brief Kąt pitch w stopniach (pochylenie góra/dół). */
    float Pitch;

    /** @brief Pole widzenia (FOV) w stopniach. */
    float Zoom;

    /**
     * @brief Tworzy kamerę o zadanej pozycji i orientacji.
     * @param position Pozycja startowa kamery.
     * @param up Wektor „góry” świata (domyślnie (0,1,0)).
     * @param yaw Początkowy yaw w stopniach.
     * @param pitch Początkowy pitch w stopniach.
     */
    Camera(glm::vec3 position, glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH);

    /**
     * @brief Zwraca macierz widoku (View Matrix).
     * @return Macierz widoku `lookAt(Position, Position + Front, Up)`.
     */
    glm::mat4 GetViewMatrix() const;

    /**
     * @brief Zwraca macierz projekcji perspektywicznej (Projection Matrix).
     * @param aspectRatio Proporcje ekranu (szerokość / wysokość).
     * @return Macierz perspektywy z FOV = `Zoom`.
     */
    glm::mat4 GetProjectionMatrix(float aspectRatio) const;

    /**
     * @brief Ustawia kamerę w tryb trzecioosobowy i płynnie podąża za samochodem.
     * @param carPosition Aktualna pozycja samochodu.
     * @param carFrontVector Znormalizowany wektor przodu samochodu.
     */
    void FollowCar(const glm::vec3& carPosition, const glm::vec3& carFrontVector);

    /**
     * @brief Ustawia tryb pierwszoosobowy (kamera nisko z przodu pojazdu).
     *
     * Metoda ustawia pozycję kamery względem pojazdu bez wygładzania oraz synchronizuje jej orientację z yaw pojazdu.
     *
     * @param carPosition Aktualna pozycja samochodu.
     * @param carYaw Yaw samochodu w stopniach.
     */
    void SetFrontCamera(const glm::vec3& carPosition, float carYaw);

    /**
     * @brief Ustawia orientację kamery względem zadanego punktu odniesienia, używając rotacji yaw.
     * @param centerPoint Punkt odniesienia (nieużywany w implementacji, zachowany w interfejsie).
     * @param yawRotation Żądana rotacja yaw w stopniach.
     */
    void LookAt(const glm::vec3& centerPoint, float yawRotation);

private:
    /**
     * @brief Aktualizuje wektory `Front` i `Up` na podstawie `Yaw`, `Pitch` i `WorldUp`.
     */
    void updateCameraVectors();
};