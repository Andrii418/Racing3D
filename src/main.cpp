#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <windows.h>

#include "ProfileManager.h"

struct CarData {
    std::string id;
    std::string name;
    std::string bodyPath;
    std::string wheelFrontPath;
    std::string wheelBackPath;
    int price;
    float wheelFrontX;
    float wheelBackX;
    float wheelZ;
};

// Оновлений каталог з налаштуваннями осей згідно з твоїми запитами
std::vector<CarData> garage = {
    // Race: трошечки розширили по довжині (0.42 -> 0.48)
    {"race", "Racer GT", "assets/cars/OBJ format/race.obj", "assets/cars/OBJ format/wheel-racing.obj", "", 0, 0.45f, 0.45f, 0.48f},

    // Police: значно розширили по довжині (+ ширина колеса) (0.45 -> 0.60)
    {"police", "Police Car", "assets/cars/OBJ format/police.obj", "assets/cars/OBJ format/wheel-default.obj", "", 1000, 0.48f, 0.48f, 0.83f},

    // Ambulance: розширили по довжині, але менше ніж поліцію (0.70 -> 0.80)
    {"ambulance", "Ambulance", "assets/cars/OBJ format/ambulance.obj", "assets/cars/OBJ format/wheel-default.obj", "", 1200, 0.52f, 0.52f, 0.95f},

    // Taxi: розширили по довжині так само як амбуланс (+0.10 до бази) (0.45 -> 0.55)
    {"taxi", "Taxi", "assets/cars/OBJ format/taxi.obj", "assets/cars/OBJ format/wheel-default.obj", "", 800, 0.48f, 0.48f, 0.75f},

    // Cargo Van: капельку звузили по ширині (0.55 -> 0.52)
    {"van", "Cargo Van", "assets/cars/OBJ format/van.obj", "assets/cars/OBJ format/wheel-truck.obj", "", 900, 0.52f, 0.52f, 0.78f},

    // Heavy Truck: звузили по ширині більше ніж вен (0.60 -> 0.54)
    {"truck", "Heavy Truck", "assets/cars/OBJ format/truck.obj", "assets/cars/OBJ format/wheel-truck.obj", "", 1500, 0.54f, 0.54f, 0.82f},

    // Sport Hatch: значно розширили по довжині (0.38 -> 0.58)
    {"hatchback", "Sport Hatch", "assets/cars/OBJ format/hatchback-sports.obj", "assets/cars/OBJ format/wheel-racing.obj", "", 500, 0.45f, 0.45f, 0.82f},

    // Tractor: капельку розширили по довжині (0.55 -> 0.60)
    {"tractor", "Tractor 500", "assets/cars/OBJ format/tractor.obj", "assets/cars/OBJ format/wheel-tractor-front.obj", "assets/cars/OBJ format/wheel-tractor-back.obj", 2000, 0.55f, 0.75f, 0.60f}
};

ProfileManager playerProfile;

// Undefine Windows macros that may conflict with std::min/std::max
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include "Shader.h"
#include "Camera.h"
#include "RaceCar.h"
#include "Track.h"
#include "TrackCollision.h"
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
RaceCar* aiCar = nullptr; // AI companion car

std::vector<glm::vec3> aiWaypoints;
int aiCurrentWaypoint = 0;
float aiWaypointRadius = 2.7f; // jak blisko musi być AI

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
int selectedTrack = 2; // Default to Karting GP (Index 2)
float timeOfDay = 0.75f;

float carMenuRotation = 0.0f;
glm::vec3 menuCarPosition = glm::vec3(0.0f, 0.5f, 0.0f);
float backgroundYaw = 0.0f;

// ===== RACE TIMER / LAP LOGIC =====
bool raceTimerActive = false;
float raceTimeLeft = 300.0f;
float raceElapsedTime = 0.0f; // To track how long we have been racing for Game Over screen
bool raceFinished = false;
bool raceWon = false; // player won

// ===== LAP SYSTEM =====
int currentLap = 1;

// NEW: Lap selection logic
// 0 -> 1 lap, 1 -> 3 laps, 2 -> 10 laps
int selectedLapOption = 0;
int totalLaps = 1;

// MONEY SESSION
int sessionMoney = 0; // Money earned in current race (accumulated)

glm::vec3 lapStartPosition;
float lapFinishRadius = 2.0f; // трохи збільшив радіус для надійності
bool leftStartZone = false;   // щоб не фінішило одразу


// AI race state
int aiCurrentLap = 1;
bool aiLeftStartZone = false;
bool aiRaceFinished = false;
bool aiRaceWon = false;

// Track forward direction in world units (initialized at race start)
glm::vec3 trackForward = glm::vec3(0.0f, 0.0f, 1.0f);

// New: race countdown state
bool raceCountdownActive = false;
float raceCountdown = 0.0f; // seconds

// GO! animation
bool showGoAnimation = false;
float goTimer = 0.0f;
const float goDuration = 0.8f;

void setupFramebuffer(int width, int height);

unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format = GL_RGB; // Встановлюємо значення за замовчуванням
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

    // If countdown or GO animation active, ignore player input until it finishes
    if (raceCountdownActive || showGoAnimation) return;

    float currentSpeed = glm::length(car->Velocity);

    // If handbrake is held
    bool handbrakePressed = keys[GLFW_KEY_SPACE];

    // Throttle input: W = forward, S = reverse/brake
    float desiredThrottle = 0.0f;
    if (handbrakePressed) {
        desiredThrottle = 0.0f; // disable throttle when handbrake is held
    }
    else {
        if (keys[GLFW_KEY_W]) desiredThrottle = 1.0f;
        else if (keys[GLFW_KEY_S]) desiredThrottle = -1.0f;
    }

    car->ThrottleInput = desiredThrottle;

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
    ImGui::SetNextWindowSize(ImVec2(600, 500));
    ImGui::Begin("Garage", &showCarSelect, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

    ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1), "WALLET: %d $", playerProfile.balance);
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::BeginChild("CarList", ImVec2(0, 380), true)) {
        for (int i = 0; i < garage.size(); i++) {
            ImGui::PushID(i);
            bool unlocked = playerProfile.isUnlocked(garage[i].id);
            bool isCurrent = (playerProfile.currentCarId == garage[i].id);

            ImGui::BeginGroup();
            ImGui::Text("%s", garage[i].name.c_str());

            if (unlocked) {
                if (isCurrent) {
                    ImGui::TextColored(ImVec4(0, 1, 0, 1), "SELECTED");
                }
                else {
                    if (ImGui::Button("USE", ImVec2(100, 30))) {
                        playerProfile.currentCarId = garage[i].id;
                        car->WheelFrontX = garage[i].wheelFrontX;
                        car->WheelBackX = garage[i].wheelBackX;
                        car->WheelZ = garage[i].wheelZ;
                        car->loadAssets(garage[i].bodyPath, garage[i].wheelFrontPath, garage[i].wheelBackPath);
                        playerProfile.save();
                    }
                }
            }
            else {
                std::string btnText = "BUY (" + std::to_string(garage[i].price) + "$)";
                if (ImGui::Button(btnText.c_str(), ImVec2(100, 30))) {
                    if (playerProfile.buyCar(garage[i].id, garage[i].price)) {
                        // успішно куплено
                    }
                }
            }
            ImGui::EndGroup();
            ImGui::Separator();
            ImGui::PopID();
        }
        ImGui::EndChild();
    }

    if (ImGui::Button("BACK TO MENU", ImVec2(-1, 40))) showCarSelect = false;
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
        "Flat Grass Arena",
        "Desert Canyon",
        "Karting GP"
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

    ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.6f, 1.0f), "Physics Tuning");
    if (car) {
        ImGui::SliderFloat("Grip", &car->Grip, 0.0f, 1.5f);
        ImGui::SliderFloat("Aerodynamic Drag", &car->AerodynamicDrag, 0.0f, 0.2f);
        ImGui::SliderFloat("Rolling Resistance", &car->RollingResistance, 0.0f, 0.1f);
        ImGui::SliderFloat("Steering Responsiveness", &car->SteeringResponsiveness, 0.0f, 10.0f);
        ImGui::SliderFloat("Handbrake Grip Reduction", &car->HandbrakeGripReduction, 0.0f, 1.0f);
        ImGui::SliderFloat("Max Speed (m/s)", &car->MaxSpeed, 5.0f, 18.0f);
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

    // ========== NEW LAP SELECTOR UI ==========
    float radioStart = buttonX;
    ImGui::SetCursorPosX(radioStart);
    ImGui::Text("LAPS TO RACE:");

    ImGui::SetCursorPosX(radioStart);
    if (ImGui::RadioButton("1 Lap", &selectedLapOption, 0)) {}
    ImGui::SameLine();
    if (ImGui::RadioButton("3 Laps", &selectedLapOption, 1)) {}
    ImGui::SameLine();
    if (ImGui::RadioButton("10 Laps", &selectedLapOption, 2)) {}

    ImGui::Spacing(); ImGui::Spacing();
    // =========================================

    ImGui::SetCursorPosX(buttonX);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.25f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.5f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.4f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.5f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    if (ImGui::Button("START RACE", buttonSize)) {

        currentState = RACING;

        // Apply selected laps and time logic
        if (selectedLapOption == 0) {
            totalLaps = 1;
            raceTimeLeft = 60.0f; // 1 minute
        }
        else if (selectedLapOption == 1) {
            totalLaps = 3;
            raceTimeLeft = 180.0f; // 3 minutes
        }
        else if (selectedLapOption == 2) {
            totalLaps = 10;
            raceTimeLeft = 600.0f; // 10 minutes
        }
        else {
            totalLaps = 1;
            raceTimeLeft = 60.0f;
        }

        if (car) {

            glm::vec3 scaleFactor(0.1f); // taki sam jak model toru

            glm::vec3 startLeft(-127.81f, 0.0f, 204.38f);
            glm::vec3 startRight(-58.75f, 0.0f, 203.17f);

            // punkt startowy na середину лінії старту
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
            // przesunięcie o dodatkowe 0.2 metra w lewo (w jednostkach gry)
            startPos -= rightVec * 0.3f;

            // ustawienie samochodu
            float startYaw = glm::degrees(atan2(dir.x, dir.z));

            car->Position = startPos;


            // ===== RACE RESET =====
            lapStartPosition = car->Position;
            // raceTimeLeft is already set above
            raceElapsedTime = 0.0f;
            sessionMoney = 0; // Reset accumulated money

            raceFinished = false;
            raceWon = false;
            leftStartZone = false;
            raceTimerActive = false; // стартує після GO

            currentLap = 1;

            // AI car race state reset
            aiCurrentLap = 1;
            aiLeftStartZone = false;
            aiRaceFinished = false;
            aiRaceWon = false;

            trackForward = glm::normalize(dir); // set initial track forward direction

            car->Velocity = glm::vec3(0.0f);
            car->Yaw = startYaw;
            car->FrontVector = glm::normalize(glm::vec3(sin(glm::radians(startYaw)), 0.0f, cos(glm::radians(startYaw))));

            // === AI START BARDZO BLISKO GRACZA ===

            glm::vec3 forward = glm::normalize(car->FrontVector);

            // Wektor w LEWO względem kierunku jazdy
            glm::vec3 leftVec = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), forward));

            // Odległość AI od gracza
            float sideOffset = 0.3f;  // pół metra obok
            float backOffset = 0.0f;  // równo w tył

            // Pozycja startowa AI
            glm::vec3 aiStartPos = car->Position + leftVec * sideOffset - forward * backOffset;

            // Ustawienia AI
            aiCar->Position = aiStartPos;
            aiCar->Velocity = glm::vec3(0.0f);
            aiCar->Yaw = car->Yaw;
            aiCar->FrontVector = car->FrontVector;

            // AI zaczyna od pierwszego waypointa
            aiCurrentWaypoint = 0;
        }

        // start the 3-2-1 countdown
        raceCountdownActive = true;
        raceCountdown = 3.0f;
        showGoAnimation = false;

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

// Mini-map drawing helper
static void DrawMiniMap(ImDrawList* draw, const ImVec2& topLeft, const ImVec2& size, const glm::vec3& playerPos, const glm::vec3& aiPos) {
    using namespace glm;
    const auto& walls = TrackCollision::GetWalls();
    if (walls.empty()) return;

    // 1. Obliczanie granic (bez zmian)
    float minX = FLT_MAX, maxX = -FLT_MAX, minZ = FLT_MAX, maxZ = -FLT_MAX;
    for (const auto& w : walls) {
        minX = std::min({ minX, w.start.x, w.end.x });
        maxX = std::max({ maxX, w.start.x, w.end.x });
        minZ = std::min({ minZ, w.start.y, w.end.y });
        maxZ = std::max({ maxZ, w.start.y, w.end.y });
    }
    minX = std::min({ minX, playerPos.x, aiPos.x });
    maxX = std::max({ maxX, playerPos.x, aiPos.x });
    minZ = std::min({ minZ, playerPos.z, aiPos.z });
    maxZ = std::max({ maxZ, playerPos.z, aiPos.z });

    float worldW = std::max(0.001f, maxX - minX);
    float worldH = std::max(0.001f, maxZ - minZ);

    // 2. KLUCZ: Przy obrocie o 90 stopni zamieniamy worldW z worldH przy liczeniu skali!
    float pad = 0.05f;
    float scaleX = (size.x * (1.0f - 2.0f * pad)) / worldH; // Tu używamy worldH
    float scaleY = (size.y * (1.0f - 2.0f * pad)) / worldW; // Tu używamy worldW
    float scale = std::min(scaleX, scaleY);

    // Centrowanie po obrocie
    float extraX = (size.x - worldH * scale) * 0.5f;
    float extraY = (size.y - worldW * scale) * 0.5f;

    // 3. NOWA LAMBDA - Obrót o 90 stopni w prawo
    auto WorldToScreen = [&](float wx, float wz) -> ImVec2 {
        // Normalizacja (0.0 do 1.0)
        float nx = (wx - minX) / worldW;
        float nz = (wz - minZ) / worldH;

        // OBRÓT: 
        // Ekranowe X = (1.0 - nz) -> czyli przód (maxZ) będzie po prawej
        // Ekranowe Y = nx        -> czyli prawo (maxX) będzie na dole
        float rx = (1.0f - nz) * (worldH * scale) + extraX;
        float ry = nx * (worldW * scale) + extraY;

        return ImVec2(topLeft.x + rx, topLeft.y + ry);
        };

    // 4. Rysowanie ścian
    for (const auto& w : walls) {
        ImVec2 a = WorldToScreen(w.start.x, w.start.y);
        ImVec2 b = WorldToScreen(w.end.x, w.end.y);
        draw->AddLine(a, b, IM_COL32(180, 180, 180, 220), 2.0f);
    }

    // 5. Rysowanie graczy
    ImVec2 pPos = WorldToScreen(playerPos.x, playerPos.z);
    ImVec2 aiP = WorldToScreen(aiPos.x, aiPos.z);

    float markerR = std::max(3.0f, std::min(size.x, size.y) * 0.03f);
    draw->AddCircleFilled(pPos, markerR, IM_COL32(255, 160, 40, 255));
    draw->AddCircleFilled(aiP, markerR, IM_COL32(40, 220, 100, 220));
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

    /*car = new RaceCar(glm::vec3(0.0f, 0.1f, 0.0f));
    car->loadAssets();*/
    playerProfile.load(); // Завантажуємо баланс та збережену машину

    car = new RaceCar(glm::vec3(0.0f, 0.1f, 0.0f));

    // Знаходимо в гаражі збережену машину і завантажуємо її
    //bool loaded = false;
    //for (auto& c : garage) {
    //    if (c.id == playerProfile.currentCarId) {
    //        car->loadAssets(c.bodyPath, c.wheelPath);
    //        loaded = true;
    //        break;
    //    }
    //}
    //if (!loaded) car->loadAssets(garage[0].bodyPath, garage[0].wheelPath); // default
    // Знаходимо в гаражі збережену машину і завантажуємо її
    // БЕЗПЕЧНЕ завантаження збереженої машини при старті
    bool loaded = false;
    for (const auto& c : garage) {
        if (c.id == playerProfile.currentCarId) {
            car->WheelFrontX = c.wheelFrontX;
            car->WheelBackX = c.wheelBackX;
            car->WheelZ = c.wheelZ;
            if (car->loadAssets(c.bodyPath, c.wheelFrontPath, c.wheelBackPath)) loaded = true;
            break;
        }
    }

    if (!loaded && !garage.empty()) {
        car->WheelFrontX = garage[0].wheelFrontX;
        car->WheelBackX = garage[0].wheelBackX;
        car->WheelZ = garage[0].wheelZ;
        car->loadAssets(garage[0].bodyPath, garage[0].wheelFrontPath, garage[0].wheelBackPath);
    }

    // create AI companion car positioned slightly behind/to the left
    aiCar = new RaceCar(car->Position + glm::vec3(-3.0f, 0.0f, -3.0f));
    //aiCar->loadAssets();
    // Даємо AI стандартну модель гоночного авто
    aiCar->loadAssets("assets/cars/OBJ format/race.obj", "assets/cars/OBJ format/wheel-racing.obj");

    aiWaypoints.clear();

    aiWaypoints.push_back(glm::vec3(-17.4033f, 0.0f, 19.7189f));
    aiWaypoints.push_back(glm::vec3(-15.3660f, 0.0f, 19.6832f));
    aiWaypoints.push_back(glm::vec3(-13.7321f, 0.0f, 19.6545f));
    aiWaypoints.push_back(glm::vec3(-11.6130f, 0.0f, 19.6174f));
    aiWaypoints.push_back(glm::vec3(-9.36917f, 0.0f, 19.5781f));
    aiWaypoints.push_back(glm::vec3(-5.78419f, 0.0f, 19.5153f));
    aiWaypoints.push_back(glm::vec3(-5.31329f, 0.0f, 19.4698f));
    aiWaypoints.push_back(glm::vec3(-4.38402f, 0.0f, 19.1239f));
    aiWaypoints.push_back(glm::vec3(-4.14590f, 0.0f, 18.5770f));
    aiWaypoints.push_back(glm::vec3(-3.85840f, 0.0f, 17.6733f));
    aiWaypoints.push_back(glm::vec3(-3.58509f, 0.0f, 16.3736f));
    aiWaypoints.push_back(glm::vec3(-3.60564f, 0.0f, 15.2653f));
    aiWaypoints.push_back(glm::vec3(-3.66244f, 0.0f, 13.6701f));
    aiWaypoints.push_back(glm::vec3(-3.97440f, 0.0f, 12.8013f));
    aiWaypoints.push_back(glm::vec3(-4.92762f, 0.0f, 11.5202f));
    aiWaypoints.push_back(glm::vec3(-5.81082f, 0.0f, 11.2623f));
    aiWaypoints.push_back(glm::vec3(-7.50787f, 0.0f, 11.1497f));
    aiWaypoints.push_back(glm::vec3(-9.61396f, 0.0f, 11.0101f));
    aiWaypoints.push_back(glm::vec3(-12.5933f, 0.0f, 10.8900f));
    aiWaypoints.push_back(glm::vec3(-13.8865f, 0.0f, 11.0711f));
    aiWaypoints.push_back(glm::vec3(-14.6346f, 0.0f, 11.3402f));
    aiWaypoints.push_back(glm::vec3(-15.3948f, 0.0f, 12.9190f));
    aiWaypoints.push_back(glm::vec3(-16.2529f, 0.0f, 13.9903f));
    aiWaypoints.push_back(glm::vec3(-17.4402f, 0.0f, 15.1772f));
    aiWaypoints.push_back(glm::vec3(-18.7166f, 0.0f, 16.1678f));
    aiWaypoints.push_back(glm::vec3(-19.7307f, 0.0f, 16.8484f));
    aiWaypoints.push_back(glm::vec3(-20.5292f, 0.0f, 17.2504f));
    aiWaypoints.push_back(glm::vec3(-21.6503f, 0.0f, 17.7823f));
    aiWaypoints.push_back(glm::vec3(-22.6977f, 0.0f, 18.1117f));
    aiWaypoints.push_back(glm::vec3(-23.8565f, 0.0f, 18.4450f));
    aiWaypoints.push_back(glm::vec3(-25.1990f, 0.0f, 18.6440f));
    aiWaypoints.push_back(glm::vec3(-26.2335f, 0.0f, 18.5330f));
    aiWaypoints.push_back(glm::vec3(-27.1985f, 0.0f, 17.9697f));
    aiWaypoints.push_back(glm::vec3(-27.5218f, 0.0f, 17.6096f));
    aiWaypoints.push_back(glm::vec3(-27.5824f, 0.0f, 16.4448f));
    aiWaypoints.push_back(glm::vec3(-27.4934f, 0.0f, 16.0380f));
    aiWaypoints.push_back(glm::vec3(-27.0386f, 0.0f, 15.6751f));
    aiWaypoints.push_back(glm::vec3(-25.8365f, 0.0f, 15.3164f));
    aiWaypoints.push_back(glm::vec3(-25.0517f, 0.0f, 15.3764f));
    aiWaypoints.push_back(glm::vec3(-23.9274f, 0.0f, 15.4929f));
    aiWaypoints.push_back(glm::vec3(-22.9257f, 0.0f, 15.5876f));
    aiWaypoints.push_back(glm::vec3(-22.4709f, 0.0f, 15.5126f));
    aiWaypoints.push_back(glm::vec3(-21.9348f, 0.0f, 15.2519f));
    aiWaypoints.push_back(glm::vec3(-21.5428f, 0.0f, 14.9263f));
    aiWaypoints.push_back(glm::vec3(-20.9628f, 0.0f, 14.4443f));
    aiWaypoints.push_back(glm::vec3(-20.2775f, 0.0f, 13.4949f));
    aiWaypoints.push_back(glm::vec3(-20.0552f, 0.0f, 12.7826f));
    aiWaypoints.push_back(glm::vec3(-20.1685f, 0.0f, 11.8938f));
    aiWaypoints.push_back(glm::vec3(-20.3029f, 0.0f, 11.4411f));
    aiWaypoints.push_back(glm::vec3(-20.8131f, 0.0f, 10.7798f));
    aiWaypoints.push_back(glm::vec3(-22.3706f, 0.0f, 10.5592f));
    aiWaypoints.push_back(glm::vec3(-23.2476f, 0.0f, 10.6864f));
    aiWaypoints.push_back(glm::vec3(-24.0903f, 0.0f, 10.8968f));
    aiWaypoints.push_back(glm::vec3(-24.8417f, 0.0f, 11.0843f));
    aiWaypoints.push_back(glm::vec3(-25.6085f, 0.0f, 11.2758f));
    aiWaypoints.push_back(glm::vec3(-26.3332f, 0.0f, 11.4567f));
    aiWaypoints.push_back(glm::vec3(-27.0948f, 0.0f, 11.7497f));
    aiWaypoints.push_back(glm::vec3(-28.6680f, 0.0f, 12.6275f));
    aiWaypoints.push_back(glm::vec3(-29.8167f, 0.0f, 13.2684f));
    aiWaypoints.push_back(glm::vec3(-30.8902f, 0.0f, 14.1717f));
    aiWaypoints.push_back(glm::vec3(-31.3352f, 0.0f, 15.1534f));
    aiWaypoints.push_back(glm::vec3(-31.4855f, 0.0f, 16.2787f));
    aiWaypoints.push_back(glm::vec3(-31.1265f, 0.0f, 17.9219f));
    aiWaypoints.push_back(glm::vec3(-30.3571f, 0.0f, 19.5993f));
    aiWaypoints.push_back(glm::vec3(-29.8028f, 0.0f, 20.1476f));
    aiWaypoints.push_back(glm::vec3(-28.7473f, 0.0f, 20.7893f));
    aiWaypoints.push_back(glm::vec3(-26.9881f, 0.0f, 21.5204f));
    aiWaypoints.push_back(glm::vec3(-25.9370f, 0.0f, 21.8000f));
    aiWaypoints.push_back(glm::vec3(-24.7567f, 0.0f, 21.5176f));
    aiWaypoints.push_back(glm::vec3(-23.8638f, 0.0f, 21.2952f));
    aiWaypoints.push_back(glm::vec3(-22.9479f, 0.0f, 21.0669f));
    aiWaypoints.push_back(glm::vec3(-21.7535f, 0.0f, 20.7136f));
    aiWaypoints.push_back(glm::vec3(-20.9257f, 0.0f, 20.3663f));
    aiWaypoints.push_back(glm::vec3(-19.7475f, 0.0f, 19.8974f));
    aiWaypoints.push_back(glm::vec3(-18.9589f, 0.0f, 19.9399f));
    aiWaypoints.push_back(glm::vec3(-18.1971f, 0.0f, 19.9258f));
    aiWaypoints.push_back(glm::vec3(-16.9434f, 0.0f, 19.7359f));


    aiCar->MaxSpeed = car->MaxSpeed * 0.95f; // slightly slower than player by default

    camera = new Camera(glm::vec3(0.0f, 3.0f, 5.0f));
    track = new Track();
    city = new City(glm::vec3(0.0f, 0.0f, 0.0f));
    city->Scale = glm::vec3(0.3f);
    city->loadModel();

    kartingMap = new Model("assets/karting/gp.obj");
    TrackCollision::Init(2.0f);;

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

        // ... wewnątrz pętli while ...
        else if (currentState == RACING) {
            // ===== RACE TIMER UPDATE =====
            if (raceTimerActive && !raceFinished) {
                raceTimeLeft -= deltaTime;
                raceElapsedTime += deltaTime;  // <--- TRACKING ELAPSED TIME
                if (raceTimeLeft <= 0.0f) {
                    raceTimeLeft = 0.0f;
                    raceFinished = true;
                    raceWon = false;
                    raceTimerActive = false;

                    // SAVE MONEY ON TIME UP (Finish)
                    if (sessionMoney > 0) {
                        playerProfile.addMoney(sessionMoney);
                        playerProfile.save();
                    }
                }
            }

            // Only process input and physics if countdown and GO animation are not active
            if (!raceCountdownActive && !showGoAnimation) {
                processCarInput(deltaTime);
                //if (keys[GLFW_KEY_R]) car->Yaw += 200.0f * deltaTime;

                /*if (keys[GLFW_KEY_R]) {
                    car->Draw(carTrackShader, car->Position, car->Yaw + 90.0f);
                }
                else {
                    car->Draw(carTrackShader);
                }*/

                // 1. Zapamiętujemy pozycję przed aktualizacją физики
                glm::vec3 lastSafePos = car->Position;

                // 2. Wykonujemy ruch (fizyka обчислює новą pozycję na podstawie швидкості)
                car->Update(deltaTime);

                // Update AI car: simple follow logic
                if (aiCar && !aiWaypoints.empty()) {

                    glm::vec3 target = aiWaypoints[aiCurrentWaypoint];
                    glm::vec3 toTarget = target - aiCar->Position;
                    float distance = glm::length(toTarget);

                    // === ZMIANA WAYPOINTA ===
                    if (distance < aiWaypointRadius) {
                        aiCurrentWaypoint++;
                        if (aiCurrentWaypoint >= aiWaypoints.size())
                            aiCurrentWaypoint = 0;
                    }

                    // === KIERUNEK ===
                    float desiredYaw = glm::degrees(atan2(toTarget.x, toTarget.z));
                    float yawDiff = desiredYaw - aiCar->Yaw;

                    while (yawDiff > 180.0f) yawDiff -= 360.0f;
                    while (yawDiff < -180.0f) yawDiff += 360.0f;

                    aiCar->SteeringInput = glm::clamp(yawDiff / 25.0f, -1.0f, 1.0f);

                    // === GAZ ===
                    float absYaw = fabs(yawDiff);
                    if (absYaw > 60.0f)
                        aiCar->ThrottleInput = 0.4f;
                    else if (absYaw > 30.0f)
                        aiCar->ThrottleInput = 0.7f;
                    else
                        aiCar->ThrottleInput = 1.0f;

                    // === OBRÓT ===
                    float speed = glm::length(aiCar->Velocity);
                    if (speed > 0.1f) {
                        float turnAmount = aiCar->TurnRate * deltaTime * 50.0f;
                        aiCar->Yaw += turnAmount * aiCar->SteeringInput;

                        aiCar->FrontVector = glm::normalize(glm::vec3(
                            sin(glm::radians(aiCar->Yaw)),
                            0.0f,
                            cos(glm::radians(aiCar->Yaw))
                        ));
                    }

                    aiCar->Update(deltaTime);

                    // === LIMIT PRĘDKOŚCI ===
                    float aiSpeed = glm::length(aiCar->Velocity);
                    if (aiSpeed > aiCar->MaxSpeed)
                        aiCar->Velocity = glm::normalize(aiCar->Velocity) * aiCar->MaxSpeed;
                }

                // clamp velocity to MaxSpeed after Update
                float speed = glm::length(car->Velocity);
                if (speed > car->MaxSpeed) {
                    car->Velocity = glm::normalize(car->Velocity) * car->MaxSpeed;
                }

                // ===============================
                // == AI FINISH CHECK ============
                // ===============================
                float dist = glm::distance(aiCar->Position, lapStartPosition);
                if (!aiLeftStartZone && dist > lapFinishRadius * 2.0f) aiLeftStartZone = true;

                if (aiLeftStartZone && dist < lapFinishRadius && !aiRaceFinished) {
                    aiCurrentLap++;
                    if (aiCurrentLap > totalLaps) {
                        aiRaceFinished = true;

                        if (!raceFinished) {          // якщо гр玩家 ще не скончив
                            aiRaceWon = true;         // AI виграв
                            raceWon = false;          // гр玩家 не виграв
                            raceFinished = true;      // закінчи гонку
                            raceTimerActive = false;

                            // SAVE MONEY EVEN IF LOST 
                            if (sessionMoney > 0) {
                                playerProfile.addMoney(sessionMoney);
                                playerProfile.save();
                            }
                        }

                        aiCar->Velocity = glm::vec3(0.0f); // зупинка AI
                    }
                    aiLeftStartZone = false;
                }

                // ===============================
                // == PLAYER FINISH CHECK ========
                // ===============================
                dist = glm::distance(car->Position, lapStartPosition);
                if (!leftStartZone && dist > lapFinishRadius * 2.0f) leftStartZone = true;

                // CHECK IF DRIVING FORWARD (Avoid reverse cheating)
                // We check dot product of car FrontVector vs Track Direction
                bool isMovingForward = glm::dot(car->FrontVector, trackForward) > 0.0f;

                if (leftStartZone && dist < lapFinishRadius && raceTimerActive && isMovingForward) {
                    currentLap++;

                    // REWARD: 50 coins per completed lap
                    sessionMoney += 50;  // Accumulate locally

                    if (currentLap > totalLaps) {
                        raceFinished = true;

                        if (!aiRaceFinished) {     // гр玩家 доїхав першим
                            raceWon = true;
                            aiRaceWon = false;

                            // BONUS REWARDS FOR WINNING
                            int bonus = 0;
                            if (totalLaps == 1) bonus = 100;
                            else if (totalLaps == 3) bonus = 300;
                            else if (totalLaps == 10) bonus = 1000;
                            else bonus = 100 * totalLaps;

                            sessionMoney += bonus;
                        }
                        else {                   // AI вже скінчив раніше
                            raceWon = false;
                            aiRaceWon = true;
                        }

                        // SAVE ALL MONEY AT END OF RACE
                        if (sessionMoney > 0) {
                            playerProfile.addMoney(sessionMoney);
                            playerProfile.save();
                        }
                    }

                    leftStartZone = false;
                }

                // ===============================
                // == KOLIZJA ====================
                // ===============================
                if (selectedTrack == 2) {
                    TrackCollision track;

                    if (track.CheckCollision(car->Position, 0.35f)) {
                        car->Position = lastSafePos;
                        car->Velocity *= -0.25f;
                    }
                }

                // Logowanie (klawisz P)
                static float logTimer = 0.0f;
                logTimer += deltaTime;
                if (keys[GLFW_KEY_P] && logTimer > 0.2f) {
                    std::cout << "AKTUALNA POZYCJA: X: " << car->Position.x << " Z: " << car->Position.z << std::endl;
                    logTimer = 0.0f;
                }

            }
            else {

                // countdown active -> reduce timer
                if (raceCountdownActive) {
                    raceCountdown -= deltaTime;
                    if (raceCountdown <= 0.0f) {
                        raceCountdownActive = false;

                        showGoAnimation = true;
                        goTimer = goDuration;

                        if (car) {
                            car->Velocity = glm::vec3(0.0f);
                            car->Handbrake = true;
                        }
                    }
                }

                // GO! animation timing
                if (showGoAnimation) {
                    goTimer -= deltaTime;
                    if (goTimer <= 0.0f) {
                        showGoAnimation = false;
                        if (car) car->Handbrake = false;
                        raceTimerActive = true;
                    }
                }
            }

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

        // Draw AI car (already updated above)
        if (aiCar && currentState == RACING) {
            carTrackShader.use();
            carTrackShader.setVec3("objectColor", glm::vec3(0.2f, 0.8f, 0.2f));
            aiCar->Draw(carTrackShader);
            // restore objectColor for further draws
            carTrackShader.setVec3("objectColor", carCustomColor);
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
            // If countdown is active, show animated numbers
            if (raceCountdownActive) {
                int displayNum = (int)std::ceil(raceCountdown);
                // progress from 0..1 for current number (1 when fully visible, 0 when just switched)
                float frac = raceCountdown - std::floor(raceCountdown);
                float progress = 1.0f - frac; // increases as number approaches integer
                progress = glm::clamp(progress, 0.0f, 1.0f);

                // Make numbers much larger for better visibility
                float scale = glm::mix(2.0f, 6.0f, progress);
                float alpha = glm::mix(0.15f, 1.0f, progress);

                std::string txt = std::to_string(displayNum);

                ImGui::SetNextWindowPos(ImVec2(current_width * 0.5f, current_height * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                ImGui::SetNextWindowSize(ImVec2(800, 480));
                ImGui::Begin("Race Countdown", nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings);

                // shadow
                ImGui::SetWindowFontScale(scale);
                ImVec2 winSize = ImGui::GetWindowSize();
                ImVec2 textSize = ImGui::CalcTextSize(txt.c_str());
                float x = (winSize.x - textSize.x) * 0.5f;
                float y = (winSize.y - textSize.y) * 0.45f;

                ImGui::SetCursorPos(ImVec2(x + 6.0f, y + 6.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, alpha * 0.9f));
                ImGui::TextUnformatted(txt.c_str());
                ImGui::PopStyleColor();

                // main text
                ImGui::SetCursorPos(ImVec2(x, y));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.1f, alpha));
                ImGui::TextUnformatted(txt.c_str());
                ImGui::PopStyleColor();

                ImGui::SetWindowFontScale(1.0f);
                ImGui::End();

                // small HUD below
                ImGui::SetNextWindowPos(ImVec2(10, current_height - 60));

            }
            else if (showGoAnimation) {
                // GO! animated pop + fade
                float t = glm::clamp(goTimer / goDuration, 0.0f, 1.0f);
                // at start t=1, at end t=0 -> we want scale big at start -> map to [3.0..8.0]
                float scale = glm::mix(3.0f, 8.0f, t);
                float alpha = glm::mix(0.0f, 1.0f, t);
                // slight pulse
                float pulse = 1.0f + 0.06f * std::sin((1.0f - t) * 6.28318f * 2.0f);
                scale *= pulse;

                std::string txt = "GO!";

                ImGui::SetNextWindowPos(ImVec2(current_width * 0.5f, current_height * 0.45f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                ImGui::SetNextWindowSize(ImVec2(900, 480));
                ImGui::Begin("Race GO", nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings);

                ImGui::SetWindowFontScale(scale);
                ImVec2 winSize = ImGui::GetWindowSize();
                ImVec2 textSize = ImGui::CalcTextSize(txt.c_str());
                float x = (winSize.x - textSize.x) * 0.5f;
                float y = (winSize.y - textSize.y) * 0.5f;

                // shadow
                ImGui::SetCursorPos(ImVec2(x + 8.0f, y + 8.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, alpha * 0.95f));
                ImGui::TextUnformatted(txt.c_str());
                ImGui::PopStyleColor();

                // main colorful text (green)
                ImGui::SetCursorPos(ImVec2(x, y));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.95f, 0.25f, alpha));
                ImGui::TextUnformatted(txt.c_str());
                ImGui::PopStyleColor();

                ImGui::SetWindowFontScale(1.0f);
                ImGui::End();

                // small HUD below
                ImGui::SetNextWindowPos(ImVec2(10, current_height - 60));

            }

            // Normal in-race UI when not counting down/animating
            if (!raceCountdownActive && !showGoAnimation) {
                ImGui::SetNextWindowPos(ImVec2(10, current_height - 60));
                float panelWidth = 320.0f;
                float panelHeight = 170.0f;

                ImGui::SetNextWindowPos(
                    ImVec2(current_width - panelWidth - 20.0f, 20.0f),
                    ImGuiCond_Always
                );
                ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));

                ImGui::Begin("RaceHUD", nullptr,
                    ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoSavedSettings);

                ImDrawList* draw = ImGui::GetWindowDrawList();
                ImVec2 pos = ImGui::GetWindowPos();
                ImVec2 size = ImGui::GetWindowSize();

                // Background (glass style)
                draw->AddRectFilled(
                    pos,
                    ImVec2(pos.x + size.x, pos.y + size.y),
                    IM_COL32(10, 10, 10, 200),
                    18.0f
                );

                // Neon border
                draw->AddRect(
                    pos,
                    ImVec2(pos.x + size.x, pos.y + size.y),
                    IM_COL32(255, 140, 40, 220),
                    18.0f,
                    0,
                    2.5f
                );

                ImGui::Dummy(ImVec2(0, 8));

                // ===== SPEED =====
                float speedKmh = glm::length(car->Velocity) * 3.6f;
                ImGui::SetWindowFontScale(2.6f);
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.15f, 1.0f), "%.0f km/h", speedKmh);

                ImGui::SetWindowFontScale(1.0f);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "SPEED");

                ImGui::Separator();

                // ===== TIME (Remaining properly formatted) =====
                ImGui::SetWindowFontScale(1.8f);
                ImVec4 timeColor =
                    raceTimeLeft < 10.0f
                    ? ImVec4(1.0f, 0.2f, 0.2f, 1.0f)
                    : ImVec4(0.2f, 0.9f, 1.0f, 1.0f);

                // Format Minutes:Seconds
                int minLeft = (int)raceTimeLeft / 60;
                int secLeft = (int)raceTimeLeft % 60;
                // Fraction for smooth counting if needed, but standard clock is fine

                ImGui::TextColored(timeColor, "TIME: %02d:%02d", minLeft, secLeft);

                ImGui::SetWindowFontScale(1.0f);

                ImGui::Separator();

                // ===== LAP =====
                ImGui::SetWindowFontScale(1.4f);
                ImGui::TextColored(
                    ImVec4(0.9f, 0.9f, 0.9f, 1.0f),
                    "LAP %d / %d",
                    std::min(currentLap, totalLaps),
                    totalLaps
                );
                ImGui::SetWindowFontScale(1.0f);


                ImGui::Separator();

                // ===== VIEW MODE =====
                ImGui::TextColored(
                    ImVec4(0.8f, 0.8f, 0.8f, 1.0f),
                    "CAMERA: %s",
                    cockpitView ? "COCKPIT" : "CHASE"
                );

                ImGui::End();

                // Mini-map window (bottom-right)
                ImGui::SetNextWindowPos(ImVec2(current_width - 220, current_height - 220), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(200, 200));
                ImGui::Begin("MiniMap", nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);

                ImDrawList* mdraw = ImGui::GetWindowDrawList();
                ImVec2 mpos = ImGui::GetWindowPos();
                ImVec2 msize = ImGui::GetWindowSize();

                // background and border
                mdraw->AddRectFilled(mpos, ImVec2(mpos.x + msize.x, mpos.y + msize.y), IM_COL32(8, 8, 8, 200), 8.0f);
                mdraw->AddRect(mpos, ImVec2(mpos.x + msize.x, mpos.y + msize.y), IM_COL32(160, 160, 160, 120), 8.0f, 0, 2.0f);

                // draw the minimap content with small inner padding
                DrawMiniMap(mdraw, ImVec2(mpos.x + 8.0f, mpos.y + 8.0f), ImVec2(msize.x - 16.0f, msize.y - 16.0f), car->Position, aiCar ? aiCar->Position : glm::vec3(0.0f));

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
        }

        if (raceFinished && currentState == RACING) {


            static float winAnimTime = 0.0f;
            winAnimTime += deltaTime;

            float t = glm::clamp(winAnimTime / 1.2f, 0.0f, 1.0f);
            float scale = glm::mix(0.6f, 1.0f, t);
            float alpha = glm::smoothstep(0.0f, 1.0f, t);
            float glow = 0.6f + 0.4f * sin(winAnimTime * 4.0f);

            ImGui::SetNextWindowPos(
                ImVec2(current_width * 0.5f, current_height * 0.42f),
                ImGuiCond_Always,
                ImVec2(0.5f, 0.5f)
            );

            ImGui::SetNextWindowSize(ImVec2(560, 260));

            ImGui::Begin("WIN_HUD", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoBackground |
                ImGuiWindowFlags_NoSavedSettings);

            ImDrawList* draw = ImGui::GetWindowDrawList();
            ImVec2 pos = ImGui::GetWindowPos();
            ImVec2 size = ImGui::GetWindowSize();

            // ===== GLASS BACKGROUND =====
            draw->AddRectFilled(
                pos,
                ImVec2(pos.x + size.x, pos.y + size.y),
                IM_COL32(10, 15, 20, 220),
                20.0f
            );

            // ===== NEON BORDER =====
            draw->AddRect(
                pos,
                ImVec2(pos.x + size.x, pos.y + size.y),
                IM_COL32(0, 200, 255, (int)(200 * glow)),
                20.0f,
                0,
                3.0f
            );

            // ===== TITLE =====
            const char* title = raceWon ? "YOU  WIN" : (raceTimeLeft <= 0.0f ? "TIME  UP" : "GAME OVER");

            ImGui::SetWindowFontScale(3.2f * scale);
            ImVec2 titleSize = ImGui::CalcTextSize(title);

            ImGui::SetCursorPos(ImVec2(
                (size.x - titleSize.x) * 0.5f,
                30
            ));

            ImGui::PushStyleColor(
                ImGuiCol_Text,
                raceWon
                ? ImVec4(1.0f, 0.75f, 0.2f, alpha)   // win orange
                : ImVec4(1.0f, 0.2f, 0.2f, alpha)    // lose red
            );
            ImGui::TextUnformatted(title);
            ImGui::PopStyleColor();

            ImGui::SetWindowFontScale(1.0f);

            // ===== SUMMARY =====
            // 1. SHOW ELAPSED TIME INSTEAD OF NEGATIVE TIME
            ImGui::SetCursorPos(ImVec2(40, 110));
            ImGui::TextColored(ImVec4(0.6f, 0.9f, 1.0f, alpha), "TIME");

            ImGui::SameLine(140);

            // Calc minutes/seconds from raceElapsedTime
            int eMin = (int)raceElapsedTime / 60;
            int eSec = (int)raceElapsedTime % 60;

            ImGui::TextColored(
                ImVec4(1.0f, 1.0f, 1.0f, alpha),
                "%02d:%02d", // e.g. 01:23
                eMin, eSec
            );

            // 2. SHOW MONEY EARNED
            // Lap info
            ImGui::SetCursorPos(ImVec2(40, 145));
            ImGui::TextColored(ImVec4(0.6f, 0.9f, 1.0f, alpha), "LAPS");

            ImGui::SameLine(140);
            ImGui::TextColored(
                ImVec4(1.0f, 1.0f, 1.0f, alpha),
                "%d / %d",
                std::min(currentLap - 1, totalLaps), // Don't show "3/1" if done
                totalLaps
            );

            // Money Earned line
            ImGui::SetCursorPos(ImVec2(40, 180));
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.1f, alpha), "MONEY");
            ImGui::SameLine(140);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, alpha), "+ %d $", sessionMoney);


            // ===== BACK BUTTON (RIGHT SIDE) =====
            ImGui::SetCursorPos(ImVec2(size.x - 150, size.y - 60));

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.15f, 0.2f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 1.0f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.4f, 0.8f, 1.0f));

            if (ImGui::Button("MENU", ImVec2(120, 36))) {
                currentState = MAIN_MENU;

                // 🔥 RESET RACE STATE
                raceFinished = false;
                raceWon = false;
                raceTimerActive = false;
                sessionMoney = 0; // Clear visible session money

                winAnimTime = 0.0f;
            }


            ImGui::PopStyleColor(3);

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
    delete aiCar;
    delete camera;
    delete track;
    delete city;
    delete kartingMap;

    glfwTerminate();
    return 0;
}