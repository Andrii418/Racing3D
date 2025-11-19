#ifndef TRACK_H
#define TRACK_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "Shader.h"

class Track {
public:
    glm::mat4 ModelMatrix;

    Track();
    void Draw(const Shader& shader);

private:
    void generateFlatTrack(int gridSize);

    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO, VBO, EBO;
    int vertexCount;
};

#endif