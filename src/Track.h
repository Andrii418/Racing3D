#ifndef TRACK_H
#define TRACK_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "Shader.h"

/**
 * @file Track.h
 * @brief Definicja klasy `Track` generującej i renderującej prosty płaski tor (siatkę) w OpenGL.
 */

 /**
  * @brief Prosty tor w formie płaskiej siatki trójkątów generowanej proceduralnie.
  *
  * Klasa tworzy siatkę o zadanej rozdzielczości (`gridSize`) w zakresie [-0.5, 0.5] w osiach X/Z,
  * a następnie skaluje ją macierzą `ModelMatrix`. Geometria zawiera pozycję i normalną dla każdego
  * wierzchołka (układ interleaved: 3 floaty pozycji + 3 floaty normalnej).
  */
class Track {
public:
    /** @brief Macierz modelu toru (skalowanie/rotacja/translacja) przekazywana do shadera. */
    glm::mat4 ModelMatrix;

    /**
     * @brief Konstruktor toru.
     *
     * Inicjalizuje `ModelMatrix` oraz generuje płaski tor domyślną rozdzielczością siatki.
     */
    Track();

    /**
     * @brief Renderuje tor.
     * @param shader Shader używany do renderowania (ustawiany jest uniform `model`).
     */
    void Draw(const Shader& shader);

private:
    /**
     * @brief Generuje płaski tor w postaci siatki trójkątów oraz tworzy VAO/VBO/EBO.
     *
     * Generuje vertices i indices dla siatki (grid), następnie tworzy bufory OpenGL i ustawia
     * atrybuty wierzchołków:
     * - location 0: pozycja (vec3)
     * - location 1: normalna (vec3)
     *
     * @param gridSize Liczba podziałów na osi X/Z (siatka ma (gridSize+1)^2 wierzchołków).
     */
    void generateFlatTrack(int gridSize);

    /** @brief Bufor danych wierzchołków (interleaved: pos + normal). */
    std::vector<float> vertices;

    /** @brief Bufor indeksów trójkątów. */
    std::vector<unsigned int> indices;

    /** @brief Uchwyty buforów OpenGL. */
    unsigned int VAO, VBO, EBO;

    /** @brief Liczba indeksów rysowanych w `glDrawElements`. */
    int vertexCount;
};

#endif