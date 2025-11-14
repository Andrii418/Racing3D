// src/Track.cpp
#include "Track.h"
#include "Shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

Track::Track() {
    ModelMatrix = glm::mat4(1.0f);
    // Skalujemy tor, aby by³ du¿y i widoczny
    ModelMatrix = glm::scale(ModelMatrix, glm::vec3(20.0f, 1.0f, 20.0f));
    generateFlatTrack(10); // U¿ywamy ma³ego rozmiaru siatki, np. 10x10, aby by³ szybszy.
}

void Track::generateFlatTrack(int gridSize) {
    // Generowanie p³askiej siatki (grid)
    // Tworzymy siatkê w zakresie od -0.5 do 0.5 (rozmiar 1x1, który jest skalowany w ModelMatrix)
    float halfSize = 0.5f;
    vertices.clear();
    indices.clear();

    for (int i = 0; i <= gridSize; ++i) {
        for (int j = 0; j <= gridSize; ++j) {
            float x = (float)i / (float)gridSize - halfSize;
            float z = (float)j / (float)gridSize - halfSize;

            // Pozycja (x, y, z)
            vertices.push_back(x);
            vertices.push_back(0.0f); // P³aski tor na y=0
            vertices.push_back(z);

            // Normalna (zawsze w górê, +Y)
            vertices.push_back(0.0f);
            vertices.push_back(1.0f);
            vertices.push_back(0.0f);
        }
    }

    // Generowanie indeksów dla trójk¹tów
    for (int i = 0; i < gridSize; ++i) {
        for (int j = 0; j < gridSize; ++j) {
            int current_row_start = i * (gridSize + 1);
            int next_row_start = (i + 1) * (gridSize + 1);

            unsigned int i0 = current_row_start + j;
            unsigned int i1 = current_row_start + j + 1;
            unsigned int i2 = next_row_start + j;
            unsigned int i3 = next_row_start + j + 1;

            // Pierwszy trójk¹t: i0, i2, i1
            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);

            // Drugi trójk¹t: i1, i2, i3
            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }

    vertexCount = indices.size();

    // KONFIGURACJA VAO/VBO/EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // Krok to 6 floatów: 3x pozycja + 3x normalna
    GLsizei stride = 6 * sizeof(float);

    // Atrybut 0: Pozycja (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    // Atrybut 1: Normalna (location 1) - zaczyna siê po 3 floatach
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Track::Draw(const Shader& shader) {
    shader.setMat4("model", ModelMatrix);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}