#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <vector>
#include <string>
#include "Track.h"

/**
 * @file City.h
 * @brief Deklaracja klasy `City` reprezentującej statyczne otoczenie (miasto) wczytywane z pliku OBJ.
 */

class Shader;

/**
 * @brief Klasa reprezentująca model miasta wczytywany z pliku OBJ i renderowany przez OpenGL.
 *
 * Klasa dziedziczy po `Track`, aby mogła być używana jako alternatywny typ „sceny/toru”.
 * Odpowiada za:
 * - wczytanie geometrii OBJ (wierzchołki, normalne, UV, faces),
 * - ewentualne wyliczenie normalnych (gdy brak w pliku),
 * - konfigurację VAO/VBO/EBO,
 * - renderowanie przy użyciu shadera.
 */
class City : public Track {

public:
    /** @brief Pozycja modelu miasta w świecie. */
    glm::vec3 Position;

    /** @brief Skala modelu miasta. */
    glm::vec3 Scale;

    /** @brief Obrót wokół osi Y (w stopniach). */
    float Yaw;

    /**
     * @brief Konstruktor ustawiający pozycję startową oraz inicjalizujący stan renderingu.
     * @param startPosition Pozycja początkowa modelu miasta.
     */
    City(glm::vec3 startPosition = glm::vec3(1.0f, 2.0f, 1.0f));

    /**
     * @brief Wczytuje model miasta z pliku OBJ.
     *
     * Jeśli parametr jest pusty, używany jest domyślny plik `assets/city/desert city.obj`.
     *
     * @param modelPath Ścieżka do pliku OBJ.
     * @return `true` jeśli wczytanie i przygotowanie buforów zakończyło się sukcesem; inaczej `false`.
     */
    bool loadModel(const std::string& modelPath = "assets/city/desert city.obj");

    /**
     * @brief Renderuje model miasta.
     *
     * Gdy `pos` lub `yaw` są różne od wartości domyślnych, funkcja buduje macierz modelu
     * na podstawie argumentów (z zachowaniem `Scale`). W przeciwnym razie używa `GetModelMatrix()`.
     *
     * @param shader Shader używany do renderowania.
     * @param pos Opcjonalna pozycja nadpisująca `Position`.
     * @param yaw Opcjonalny obrót nadpisujący `Yaw` (w stopniach).
     */
    void Draw(const Shader& shader, glm::vec3 pos = glm::vec3(0.0f), float yaw = 0.0f) const;

    /**
     * @brief Zwraca macierz modelu (translacja -> rotacja -> skala).
     * @return Macierz modelu.
     */
    glm::mat4 GetModelMatrix() const;

private:
    /**
     * @brief Konfiguruje VAO/VBO/EBO i przesyła dane geometrii na GPU.
     */
    void setupMesh();

    /**
     * @brief Wylicza normalne na podstawie trójkątów, gdy brak kompletnych danych normalnych.
     */
    void calculateNormals();

    /** @brief Pozycje wierzchołków siatki. */
    std::vector<glm::vec3> vertices;

    /** @brief Normalne wierzchołków siatki. */
    std::vector<glm::vec3> normals;

    /** @brief Współrzędne UV wierzchołków siatki. */
    std::vector<glm::vec2> texCoords;

    /** @brief Indeksy wierzchołków (EBO), tworzące trójkąty. */
    std::vector<unsigned int> indices;

    /** @brief Bufory OpenGL: VAO, VBO oraz EBO. */
    unsigned int VAO, VBO, EBO;
};