#include "Camaro.h"
#include "Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

Camaro::Camaro(glm::vec3 startPosition)
    : Position(startPosition), Velocity(glm::vec3(0.0f)), Yaw(0.0f), VAO(0), VBO(0), EBO(0) {
    FrontVector = glm::vec3(0.0f, 0.0f, -1.0f);
}

bool Camaro::loadModel(const std::string& modelPath) {
    std::ifstream file(modelPath);
    if (!file.is_open()) {
        std::cout << "Nie moge otworzyc pliku: " << modelPath << std::endl;
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
            glm::vec3 vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            temp_vertices.push_back(vertex);
        }
        else if (prefix == "vn") {
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            temp_normals.push_back(normal);
        }
        else if (prefix == "vt") {
            glm::vec2 texcoord;
            iss >> texcoord.x >> texcoord.y;
            temp_texcoords.push_back(texcoord);
        }
        else if (prefix == "f") {
            std::string v1, v2, v3, v4;
            iss >> v1 >> v2 >> v3 >> v4;

            std::vector<std::string> faceVertices = { v1, v2, v3 };
            if (!v4.empty()) {
                faceVertices = { v1, v2, v3, v1, v3, v4 };
            }

            for (const auto& v : faceVertices) {
                std::istringstream viss(v);
                std::string v_str, vt_str, vn_str;

                std::getline(viss, v_str, '/');
                std::getline(viss, vt_str, '/');
                std::getline(viss, vn_str, '/');

                int vi = (v_str.empty()) ? 0 : std::stoi(v_str) - 1;
                int vti = (vt_str.empty()) ? 0 : std::stoi(vt_str) - 1;
                int vni = (vn_str.empty()) ? 0 : std::stoi(vn_str) - 1;

                if (vi >= 0 && vi < temp_vertices.size()) {
                    vertices.push_back(temp_vertices[vi]);
                }
                else {
                    vertices.push_back(glm::vec3(0.0f));
                }

                if (vni >= 0 && vni < temp_normals.size()) {
                    normals.push_back(temp_normals[vni]);
                }
                else {
                    normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
                }

                if (vti >= 0 && vti < temp_texcoords.size()) {
                    texCoords.push_back(temp_texcoords[vti]);
                }
                else {
                    texCoords.push_back(glm::vec2(0.0f));
                }

                indices.push_back(indices.size());
            }
        }
    }

    file.close();

    if (normals.empty() || normals.size() != vertices.size()) {
        std::cout << "Brak normalnych w modelu, obliczam..." << std::endl;
        calculateNormals();
    }

    std::cout << "Zaladowano model Camaro: " << vertices.size() << " wierzcholkow, "
        << indices.size() << " indeksow, " << normals.size() << " normalnych, "
        << texCoords.size() << " wspolrzednych tekstur" << std::endl;

    setupMesh();
    return true;
}

void Camaro::calculateNormals() {
    normals.resize(vertices.size(), glm::vec3(0.0f, 0.0f, 0.0f));

    for (size_t i = 0; i < indices.size(); i += 3) {
        if (i + 2 >= indices.size()) break;

        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];

        if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size()) {
            continue;
        }

        glm::vec3 v0 = vertices[i0];
        glm::vec3 v1 = vertices[i1];
        glm::vec3 v2 = vertices[i2];

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 normal = glm::cross(edge1, edge2);

        if (glm::length(normal) > 0.0f) {
            normal = glm::normalize(normal);
        }
        else {
            normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        normals[i0] += normal;
        normals[i1] += normal;
        normals[i2] += normal;
    }

    for (auto& normal : normals) {
        if (glm::length(normal) > 0.0f) {
            normal = glm::normalize(normal);
        }
        else {
            normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }
}

void Camaro::setupMesh() {
    if (vertices.empty()) {
        std::cout << "Brak wierzcholkow do renderowania!" << std::endl;
        return;
    }

    if (normals.size() != vertices.size()) {
        std::cout << "Liczba normalnych nie zgadza sie z wierzcholkami, obliczam..." << std::endl;
        calculateNormals();
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    std::vector<float> vertexData;
    for (size_t i = 0; i < vertices.size(); ++i) {
        vertexData.push_back(vertices[i].x);
        vertexData.push_back(vertices[i].y);
        vertexData.push_back(vertices[i].z);

        vertexData.push_back(normals[i].x);
        vertexData.push_back(normals[i].y);
        vertexData.push_back(normals[i].z);

        vertexData.push_back(texCoords[i].x);
        vertexData.push_back(texCoords[i].y);
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float),
        vertexData.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
        indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

    std::cout << "Utworzono VAO z " << vertexData.size() / 8 << " wierzcholkami" << std::endl;
}

void Camaro::Update(float deltaTime) {
    Position += Velocity * deltaTime;
    Velocity *= 0.98f;
}

glm::mat4 Camaro::GetModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, Position);
    model = glm::rotate(model, glm::radians(Yaw), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
    return model;
}

void Camaro::Draw(const Shader& shader, glm::vec3 pos, float yaw) const {
    if (VAO == 0) return;

    glm::mat4 model = glm::mat4(1.0f);

    if (glm::length(pos) > 0.001f || std::abs(yaw) > 0.001f) {
        model = glm::translate(model, pos);
        model = glm::rotate(model, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
    }
    else {
        model = GetModelMatrix();
    }

    shader.setMat4("model", model);

    shader.setBool("useTexture", true);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}