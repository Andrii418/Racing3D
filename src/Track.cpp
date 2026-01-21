#include "Track.h"
#include "Shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

/**
 * @file Track.cpp
 * @brief Implementacja klasy `Track` generującej płaski tor i renderującej go w OpenGL.
 */

 /**
  * @brief Konstruktor toru: inicjalizuje macierz modelu i generuje siatkę.
  */
Track::Track() {
    ModelMatrix = glm::mat4(1.0f);
    ModelMatrix = glm::scale(ModelMatrix, glm::vec3(20.0f, 1.0f, 20.0f));
    generateFlatTrack(10);
}

/**
 * @brief Generuje płaską siatkę toru i tworzy bufory VAO/VBO/EBO.
 * @param gridSize Liczba podziałów siatki na osi X/Z.
 */
void Track::generateFlatTrack(int gridSize) {
    float halfSize = 0.5f;
    vertices.clear();
    indices.clear();

    for (int i = 0; i <= gridSize; ++i) {
        for (int j = 0; j <= gridSize; ++j) {
            float x = (float)i / (float)gridSize - halfSize;
            float z = (float)j / (float)gridSize - halfSize;

            vertices.push_back(x);
            vertices.push_back(0.0f);
            vertices.push_back(z);

            vertices.push_back(0.0f);
            vertices.push_back(1.0f);
            vertices.push_back(0.0f);
        }
    }

    for (int i = 0; i < gridSize; ++i) {
        for (int j = 0; j < gridSize; ++j) {
            int current_row_start = i * (gridSize + 1);
            int next_row_start = (i + 1) * (gridSize + 1);

            unsigned int i0 = current_row_start + j;
            unsigned int i1 = current_row_start + j + 1;
            unsigned int i2 = next_row_start + j;
            unsigned int i3 = next_row_start + j + 1;

            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);

            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }

    vertexCount = (int)indices.size();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    GLsizei stride = 6 * sizeof(float);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

/**
 * @brief Renderuje tor, ustawiając macierz modelu i wywołując `glDrawElements`.
 * @param shader Shader używany do renderowania.
 */
void Track::Draw(const Shader& shader) {
    shader.setMat4("model", ModelMatrix);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}