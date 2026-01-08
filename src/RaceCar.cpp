#include "RaceCar.h"
#include "Shader.h"
#include "stb_image.h" 
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>


void CarMesh::setupMesh() {
    if (vertices.empty()) return;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    std::vector<float> data;
    for (size_t i = 0; i < vertices.size(); ++i) {
        data.push_back(vertices[i].x); data.push_back(vertices[i].y); data.push_back(vertices[i].z);
        data.push_back(normals[i].x);  data.push_back(normals[i].y);  data.push_back(normals[i].z);
        data.push_back(texCoords[i].x); data.push_back(texCoords[i].y);
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}


RaceCar::RaceCar(glm::vec3 startPosition)
    : Position(startPosition),
    PreviousPosition(startPosition),
    Velocity(0.0f),
    Yaw(0.0f),
    textureID(0),
    Throttle(0.0f), ThrottleInput(0.0f), ThrottleResponse(3.5f)
{
    FrontVector = glm::vec3(0.0f, 0.0f, 1.0f);
}

bool RaceCar::loadAssets() {
    textureID = loadTexture("assets/cars/OBJ format/Textures/colormap.png");

    if (!loadObj("assets/cars/OBJ format/race.obj", bodyMesh)) {
        std::cout << "Failed to load race.obj" << std::endl;
        return false;
    }

    if (!loadObj("assets/cars/OBJ format/wheel-racing.obj", wheelMesh)) {
        std::cout << "Failed to load wheel-racing.obj" << std::endl;
        return false;
    }

    return true;
}


void RaceCar::Update(float deltaTime) {
    PreviousPosition = Position;

    // Compute forward vector from yaw to ensure consistency
    FrontVector.x = sin(glm::radians(Yaw));
    FrontVector.z = cos(glm::radians(Yaw));
    FrontVector.y = 0.0f;
    FrontVector = glm::normalize(FrontVector);

    // Smooth throttle input toward desired input
    float tResponse = glm::clamp(ThrottleResponse * deltaTime, 0.0f, 1.0f);
    Throttle = glm::mix(Throttle, ThrottleInput, tResponse);

    // Decompose velocity into forward and lateral components
    glm::vec3 forward = FrontVector;
    float speed = glm::length(Velocity);
    glm::vec3 velLong = forward * glm::dot(Velocity, forward);
    glm::vec3 velLat = Velocity - velLong;

    // Effective grip starts as base Grip
    float effectiveGrip = Grip;

    // Apply throttle acceleration to forward component
    float forwardSpeed = glm::dot(Velocity, forward);

    // Positive throttle -> accelerate forward
    if (Throttle > 0.01f && !Handbrake) {
        float desiredAccel = Throttle * Acceleration; // m/s^2
        forwardSpeed += desiredAccel * deltaTime;
    }
    // Negative throttle -> reverse / braking input
    else if (Throttle < -0.01f) {
        float desiredBrake = -Throttle * Braking;
        forwardSpeed -= desiredBrake * deltaTime;
    }

    // If space (handbrake) is held it will be handled below

    // Prevent instant overspeed: clamp forwardSpeed to MaxSpeed
    if (forwardSpeed > MaxSpeed) forwardSpeed = MaxSpeed;
    if (forwardSpeed < -MaxSpeed * 0.5f) forwardSpeed = -MaxSpeed * 0.5f; // limited reverse

    velLong = forward * forwardSpeed;

    // If handbrake is held: smoothly brake forward velocity and enable sliding
    if (Handbrake) {
        float newForwardSpeed = forwardSpeed * powf(HandbrakeDeceleration, deltaTime);
        if (newForwardSpeed < 0.0f) newForwardSpeed = 0.0f;
        velLong = forward * newForwardSpeed;

        // reduce grip to allow sliding
        effectiveGrip *= (1.0f - HandbrakeGripReduction);

        // lateral slide based on steering input (user steers the slide direction)
        float steer = glm::clamp(SteeringInput, -1.0f, 1.0f);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        float lateralAmount = 0.25f * (glm::clamp(newForwardSpeed, 0.0f, MaxSpeed) / (MaxSpeed + 0.1f));
        velLat += right * (-steer) * lateralAmount;

        // prevent handbrake from increasing total speed: cap combined magnitude to previous speed, with small energy loss
        glm::vec3 combined = velLong + velLat;
        float combinedMag = glm::length(combined);
        if (combinedMag > 0.001f) {
            float prevSpeed = speed;
            if (combinedMag > prevSpeed) {
                combined *= (prevSpeed / combinedMag);
                // apply small energy loss to simulate braking during drift
                combined *= 0.99f;
            }
            // decompose back
            velLong = forward * glm::dot(combined, forward);
            velLat = combined - velLong;
        }
    }

    // Recompose velocity before drag
    Velocity = velLong + velLat;

    // Aerodynamic drag (quadratic) applied to current velocity
    speed = glm::length(Velocity);
    if (speed > 0.001f) {
        glm::vec3 drag = -glm::normalize(Velocity) * (AerodynamicDrag * speed * speed);
        Velocity += drag * deltaTime;
    }

    // Rolling resistance (linear damping)
    Velocity -= Velocity * RollingResistance * deltaTime;

    // Recompute decomposition after drag/resistance
    speed = glm::length(Velocity);
    velLong = forward * glm::dot(Velocity, forward);
    velLat = Velocity - velLong;

    // Damp lateral velocity according to effective grip (less damping when handbrake reduces grip)
    velLat *= (1.0f - glm::clamp(effectiveGrip * deltaTime, 0.0f, 0.95f));

    // Recompose velocity
    Velocity = velLong + velLat;

    // Integrate position
    Position += Velocity * deltaTime;

    // Align velocity toward forward slightly (simulates tire aligning) when not sliding wildly
    speed = glm::length(Velocity);
    if (speed > 0.5f && !Handbrake) {
        float alignFactor = SteeringResponsiveness * deltaTime * glm::clamp(Grip * (1.0f - speed / (MaxSpeed + 0.001f)), 0.0f, 1.0f);
        glm::vec3 velDir = (speed > 0.001f) ? glm::normalize(Velocity) : forward;
        glm::vec3 newDir = glm::normalize(glm::mix(velDir, forward, alignFactor));

        // Slight speed loss when changing direction
        float dotp = glm::clamp(glm::dot(velDir, newDir), -1.0f, 1.0f);
        float angle = acos(dotp);
        float angleDeg = glm::degrees(angle);
        float speedLoss = glm::min(0.05f, angleDeg / 180.0f * 0.2f);
        float newSpeed = speed * (1.0f - speedLoss);

        Velocity = newDir * newSpeed;
    }

    // Minor damping similar to original
    Velocity *= 0.98f;

    // Keep grounded
    Position.y = 0.0f;

    HandleCollision();

    float s = glm::length(Velocity);
    if (s > 0.1f) {
        float forwardSpeedNow = glm::dot(Velocity, forward);
        WheelRotation += forwardSpeedNow * deltaTime * 10.0f;
    }
}


void RaceCar::HandleCollision() {

    const float R = 1.0f;


    bool collided = false;

    if (Position.x > 100.0f) collided = true;
    if (Position.x < -100.0f) collided = true;
    if (Position.z > 100.0f) collided = true;
    if (Position.z < -100.0f) collided = true;

    if (collided) {
        Position = PreviousPosition;
        Velocity = glm::vec3(0.0f);
    }
}


glm::mat4 RaceCar::GetModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, Position);
    model = glm::rotate(model, glm::radians(Yaw), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(0.15f));
    return model;
}


void RaceCar::Draw(const Shader& shader, glm::vec3 pos, float yaw) const {

    glm::mat4 model = glm::mat4(1.0f);

    bool isCustomDraw = (glm::length(pos) > 0.001f || std::abs(yaw) > 0.001f);

    if (isCustomDraw) {
        model = glm::translate(model, pos);
        model = glm::rotate(model, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f));
    }
    else {
        model = GetModelMatrix();
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    shader.setBool("useTexture", true);
    shader.setInt("texture_diffuse1", 0);

    shader.setMat4("model", model);

    glBindVertexArray(bodyMesh.VAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)bodyMesh.indices.size(), GL_UNSIGNED_INT, 0);

    glm::vec3 wheelOffsets[] = {
        {0.45f, 0.15f, 0.42f},
        {-0.45f, 0.15f, 0.42f},
        {0.45f, 0.15f, -0.42f},
        {-0.45f, 0.15f, -0.42f}
    };

    glBindVertexArray(wheelMesh.VAO);
    for (int i = 0; i < 4; i++) {
        glm::mat4 wheelModel = model;
        wheelModel = glm::translate(wheelModel, wheelOffsets[i]);
        wheelModel = glm::rotate(wheelModel, glm::radians(WheelRotation), glm::vec3(1.0f, 0.0f, 0.0f));

        shader.setMat4("model", wheelModel);
        glDrawElements(GL_TRIANGLES, (GLsizei)wheelMesh.indices.size(), GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
}


bool RaceCar::loadObj(const std::string& path, CarMesh& mesh) {

    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::vector<glm::vec3> temp_v;
    std::vector<glm::vec3> temp_vn;
    std::vector<glm::vec2> temp_vt;

    std::string line;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string prefix;
        ss >> prefix;

        if (prefix == "v") {
            glm::vec3 v; ss >> v.x >> v.y >> v.z; temp_v.push_back(v);
        }
        else if (prefix == "vn") {
            glm::vec3 vn; ss >> vn.x >> vn.y >> vn.z; temp_vn.push_back(vn);
        }
        else if (prefix == "vt") {
            glm::vec2 vt; ss >> vt.x >> vt.y; temp_vt.push_back(vt);
        }
        else if (prefix == "f") {
            std::string v_str[4];
            int count = 0;
            while (ss >> v_str[count] && count < 4) count++;

            for (int i = 0; i < count - 2; i++) {
                std::string faces[] = { v_str[0], v_str[i + 1], v_str[i + 2] };

                for (auto& f : faces) {
                    std::stringstream fss(f);
                    std::string segment;
                    std::vector<std::string> inds;

                    while (std::getline(fss, segment, '/'))
                        inds.push_back(segment);

                    int vi = std::stoi(inds[0]) - 1;
                    mesh.vertices.push_back(temp_v[vi]);

                    if (inds.size() > 1 && !inds[1].empty()) {
                        int vti = std::stoi(inds[1]) - 1;
                        mesh.texCoords.push_back(temp_vt[vti]);
                    }
                    else mesh.texCoords.push_back(glm::vec2(0));

                    if (inds.size() > 2 && !inds[2].empty()) {
                        int vni = std::stoi(inds[2]) - 1;
                        mesh.normals.push_back(temp_vn[vni]);
                    }
                    else mesh.normals.push_back(glm::vec3(0, 1, 0));

                    mesh.indices.push_back((unsigned int)mesh.indices.size());
                }
            }
        }
    }

    if (mesh.normals.size() != mesh.vertices.size())
        calculateNormals(mesh);

    mesh.setupMesh();
    return true;
}

void RaceCar::calculateNormals(CarMesh& mesh) {}

unsigned int RaceCar::loadTexture(const char* path) {

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);

    if (data) {
        GLenum format = (nrComponents == 4) ? GL_RGBA : GL_RGB;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        stbi_image_free(data);
    }
    else {
        std::cout << "Failed loading texture: " << path << std::endl;
    }

    return textureID;
}
