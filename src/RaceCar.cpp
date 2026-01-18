#include "RaceCar.h"
#include "Shader.h"
#include "stb_image.h" 
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

void CarMesh::setupMesh() {
    if (vertices.empty()) return;
    if (VAO != 0) { glDeleteVertexArrays(1, &VAO); glDeleteBuffers(1, &VBO); glDeleteBuffers(1, &EBO); }
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO); glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    std::vector<float> data;
    for (size_t i = 0; i < vertices.size(); ++i) {
        data.push_back(vertices[i].x); data.push_back(vertices[i].y); data.push_back(vertices[i].z);
        data.push_back(normals[i].x);  data.push_back(normals[i].y);  data.push_back(normals[i].z);
        data.push_back(texCoords[i].x); data.push_back(texCoords[i].y);
    }
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(data.size() * sizeof(float)), data.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

RaceCar::RaceCar(glm::vec3 startPosition) : Position(startPosition), PreviousPosition(startPosition), Velocity(0.0f), Yaw(0.0f), textureID(0) { FrontVector = glm::vec3(0, 0, 1); }

void RaceCar::cleanup() {
    auto del = [](CarMesh& m) { if (m.VAO) { glDeleteVertexArrays(1, &m.VAO); glDeleteBuffers(1, &m.VBO); glDeleteBuffers(1, &m.EBO); m.VAO = 0; } m.vertices.clear(); m.indices.clear(); m.normals.clear(); m.texCoords.clear(); };
    del(bodyMesh); del(wheelFrontMesh); del(wheelBackMesh);
}

bool RaceCar::loadAssets(const std::string& bodyPath, const std::string& wheelFrontPath, const std::string& wheelBackPath) {
    cleanup();
    if (textureID == 0) textureID = loadTexture("assets/cars/OBJ format/Textures/colormap.png");
    if (!loadObj(bodyPath, bodyMesh, true)) return false;
    if (!loadObj(wheelFrontPath, wheelFrontMesh, false)) return false;
    std::string bPath = wheelBackPath.empty() ? wheelFrontPath : wheelBackPath;
    if (!loadObj(bPath, wheelBackMesh, false)) return false;
    return true;
}

void RaceCar::Update(float deltaTime) {
    PreviousPosition = Position;
    FrontVector.x = sin(glm::radians(Yaw));
    FrontVector.z = cos(glm::radians(Yaw));
    FrontVector.y = 0.0f;
    FrontVector = glm::normalize(FrontVector);

    float tResponse = glm::clamp(ThrottleResponse * deltaTime, 0.0f, 1.0f);
    Throttle = glm::mix(Throttle, ThrottleInput, tResponse);

    glm::vec3 forward = FrontVector;
    float speed = glm::length(Velocity);
    glm::vec3 velLong = forward * glm::dot(Velocity, forward);
    glm::vec3 velLat = Velocity - velLong;

    float effectiveGrip = Grip;
    float forwardSpeed = glm::dot(Velocity, forward);

    if (Throttle > 0.01f && !Handbrake) forwardSpeed += (Throttle * Acceleration) * deltaTime;
    else if (Throttle < -0.01f) forwardSpeed -= (-Throttle * Braking) * deltaTime;

    forwardSpeed = glm::clamp(forwardSpeed, -MaxSpeed * 0.5f, MaxSpeed);
    velLong = forward * forwardSpeed;

    if (Handbrake) {
        float newForwardSpeed = forwardSpeed * powf(HandbrakeDeceleration, deltaTime);
        if (newForwardSpeed < 0.0f) newForwardSpeed = 0.0f;
        velLong = forward * newForwardSpeed;
        effectiveGrip *= (1.0f - HandbrakeGripReduction);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
        velLat += right * (-SteeringInput) * 0.25f * (glm::clamp(newForwardSpeed, 0.0f, MaxSpeed) / (MaxSpeed + 0.1f));
    }

    Velocity = velLong + velLat;
    speed = glm::length(Velocity);

    if (speed > 0.001f) Velocity += (-glm::normalize(Velocity) * (AerodynamicDrag * speed * speed)) * deltaTime;
    Velocity -= Velocity * RollingResistance * deltaTime;

    velLong = forward * glm::dot(Velocity, forward);
    velLat = Velocity - velLong;
    velLat *= (1.0f - glm::clamp(effectiveGrip * deltaTime, 0.0f, 0.95f));
    Velocity = velLong + velLat;

    Position += Velocity * deltaTime;

    speed = glm::length(Velocity);
    if (speed > 0.5f && !Handbrake) {
        float alignFactor = SteeringResponsiveness * deltaTime * glm::clamp(Grip * (1.0f - speed / (MaxSpeed + 0.001f)), 0.0f, 1.0f);
        glm::vec3 velDir = (speed > 0.001f) ? glm::normalize(Velocity) : forward;
        Velocity = glm::normalize(glm::mix(velDir, forward, alignFactor)) * speed * 0.99f;
    }

    Velocity *= 0.98f;
    Position.y = 0.0f;
    HandleCollision();
    WheelRotation += glm::dot(Velocity, forward) * deltaTime * 10.0f;
}

void RaceCar::HandleCollision() { if (glm::length(Position) > 500.0f) { Position = PreviousPosition; Velocity = glm::vec3(0); } }

glm::mat4 RaceCar::GetModelMatrix() const {
    glm::mat4 m = glm::translate(glm::mat4(1.0f), Position);
    m = glm::rotate(m, glm::radians(Yaw), glm::vec3(0, 1, 0));
    return glm::scale(m, glm::vec3(0.15f));
}

void RaceCar::Draw(const Shader& shader, glm::vec3 pos, float yaw) const {
    glm::mat4 m = (glm::length(pos) > 0.001f) ? glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), pos), glm::radians(yaw), glm::vec3(0, 1, 0)), glm::vec3(0.15f)) : GetModelMatrix();
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, textureID);
    shader.setBool("useTexture", true); shader.setMat4("model", m);
    glBindVertexArray(bodyMesh.VAO); glDrawElements(GL_TRIANGLES, (GLsizei)bodyMesh.indices.size(), GL_UNSIGNED_INT, 0);

    glm::vec3 wOffs[] = { {WheelFrontX, 0.15f, WheelZ}, {-WheelFrontX, 0.15f, WheelZ}, {WheelBackX, 0.15f, -WheelZ}, {-WheelBackX, 0.15f, -WheelZ} };

    for (int i = 0; i < 4; i++) {
        const CarMesh& currentWheel = (i < 2) ? wheelFrontMesh : wheelBackMesh;
        if (currentWheel.VAO == 0) continue;
        glm::mat4 wM = glm::rotate(glm::translate(m, wOffs[i]), glm::radians(WheelRotation), glm::vec3(1, 0, 0));
        shader.setMat4("model", wM);
        glBindVertexArray(currentWheel.VAO); glDrawElements(GL_TRIANGLES, (GLsizei)currentWheel.indices.size(), GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}

bool RaceCar::loadObj(const std::string& path, CarMesh& mesh, bool isBody) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    std::vector<glm::vec3> t_v; std::vector<glm::vec3> t_vn; std::vector<glm::vec2> t_vt;
    std::string line; bool skipPart = false;
    while (std::getline(file, line)) {
        std::stringstream ss(line); std::string pr; ss >> pr;
        if (pr == "o" || pr == "g") {
            std::string name; ss >> name; std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            skipPart = (isBody && (name.find("wheel") != std::string::npos));
        }
        if (pr == "v") { glm::vec3 v; ss >> v.x >> v.y >> v.z; t_v.push_back(v); }
        else if (pr == "vn") { glm::vec3 vn; ss >> vn.x >> vn.y >> vn.z; t_vn.push_back(vn); }
        else if (pr == "vt") { glm::vec2 vt; ss >> vt.x >> vt.y; t_vt.push_back(vt); }
        else if (pr == "f") {
            if (skipPart) continue;
            std::string v_s; std::vector<std::string> face;
            while (ss >> v_s) face.push_back(v_s);
            if (face.size() < 3) continue;
            for (size_t i = 1; i < face.size() - 1; i++) {
                std::string tri[] = { face[0], face[i], face[i + 1] };
                for (auto& f_str : tri) {
                    std::stringstream fss(f_str); std::string seg; std::vector<int> ids;
                    while (std::getline(fss, seg, '/')) { try { ids.push_back(seg.empty() ? 0 : std::stoi(seg)); } catch (...) { ids.push_back(0); } }
                    if (!ids.empty()) {
                        int v_idx = ids[0]; if (v_idx < 0) v_idx = (int)t_v.size() + v_idx + 1;
                        if (v_idx > 0 && v_idx <= (int)t_v.size()) {
                            mesh.vertices.push_back(t_v[v_idx - 1]);
                            mesh.texCoords.push_back((ids.size() > 1 && ids[1] > 0 && ids[1] <= (int)t_vt.size()) ? t_vt[ids[1] - 1] : glm::vec2(0));
                            mesh.normals.push_back((ids.size() > 2 && ids[2] > 0 && ids[2] <= (int)t_vn.size()) ? t_vn[ids[2] - 1] : glm::vec3(0, 1, 0));
                            mesh.indices.push_back((unsigned int)mesh.indices.size());
                        }
                    }
                }
            }
        }
    }
    mesh.setupMesh(); return true;
}

unsigned int RaceCar::loadTexture(const char* path) {
    unsigned int id; glGenTextures(1, &id); int w, h, nrComponents; // Змінено n на nrComponents
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &w, &h, &nrComponents, 0);
    if (data) {
        GLenum format = (nrComponents == 4) ? GL_RGBA : GL_RGB; // Тепер назва однакова
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D); stbi_image_free(data);
    }
    return id;
}