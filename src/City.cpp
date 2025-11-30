#include "City.h"
#include "Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

City::City(glm::vec3 startPosition)
    : Position(startPosition), Scale(glm::vec3(1.0f)), Yaw(0.0f), VAO(0), VBO(0), EBO(0) {
}

bool City::loadModel(const std::string& path) {
    std::string modelFile = path.empty() ? "assets/city/desert city.obj" : path;

    std::ifstream file(modelFile);
    if (!file.is_open()) {
        std::cout << "Nie mogê otworzyæ pliku: " << modelFile << std::endl;
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

                vertices.push_back(vi >= 0 && vi < temp_vertices.size() ? temp_vertices[vi] : glm::vec3(0.0f));
                normals.push_back(vni >= 0 && vni < temp_normals.size() ? temp_normals[vni] : glm::vec3(0.0f, 1.0f, 0.0f));
                texCoords.push_back(vti >= 0 && vti < temp_texcoords.size() ? temp_texcoords[vti] : glm::vec2(0.0f));

                indices.push_back(indices.size());
            }
        }
    }
    file.close();

    if (normals.empty() || normals.size() != vertices.size()) {
        calculateNormals();
    }

    setupMesh();
    std::cout << "Za³adowano model miasta: " << vertices.size() << " wierzcho³ków" << std::endl;
    return true;
    

}

void City::calculateNormals() {
    normals.resize(vertices.size(), glm::vec3(0.0f));
    for (size_t i = 0; i < indices.size(); i += 3) {
        if (i + 2 >= indices.size()) break;
        glm::vec3 v0 = vertices[indices[i]];
        glm::vec3 v1 = vertices[indices[i + 1]];
        glm::vec3 v2 = vertices[indices[i + 2]];
        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
        normals[indices[i]] += normal;
        normals[indices[i + 1]] += normal;
        normals[indices[i + 2]] += normal;
    }
    for (auto& n : normals) {
        if (glm::length(n) > 0.0f)
            n = glm::normalize(n);
        else
            n = glm::vec3(0.0f, 1.0f, 0.0f);
    }
}

void City::setupMesh() {
    if (vertices.empty()) return;

    
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
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

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

glm::mat4 City::GetModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, Position);
    model = glm::rotate(model, glm::radians(Yaw), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, Scale);
    return model;
}

void City::Draw(const Shader& shader, glm::vec3 pos, float yaw) const {
    if (VAO == 0) return;

    
        glm::mat4 model = GetModelMatrix();
    if (glm::length(pos) > 0.001f || std::abs(yaw) > 0.001f) {
        model = glm::translate(glm::mat4(1.0f), pos);
        model = glm::rotate(model, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, Scale);
    }

    shader.setMat4("model", model);
    shader.setBool("useTexture", true);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    

}
