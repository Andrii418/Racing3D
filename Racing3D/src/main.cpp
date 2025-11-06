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
#include "Car.h"
#include "Track.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

// -------------------------------------------------------------
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Stany gry
enum GameState {
    SPLASH_SCREEN,
    MAIN_MENU,
    RACING
};

// Globalne zmienne
Car* car = nullptr;
Camera* camera = nullptr;
Track* track = nullptr;
bool keys[1024] = { false };
float lastFrame = 0.0f;
GameState currentState = SPLASH_SCREEN;
float splashTimer = 0.0f;
glm::vec3 carCustomColor = glm::vec3(1.0f, 0.5f, 0.2f);

// -------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// -------------------------------------------------------------
// Input
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    static bool handling = false;
    if (handling) return; // zapobiega rekurencji
    handling = true;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (currentState == RACING)
            currentState = MAIN_MENU;
        else
            glfwSetWindowShouldClose(window, true);
    }

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }

    // UWAGA: NIE przekazujemy już eventów do ImGui ręcznie!
    // ImGui rejestruje własne callbacki (bo inicjalizujemy z "true").

    handling = false;
}

// -------------------------------------------------------------
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

// -------------------------------------------------------------
// ImGui – ekrany
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

void RenderMainMenu() {
    ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH * 0.5f, SCR_HEIGHT * 0.5f),
        ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::Begin("Main Menu", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.2f, 1.0f));
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("RACING 3D");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();

    ImGui::Separator();
    ImGui::Spacing();

    float col[3] = { carCustomColor.r, carCustomColor.g, carCustomColor.b };
    ImGui::Text("Car Color:");
    if (ImGui::ColorEdit3("##CarColorPicker", col)) {
        carCustomColor = glm::vec3(col[0], col[1], col[2]);
    }

    ImGui::Spacing();

    if (ImGui::Button("START RACE", ImVec2(150, 40))) {
        currentState = RACING;
        if (car) {
            car->Position = glm::vec3(0.0f, 0.5f, 0.0f);
            car->Velocity = glm::vec3(0.0f);
            car->Yaw = 0.0f;
            car->FrontVector = glm::vec3(0.0f, 0.0f, 1.0f);
        }
    }

    ImGui::Spacing();

    if (ImGui::Button("EXIT GAME", ImVec2(150, 40))) {
        GLFWwindow* window = glfwGetCurrentContext();
        glfwSetWindowShouldClose(window, true);
    }

    ImGui::End();
}

// -------------------------------------------------------------
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

    // --- ImGui ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // 🔥 KLUCZOWE: ustaw "true" → ImGui sam zarejestruje swoje callbacki.
    // NIE wywołujemy już ImGui_ImplGlfw_KeyCallback ręcznie w naszym callbacku!
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGui::StyleColorsDark();

    // --- Obiekty gry ---
    Shader carTrackShader("shaders/phong.vert", "shaders/phong.frag");
    car = new Car(glm::vec3(0.0f, 0.5f, 0.0f));
    camera = new Camera(glm::vec3(0.0f, 3.0f, 5.0f));
    track = new Track();

    // --- Pętla gry ---
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (currentState == SPLASH_SCREEN) {
            splashTimer += deltaTime;
            if (splashTimer > 2.5f) currentState = MAIN_MENU;
        }
        else if (currentState == RACING) {
            processCarInput(deltaTime);
            car->Update(deltaTime);
        }

        if (camera && car)
            camera->FollowCar(car->Position, car->FrontVector);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        carTrackShader.use();
        if (camera) {
            carTrackShader.setMat4("view", camera->GetViewMatrix());
            carTrackShader.setMat4("projection",
                camera->GetProjectionMatrix((float)SCR_WIDTH / (float)SCR_HEIGHT));
        }

        glm::vec3 lightPos(5.0f, 10.0f, 5.0f);
        carTrackShader.setVec3("lightPos", lightPos);
        carTrackShader.setVec3("lightColor", glm::vec3(1.0f));
        if (camera)
            carTrackShader.setVec3("viewPos", camera->Position);

        if (track) {
            carTrackShader.setVec3("objectColor", glm::vec3(0.2f, 0.4f, 0.2f));
            track->Draw(carTrackShader);
        }

        if (car) {
            carTrackShader.setVec3("objectColor", carCustomColor);
            car->Draw(carTrackShader);
        }

        // --- ImGui ---
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
            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- Sprzątanie ---
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    delete car;
    delete camera;
    delete track;

    glfwTerminate();
    return 0;
}
