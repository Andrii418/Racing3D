#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <windows.h>

#include "Shader.h"
#include "Camera.h"
#include "RaceCar.h"
#include "Track.h"
#include "City.h"
#include "Model.h"      

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui_internal.h"

int current_width = 1280;
int current_height = 720;

unsigned int FBO_Scene;
unsigned int textureColorBuffer;
unsigned int RBO_DepthStencil;
unsigned int quadVAO, quadVBO;

unsigned int grassTextureID = 0;
unsigned int splashTextureID = 0;

enum GameState {
    SPLASH_SCREEN,
    MAIN_MENU,
    RACING
};

RaceCar* car = nullptr;
Camera* camera = nullptr;
Track* track = nullptr;
City* city = nullptr;

Model* kartingMap = nullptr;

bool keys[1024] = { false };
float lastFrame = 0.0f;
GameState currentState = SPLASH_SCREEN;
float splashTimer = 0.0f;
glm::vec3 carCustomColor = glm::vec3(1.0f, 0.5f, 0.2f);

bool cockpitView = false;

bool showSettings = false;
bool showCarSelect = false;
bool showTrackSelect = false;
int selectedCar = 0;
int selectedTrack = 0;
float timeOfDay = 0.75f;

float carMenuRotation = 0.0f;
glm::vec3 menuCarPosition = glm::vec3(0.0f, 0.5f, 0.0f);
float backgroundYaw = 0.0f;

void setupFramebuffer(int width, int height);

unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
        return 0;
    }

    return textureID;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    current_width = width;
    current_height = height;
    if (FBO_Scene != 0) {
        setupFramebuffer(width, height);
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    static bool handling = false;
    if (handling) return;
    handling = true;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (currentState == RACING) {
            currentState = MAIN_MENU;
            showSettings = false;
            showCarSelect = false;
            showTrackSelect = false;
        }
        else if (currentState == MAIN_MENU) {
            if (showSettings) showSettings = false;
            else if (showCarSelect) showCarSelect = false;
            else if (showTrackSelect) showTrackSelect = false;
            else glfwSetWindowShouldClose(window, true);
        }
        else {
            glfwSetWindowShouldClose(window, true);
        }
    }

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }
    if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        cockpitView = !cockpitView;
    }

    handling = false;
}

void processCarInput(float deltaTime) {
    if (!car || currentState != RACING) return;

    float currentSpeed = glm::length(car->Velocity);

    // If handbrake is held, ignore throttle input so it always brakes
    bool handbrakePressed = keys[GLFW_KEY_SPACE];

    if (keys[GLFW_KEY_W] && !handbrakePressed) car->Velocity += car->FrontVector * car->Acceleration * deltaTime;
    if (keys[GLFW_KEY_S]) car->Velocity -= car->FrontVector * car->Braking * deltaTime;

    // handbrake input
    car->Handbrake = handbrakePressed;

    // steering input for physics (A = left +1, D = right -1)
    float steeringIn = 0.0f;
    if (keys[GLFW_KEY_A]) steeringIn = 1.0f;
    if (keys[GLFW_KEY_D]) steeringIn = -1.0f;
    car->SteeringInput = steeringIn;

    if (glm::length(car->Velocity) > car->MaxSpeed)
        car->Velocity = glm::normalize(car->Velocity) * car->MaxSpeed;

    if (currentSpeed > 0.1f) {
        float turnAmount = car->TurnRate * deltaTime * 50.0f;
        if (keys[GLFW_KEY_A]) car->Yaw += turnAmount;
        if (keys[GLFW_KEY_D]) car->Yaw -= turnAmount;

        car->FrontVector.x = sin(glm::radians(car->Yaw));
        car->FrontVector.z = cos(glm::radians(car->Yaw));
        car->FrontVector.y = 0.0f;
        car->FrontVector = glm::normalize(car->FrontVector);
    }

    if (glm::any(glm::isnan(car->FrontVector)))
        car->FrontVector = glm::vec3(0.0f, 0.0f, 1.0f);
}

void setupPostProcessingQuad() {
    float quadVertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void setupFramebuffer(int width, int height) {
    if (FBO_Scene != 0) {
        glDeleteFramebuffers(1, &FBO_Scene);
        glDeleteTextures(1, &textureColorBuffer);
        glDeleteRenderbuffers(1, &RBO_DepthStencil);
    }

    glGenFramebuffers(1, &FBO_Scene);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO_Scene);

    glGenTextures(1, &textureColorBuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBuffer, 0);

    glGenRenderbuffers(1, &RBO_DepthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO_DepthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO_DepthStencil);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderSplashScreen() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(current_width, current_height));
    ImGui::Begin("Splash Screen", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground);

    if (splashTextureID != 0) {
        ImGui::GetWindowDrawList()->AddImage(
            (void*)(intptr_t)splashTextureID,
            ImVec2(0, 0),
            ImVec2(current_width, current_height)
        );
    }
    else {
        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0, 0), ImVec2(current_width, current_height),
            IM_COL32(20, 20, 20, 255));
        ImGui::SetCursorPosX((current_width - ImGui::CalcTextSize("RACING 3D").x * 2.0f) * 0.5f);
        ImGui::SetCursorPosY(current_height * 0.4f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.2f, 1.0f));
        ImGui::SetWindowFontScale(2.0f);
        ImGui::Text("RACING 3D");
        ImGui::PopStyleColor();
        ImGui::SetWindowFontScale(1.0f);
    }
    ImGui::SetCursorPosX((current_width - ImGui::CalcTextSize("Loading...").x) * 0.5f);
    ImGui::SetCursorPosY(current_height * 0.9f);
    ImGui::Text("Loading...");
    ImGui::End();
}

void RenderCarSelectMenu() {
    ImGui::SetNextWindowPos(ImVec2(current_width * 0.5f, current_height * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 450));
    ImGui::Begin("Car Select Menu", &showCarSelect,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

    ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos(),
        ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y),
        IM_COL32(0, 0, 0, 220));
    ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos(),
        ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + 10),
        IM_COL32(25, 127, 255, 255));

    ImGui::Dummy(ImVec2(0, 10));
    ImGui::TextColored(ImVec4(0.1f, 0.5f, 1.0f, 1.0f), "  [ GARAGE ]");
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Dummy(ImVec2(400, 120));
    ImGui::Spacing();

    const char* carNames[] = { "Model 3D Racer (Default)", "Off-Road Truck (Locked)" };
    ImGui::Text("Currently Selected: %s", carNames[selectedCar]);
    ImGui::Spacing();

    ImGui::Columns(2, "CarList", false);
    ImGui::SetColumnWidth(0, 250);
    if (ImGui::Selectable(carNames[0], selectedCar == 0, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0, 40))) {
        selectedCar = 0;
    }
    ImGui::NextColumn();
    ImGui::Text("Top Speed: 200 km/h\nAcceleration: High");
    ImGui::Columns(1);
    ImGui::Separator();

    ImGui::Columns(2, "CarList2", false);
    ImGui::SetColumnWidth(0, 250);
    if (ImGui::Selectable(carNames[1], selectedCar == 1, ImGuiSelectableFlags_Disabled | ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0, 40))) {
    }
    ImGui::NextColumn();
    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "LOCKED\nRequires Level 5");
    ImGui::Columns(1);
    ImGui::Separator();

    ImGui::Spacing();
    float col[3] = { carCustomColor.r, carCustomColor.g, carCustomColor.b };
    ImGui::Text("Car Color:");
    if (ImGui::ColorEdit3("##CarColorPicker", col)) {
        carCustomColor = glm::vec3(col[0], col[1], col[2]);
    }
    ImGui::Spacing();
    ImGui::Separator();
    if (ImGui::Button("BACK", ImVec2(200, 35))) {
        showCarSelect = false;
    }
    ImGui::End();
}

void RenderTrackSelectMenu() {
    ImGui::SetNextWindowPos(ImVec2(current_width * 0.5f, current_height * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 400));
    ImGui::Begin("Track Select Menu", &showTrackSelect,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

    ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos(),
        ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x,
            ImGui::GetWindowPos().y + ImGui::GetWindowSize().y),
        IM_COL32(0, 0, 0, 220));

    ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos(),
        ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x,
            ImGui::GetWindowPos().y + 10),
        IM_COL32(25, 127, 255, 255));

    ImGui::Dummy(ImVec2(0, 10));
    ImGui::TextColored(ImVec4(0.1f, 0.5f, 1.0f, 1.0f), "  [ TRACK SELECTION ]");
    ImGui::Separator();
    ImGui::Spacing();

    const char* trackNames[] = {
        "Flat Grass Arena (Level 1)",
        "Desert Canyon (Level 2)",
        "Karting GP (Level 3)"
    };
    ImGui::Text("Selected Track: %s", trackNames[selectedTrack]);

    if (ImGui::BeginListBox("##TrackList", ImVec2(-FLT_MIN, 120))) {
        if (ImGui::Selectable(trackNames[0], selectedTrack == 0)) selectedTrack = 0;
        if (ImGui::Selectable(trackNames[1], selectedTrack == 1)) selectedTrack = 1;
        if (ImGui::Selectable(trackNames[2], selectedTrack == 2)) selectedTrack = 2;
        ImGui::EndListBox();
    }

    ImGui::Spacing();
    ImGui::Text("Time of Day:");
    const char* timeNames[] = { "Night", "Dawn", "Day", "Dusk" };
    int currentTime = (int)(timeOfDay * 3.99f);
    if (ImGui::SliderInt("##TimeSlider", &currentTime, 0, 3, timeNames[currentTime])) {
        timeOfDay = (float)currentTime / 3.0f;
    }

    ImGui::Spacing();
    ImGui::Separator();
    if (ImGui::Button("BACK", ImVec2(200, 35))) {
        showTrackSelect = false;
    }
    ImGui::End();
}

void RenderSettingsMenu() {
    ImGui::SetNextWindowPos(ImVec2(current_width * 0.5f, current_height * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 360));
    ImGui::Begin("Settings Menu", &showSettings,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

    ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos(),
        ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y),
        IM_COL32(0, 0, 0, 220));
    ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos(),
        ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + 10),
        IM_COL32(25, 127, 255, 255));

    ImGui::Dummy(ImVec2(0, 10));
    ImGui::TextColored(ImVec4(0.1f, 0.5f, 1.0f, 1.0f), "  [ GENERAL SETTINGS ]");
    ImGui::Separator();
    ImGui::Spacing();

    static float volume = 0.5f;
    ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f);
    static bool vsync = true;
    ImGui::Checkbox("V-Sync Enabled", &vsync);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(0.9f,0.9f,0.6f,1.0f), "Physics Tuning");
    if (car) {
        ImGui::SliderFloat("Grip", &car->Grip, 0.0f, 1.5f);
        ImGui::SliderFloat("Aerodynamic Drag", &car->AerodynamicDrag, 0.0f, 0.2f);
        ImGui::SliderFloat("Rolling Resistance", &car->RollingResistance, 0.0f, 0.1f);
        ImGui::SliderFloat("Steering Responsiveness", &car->SteeringResponsiveness, 0.0f, 10.0f);
        ImGui::SliderFloat("Handbrake Grip Reduction", &car->HandbrakeGripReduction, 0.0f, 1.0f);
        ImGui::SliderFloat("Max Speed (m/s)", &car->MaxSpeed, 5.0f, 80.0f);
        ImGui::Checkbox("Handbrake (toggle)", &car->Handbrake);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    if (ImGui::Button("BACK", ImVec2(180, 35))) {
        showSettings = false;
    }
    ImGui::End();
}

void RenderMainMenu() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(current_width, current_height));
    ImGui::Begin("Fullscreen Background", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);

    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0, 0), ImVec2(current_width, current_height), IM_COL32(0, 0, 0, 180));
    if (splashTextureID != 0) {
        ImGui::GetWindowDrawList()->AddImage(
            (void*)(intptr_t)splashTextureID,
            ImVec2(0, 0),
            ImVec2(current_width, current_height),
            ImVec2(0, 0),
            ImVec2(1, 1),
            IM_COL32(255, 255, 255, 100)
        );
    }

    if (showSettings || showCarSelect || showTrackSelect) {
        ImGui::End();
        if (showSettings) RenderSettingsMenu();
        if (showCarSelect) RenderCarSelectMenu();
        if (showTrackSelect) RenderTrackSelectMenu();
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(current_width, current_height * 0.2f));
    ImGui::Begin("Title Panel", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0, 0), ImVec2(current_width, current_height * 0.2f), IM_COL32(0, 0, 0, 150));

    float titleSize = 4.0f;
    ImGui::SetWindowFontScale(titleSize);
    float titleWidth = ImGui::CalcTextSize("RACING 3D").x;
    ImGui::SetCursorPosX((current_width - titleWidth) * 0.5f);
    ImGui::SetCursorPosY((current_height * 0.2f - ImGui::GetTextLineHeightWithSpacing() * titleSize) * 0.5f);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.5f, 1.0f, 1.0f));
    ImGui::Text("RACING 3D");
    ImGui::PopStyleColor();

    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(current_width * 0.5f - 150.0f, ImGui::GetCursorScreenPos().y - 10.0f),
        ImVec2(current_width * 0.5f + 150.0f, ImGui::GetCursorScreenPos().y - 10.0f),
        IM_COL32(25, 127, 255, 255), 2.0f
    );
    ImGui::SetWindowFontScale(1.0f);
    ImGui::End();

    float buttonsPanelHeight = 400.0f;
    float startY = current_height * 0.2f + (current_height * 0.8f - buttonsPanelHeight) * 0.5f;

    ImGui::SetNextWindowPos(ImVec2(current_width * 0.5f, startY), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400, buttonsPanelHeight));
    ImGui::Begin("Main Menu Buttons", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground);

    ImVec2 buttonSize(350, 60);
    float buttonX = (ImGui::GetWindowSize().x - buttonSize.x) * 0.5f;
    ImGui::SetWindowFontScale(1.5f);

    ImGui::SetCursorPosX(buttonX);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.25f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.5f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.4f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.5f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    if (ImGui::Button("START RACE", buttonSize)) {
        currentState = RACING;
        if (car) {
            glm::vec3 scaleFactor(0.1f); // taki sam jak model toru
            glm::vec3 startLeft(-127.81f, 0.0f, 204.38f);
            glm::vec3 startRight(-58.75f, 0.0f, 203.17f);

            // punkt startowy na środku linii startu
            glm::vec3 startPos = (startLeft + startRight) * 0.5f;

            // kierunek wzdłuż toru
            glm::vec3 dir = glm::normalize(startRight - startLeft);

            // przesunięcie samochodu daleko w tył
            float offsetBack = 95.0f;
            startPos -= dir * offsetBack;

            // kierunek prostopadły do toru (w prawo)
            glm::vec3 rightVec = glm::normalize(glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f)));

            // przesunięcie w lewo, aby samochód nie wchodził w beton
            float offsetSide = 5.0f; // zwiększ jeśli trzeba więcej
            startPos -= rightVec * offsetSide;

            // dopasowanie do skali toru
            startPos *= scaleFactor;

            // ustawienie samochodu
            float startYaw = glm::degrees(atan2(dir.x, dir.z));
            car->Position = startPos;
            car->Velocity = glm::vec3(0.0f);
            car->Yaw = startYaw;
            car->FrontVector = glm::normalize(glm::vec3(sin(glm::radians(startYaw)), 0.0f, cos(glm::radians(startYaw))));

        }
    }
    ImGui::PopStyleColor(5);
    ImGui::Spacing(); ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.1f, 0.5f, 1.0f, 1.0f));
    ImGui::SetCursorPosX(buttonX);
    if (ImGui::Button("SELECT CAR", buttonSize)) showCarSelect = true;
    ImGui::SetCursorPosX(buttonX);
    if (ImGui::Button("SELECT TRACK", buttonSize)) showTrackSelect = true;
    ImGui::SetCursorPosX(buttonX);
    if (ImGui::Button("SETTINGS", buttonSize)) showSettings = true;
    ImGui::PopStyleColor(1);

    ImGui::SetCursorPosX(buttonX);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));

    if (ImGui::Button("EXIT GAME", buttonSize)) {
        GLFWwindow* window = glfwGetCurrentContext();
        glfwSetWindowShouldClose(window, true);
    }
    ImGui::PopStyleColor(4);
    ImGui::SetWindowFontScale(1.0f);
    ImGui::End();
    ImGui::End();
}

int main() {

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(current_width, current_height, "Racing3D", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;
    glEnable(GL_DEPTH_TEST);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 12.0f;
    style.FrameBorderSize = 2.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.94f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.9f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.1f, 0.5f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.05f, 0.45f, 0.9f, 1.0f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.1f, 0.5f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_Text] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);

    Shader carTrackShader("shaders/phong.vert", "shaders/phong.frag");

    Shader postProcessShader("shaders/postprocess.vert", "shaders/postprocess.frag");
    postProcessShader.use();
    postProcessShader.setInt("screenTexture", 0);

    setupPostProcessingQuad();
    setupFramebuffer(current_width, current_height);

    stbi_set_flip_vertically_on_load(false);
    splashTextureID = loadTexture("assets/Tlo.png");

    car = new RaceCar(glm::vec3(0.0f, 0.1f, 0.0f));
    car->loadAssets();
    camera = new Camera(glm::vec3(0.0f, 3.0f, 5.0f));
    track = new Track();
    city = new City(glm::vec3(0.0f, 0.0f, 0.0f));
    city->Scale = glm::vec3(0.3f);
    city->loadModel();

    kartingMap = new Model("assets/karting/gp.obj");

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (currentState == SPLASH_SCREEN) {
            splashTimer += deltaTime;
            if (splashTimer > 2.5f) currentState = MAIN_MENU;
        }
        else if (currentState == MAIN_MENU) {
            carMenuRotation += 90.0f * deltaTime;
            if (carMenuRotation > 360.0f) carMenuRotation -= 360.0f;
            backgroundYaw += 3.0f * deltaTime;
            if (backgroundYaw > 360.0f) backgroundYaw -= 360.0f;
        }
        else if (currentState == RACING) {
            processCarInput(deltaTime);
            car->Update(deltaTime);
        }

        if (camera && car) {
            if (currentState == RACING) {
                if (cockpitView) {
                    glm::vec3 cockpitOffset(0.0f, 0.7f, 0.2f);
                    camera->Position = car->Position
                        + car->FrontVector * cockpitOffset.z
                        + glm::vec3(0.0f, cockpitOffset.y, 0.0f);
                    camera->Front = glm::normalize(car->FrontVector);
                }
                else {
                    camera->FollowCar(car->Position, car->FrontVector);
                }
            }
            else {
                camera->Position = glm::vec3(0.0f, 2.5f, 5.0f);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, FBO_Scene);
        glEnable(GL_DEPTH_TEST);
        glViewport(0, 0, current_width, current_height);

        glm::vec3 skyColor = glm::mix(glm::vec3(0.05f, 0.05f, 0.15f), glm::vec3(0.2f, 0.3f, 0.3f), timeOfDay);
        glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        carTrackShader.use();
        if (camera) {
            carTrackShader.setMat4("view", camera->GetViewMatrix());
            carTrackShader.setMat4("projection", camera->GetProjectionMatrix((float)current_width / (float)current_height));
        }
        glm::vec3 lightPos(5.0f, 10.0f, 5.0f);
        carTrackShader.setVec3("lightPos", lightPos);

        glm::vec3 lightColor = glm::mix(glm::vec3(0.1f), glm::vec3(1.0f), timeOfDay);
        carTrackShader.setVec3("lightColor", lightColor);

        if (camera)
            carTrackShader.setVec3("viewPos", camera->Position);

        if (selectedTrack == 0 && track) {
            track->Draw(carTrackShader);
        }
        else if (selectedTrack == 1 && city) {
            glm::mat4 model = glm::mat4(1.0f);
            carTrackShader.setMat4("model", model);
            city->Draw(carTrackShader);
        }
        else if (selectedTrack == 2 && kartingMap) {
            
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.1f)); 

            carTrackShader.setMat4("model", model);

            kartingMap->Draw(carTrackShader);
        }

        if (car) {
            carTrackShader.setVec3("objectColor", carCustomColor);

            if (currentState == MAIN_MENU && showCarSelect) {
                car->Draw(carTrackShader, menuCarPosition, carMenuRotation);
            }
            else if (currentState == RACING) {
                car->Draw(carTrackShader);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glViewport(0, 0, current_width, current_height);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        postProcessShader.use();
        bool menuActive = (currentState == MAIN_MENU || currentState == SPLASH_SCREEN);
        postProcessShader.setBool("isBlur", menuActive);

        glBindVertexArray(quadVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (currentState == SPLASH_SCREEN) RenderSplashScreen();
        else if (currentState == MAIN_MENU) RenderMainMenu();
        else if (currentState == RACING && car) {
            ImGui::SetNextWindowPos(ImVec2(10, current_height - 60));
            ImGui::Begin("HUD", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
            ImGui::Text("Speed: %.1f km/h", glm::length(car->Velocity) * 3.6f);
            ImGui::Text("View: %s", cockpitView ? "Cockpit" : "Chase");
            ImGui::End();

            ImGui::SetNextWindowPos(ImVec2(10, 10));
            ImGui::Begin("RaceMenu", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
            if (ImGui::Button("BACK TO MAIN MENU", ImVec2(200, 40))) {
                currentState = MAIN_MENU;
                showSettings = false;
                showCarSelect = false;
                showTrackSelect = false;
            }
            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (splashTextureID != 0) glDeleteTextures(1, &splashTextureID);
    glDeleteFramebuffers(1, &FBO_Scene);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);

    delete car;
    delete camera;
    delete track;
    delete city;
    delete kartingMap; 

    glfwTerminate();
    return 0;
}