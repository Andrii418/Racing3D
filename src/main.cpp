#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Camaro.h"
#include "Track.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui_internal.h"

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

unsigned int FBO_Scene;
unsigned int textureColorBuffer;
unsigned int RBO_DepthStencil;
unsigned int quadVAO, quadVBO;

enum GameState {
    SPLASH_SCREEN,
    MAIN_MENU,
    RACING
};

Camaro* car = nullptr;
Camera* camera = nullptr;
Track* track = nullptr;
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

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
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

    if (keys[GLFW_KEY_W]) car->Velocity += car->FrontVector * car->Acceleration * deltaTime;
    if (keys[GLFW_KEY_S]) car->Velocity -= car->FrontVector * car->Braking * deltaTime;

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

void setupFramebuffer() {
    glGenFramebuffers(1, &FBO_Scene);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO_Scene);

    glGenTextures(1, &textureColorBuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBuffer, 0);

    glGenRenderbuffers(1, &RBO_DepthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO_DepthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO_DepthStencil);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderSplashScreen() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(SCR_WIDTH, SCR_HEIGHT));
    ImGui::Begin("Splash Screen", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground);

    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0, 0), ImVec2(SCR_WIDTH, SCR_HEIGHT),
        IM_COL32(20, 20, 20, 255));

    ImGui::SetCursorPosX((SCR_WIDTH - ImGui::CalcTextSize("RACING 3D").x * 2.0f) * 0.5f);
    ImGui::SetCursorPosY(SCR_HEIGHT * 0.4f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.2f, 1.0f));
    ImGui::SetWindowFontScale(2.0f);
    ImGui::Text("RACING 3D");
    ImGui::PopStyleColor();
    ImGui::SetWindowFontScale(1.0f);

    ImGui::SetCursorPosX((SCR_WIDTH - ImGui::CalcTextSize("Loading...").x) * 0.5f);
    ImGui::SetCursorPosY(SCR_HEIGHT * 0.6f);
    ImGui::Text("Loading...");

    ImGui::End();
}

void RenderCarSelectMenu() {
    ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH * 0.5f, SCR_HEIGHT * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
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

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.15f, 0.18f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.5f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.05f, 0.45f, 0.9f, 1.0f));

    if (ImGui::Button("BACK", ImVec2(200, 35))) {
        showCarSelect = false;
    }

    ImGui::PopStyleColor(3);

    ImGui::End();
}

void RenderTrackSelectMenu() {
    ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH * 0.5f, SCR_HEIGHT * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 350));
    ImGui::Begin("Track Select Menu", &showTrackSelect,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

    ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos(),
        ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y),
        IM_COL32(0, 0, 0, 220));

    ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetWindowPos(),
        ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + 10),
        IM_COL32(25, 127, 255, 255));

    ImGui::Dummy(ImVec2(0, 10));

    ImGui::TextColored(ImVec4(0.1f, 0.5f, 1.0f, 1.0f), "  [ TRACK SELECTION ]");
    ImGui::Separator();
    ImGui::Spacing();

    const char* trackNames[] = { "Flat Grass Arena (Level 1)", "Desert Canyon (Level 2)" };
    ImGui::Text("Selected Track: %s", trackNames[selectedTrack]);

    if (ImGui::BeginListBox("##TrackList", ImVec2(-FLT_MIN, 100)))
    {
        if (ImGui::Selectable(trackNames[0], selectedTrack == 0)) selectedTrack = 0;
        if (ImGui::Selectable(trackNames[1], selectedTrack == 1)) selectedTrack = 1;
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

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.15f, 0.18f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.5f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.05f, 0.45f, 0.9f, 1.0f));

    if (ImGui::Button("BACK", ImVec2(200, 35))) {
        showTrackSelect = false;
    }

    ImGui::PopStyleColor(3);

    ImGui::End();
}

void RenderSettingsMenu() {
    ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH * 0.5f, SCR_HEIGHT * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 300));
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

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.15f, 0.18f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.5f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.05f, 0.45f, 0.9f, 1.0f));

    if (ImGui::Button("BACK", ImVec2(180, 35))) {
        showSettings = false;
    }

    ImGui::PopStyleColor(3);

    ImGui::End();
}

void RenderMainMenu() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(SCR_WIDTH, SCR_HEIGHT));
    ImGui::Begin("Fullscreen Background", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);

    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0, 0), ImVec2(SCR_WIDTH, SCR_HEIGHT), IM_COL32(0, 0, 0, 180));

    if (showSettings || showCarSelect || showTrackSelect) {
        ImGui::End();
        if (showSettings) RenderSettingsMenu();
        if (showCarSelect) RenderCarSelectMenu();
        if (showTrackSelect) RenderTrackSelectMenu();
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(SCR_WIDTH, SCR_HEIGHT * 0.2f));
    ImGui::Begin("Title Panel", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);

    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0, 0), ImVec2(SCR_WIDTH, SCR_HEIGHT * 0.2f), IM_COL32(0, 0, 0, 150));

    float titleSize = 4.0f;
    ImGui::SetWindowFontScale(titleSize);

    float titleWidth = ImGui::CalcTextSize("RACING 3D").x;
    ImGui::SetCursorPosX((SCR_WIDTH - titleWidth) * 0.5f);

    ImGui::SetCursorPosY((SCR_HEIGHT * 0.2f - ImGui::GetTextLineHeightWithSpacing() * titleSize) * 0.5f);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.5f, 1.0f, 1.0f));
    ImGui::Text("RACING 3D");
    ImGui::PopStyleColor();

    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(SCR_WIDTH * 0.5f - 150.0f, ImGui::GetCursorScreenPos().y - 10.0f),
        ImVec2(SCR_WIDTH * 0.5f + 150.0f, ImGui::GetCursorScreenPos().y - 10.0f),
        IM_COL32(25, 127, 255, 255), 2.0f
    );

    ImGui::SetWindowFontScale(1.0f);

    ImGui::End();

    float buttonsPanelHeight = 400.0f;
    float startY = SCR_HEIGHT * 0.2f + (SCR_HEIGHT * 0.8f - buttonsPanelHeight) * 0.5f;

    ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH * 0.5f, startY), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400, buttonsPanelHeight));
    ImGui::Begin("Main Menu Buttons", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground);

    ImVec2 buttonSize(350, 60);
    float buttonX = (ImGui::GetWindowSize().x - buttonSize.x) * 0.5f;

    ImGui::SetCursorPosX(buttonX);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.05f, 0.45f, 0.9f, 1.0f));

    if (ImGui::Button("START RACE", buttonSize)) {
        currentState = RACING;
        if (car) {
            car->Position = glm::vec3(0.0f, 0.5f, 0.0f);
            car->Velocity = glm::vec3(0.0f);
            car->Yaw = 0.0f;
            car->FrontVector = glm::vec3(0.0f, 0.0f, 1.0f);
        }
    }

    ImGui::PopStyleColor(4);

    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::SetCursorPosX(buttonX);
    if (ImGui::Button("SELECT CAR", buttonSize)) showCarSelect = true;

    ImGui::SetCursorPosX(buttonX);
    if (ImGui::Button("SELECT TRACK", buttonSize)) showTrackSelect = true;

    ImGui::SetCursorPosX(buttonX);
    if (ImGui::Button("SETTINGS", buttonSize)) showSettings = true;

    ImGui::SetCursorPosX(buttonX);
    if (ImGui::Button("EXIT GAME", buttonSize)) {
        GLFWwindow* window = glfwGetCurrentContext();
        glfwSetWindowShouldClose(window, true);
    }

    ImGui::End();
    ImGui::End();
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Racing3D", nullptr, nullptr);
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

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    style.FrameRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.WindowPadding = ImVec2(0, 0);
    style.ItemSpacing = ImVec2(8, 10);

    ImVec4 primaryBgColor = ImVec4(0.1f, 0.15f, 0.18f, 0.9f);
    ImVec4 hoverColor = ImVec4(0.1f, 0.5f, 1.0f, 1.0f);
    ImVec4 activeColor = ImVec4(0.05f, 0.45f, 0.9f, 1.0f);

    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.94f);

    style.Colors[ImGuiCol_Button] = primaryBgColor;
    style.Colors[ImGuiCol_ButtonHovered] = hoverColor;
    style.Colors[ImGuiCol_ButtonActive] = activeColor;

    style.Colors[ImGuiCol_Text] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
    style.Colors[ImGuiCol_Header] = primaryBgColor;
    style.Colors[ImGuiCol_HeaderHovered] = hoverColor;
    style.Colors[ImGuiCol_HeaderActive] = activeColor;

    Shader carTrackShader("shaders/phong.vert", "shaders/phong.frag");

    Shader postProcessShader("shaders/postprocess.vert", "shaders/postprocess.frag");
    postProcessShader.use();
    postProcessShader.setInt("screenTexture", 0);

    setupPostProcessingQuad();
    setupFramebuffer();

    car = new Camaro(glm::vec3(0.0f, 0.5f, 0.0f));
    car->loadModel();
    camera = new Camera(glm::vec3(0.0f, 3.0f, 5.0f));
    track = new Track();

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

        glm::vec3 skyColor = glm::mix(glm::vec3(0.05f, 0.05f, 0.15f), glm::vec3(0.2f, 0.3f, 0.3f), timeOfDay);
        glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        carTrackShader.use();
        if (camera) {
            carTrackShader.setMat4("view", camera->GetViewMatrix());
            carTrackShader.setMat4("projection", camera->GetProjectionMatrix((float)SCR_WIDTH / (float)SCR_HEIGHT));
        }
        glm::vec3 lightPos(5.0f, 10.0f, 5.0f);
        carTrackShader.setVec3("lightPos", lightPos);

        glm::vec3 lightColor = glm::mix(glm::vec3(0.1f), glm::vec3(1.0f), timeOfDay);
        carTrackShader.setVec3("lightColor", lightColor);

        if (camera)
            carTrackShader.setVec3("viewPos", camera->Position);

        if (track) {
            carTrackShader.setVec3("objectColor", glm::vec3(0.2f, 0.4f, 0.2f));
            track->Draw(carTrackShader);
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
            ImGui::SetNextWindowPos(ImVec2(10, SCR_HEIGHT - 60));
            ImGui::Begin("HUD", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
            ImGui::Text("Speed: %.1f km/h", glm::length(car->Velocity) * 3.6f);
            ImGui::Text("View: %s", cockpitView ? "Cockpit" : "Chase");

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

    glDeleteFramebuffers(1, &FBO_Scene);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);

    delete car;
    delete camera;
    delete track;

    glfwTerminate();
    return 0;
}