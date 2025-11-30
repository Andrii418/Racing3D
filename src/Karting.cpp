#include "Karting.h"
#include "Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

Karting::Karting(glm::vec3 startPosition)
    : Position(startPosition), Scale(glm::vec3(1.0f)), Yaw(0.0f),
    VAO(0), VBO(0), EBO(0) {
}

bool Karting::loadModel(const std::string& modelPath) {
    std::ifstream file(modelPath);
    if (!file.is_open()) {
        std::cout << "Nie mogê otworzyæ pliku kartingu: " << modelPath << std::endl;
        return false;
    }

    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec3> temp_normals;
    std::vector<glm::vec2> temp_texcoords;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            glm::vec3 v;
            iss >> v.x >> v.y >> v.z;
            temp_vertices.push_back(v);
        }
        else if (prefix == "vn") {
            glm::vec3 n;
            iss >> n.x >> n.y >> n.z;
            temp_normals.push_back(n);
        }
        else if (prefix == "vt") {
            glm::vec2 t;
            iss >> t.x >> t.y;
            temp_texcoords.push_back(t);
        }
        else if (prefix == "f") {
            std::string a, b, c, d;
            iss >> a >> b >> c >> d;

            std::vector<std::string> faces = { a, b, c };
            if (!d.empty()) faces = { a, b, c, a, c, d };

            for (auto& f : faces) {
                std::istringstream parts(f);
                std::string v, t, n;
                std::getline(parts, v, '/');
                std::getline(parts, t, '/');
                std::getline(parts, n, '/');

                int vi = v.empty() ? 0 : std::stoi(v) - 1;
                int ti = t.empty() ? 0 : std::stoi(t) - 1;
                int ni = n.empty() ? 0 : std::stoi(n) - 1;

                vertices.push_back(temp_vertices[vi]);
                texCoords.push_back(ti >= 0 && ti < temp_texcoords.size() ? temp_texcoords[ti] : glm::vec2(0));
                normals.push_back(ni >= 0 && ni < temp_normals.size() ? temp_normals[ni] : glm::vec3(0, 1, 0));

                indices.push_back(indices.size());
            }
        }
    }
    file.close();

    if (normals.empty() || normals.size() != vertices.size())
        calculateNormals();

    setupMesh();
    std::cout << "Za³adowano trasê karting — " << vertices.size() << " wierzcho³ków" << std::endl;
    return true;
}

void Karting::calculateNormals() {
    normals.resize(vertices.size(), glm::vec3(0));
    for (size_t i = 0; i < indices.size(); i += 3) {
        glm::vec3 v0 = vertices[indices[i]];
        glm::vec3 v1 = vertices[indices[i + 1]];
        glm::vec3 v2 = vertices[indices[i + 2]];
        glm::vec3 n = glm::normalize(glm::cross(v1 - v0, v2 - v0));
        normals[indices[i]] += n;
        normals[indices[i + 1]] += n;
        normals[indices[i + 2]] += n;
    }
    for (auto& n : normals) n = glm::length(n) > 0 ? glm::normalize(n) : glm::vec3(0, 1, 0);
}

void Karting::setupMesh() {
    if (vertices.empty()) return;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    std::vector<float> data;
    for (size_t i = 0; i < vertices.size(); ++i) {
        data.push_back(vertices[i].x);
        data.push_back(vertices[i].y);
        data.push_back(vertices[i].z);

        data.push_back(normals[i].x);
        data.push_back(normals[i].y);
        data.push_back(normals[i].z);

        data.push_back(texCoords[i].x);
        data.push_back(texCoords[i].y);
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    glBindVertexArray(0);
}

glm::mat4 Karting::GetModelMatrix() const {
    glm::mat4 model(1);
    model = glm::translate(model, Position);
    model = glm::rotate(model, glm::radians(Yaw), glm::vec3(0, 1, 0));
    model = glm::scale(model, Scale);
    return model;
}

void Karting::Draw(const Shader& shader, glm::vec3 pos, float yaw) const {
    if (!VAO) return;

    glm::mat4 model = GetModelMatrix();
    shader.setMat4("model", model);
    shader.setBool("useTexture", true);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
