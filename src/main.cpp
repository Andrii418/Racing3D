#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // Zakładam, że masz ten plik w projekcie

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

// Zmienne globalne do przechowywania AKTUALNEJ rozdzielczości
int current_width = 1280;
int current_height = 720;

unsigned int FBO_Scene;
unsigned int textureColorBuffer;
unsigned int RBO_DepthStencil;
unsigned int quadVAO, quadVBO;

// NOWA ZMIENNA GLOBALNA dla tekstury splash screena
unsigned int splashTextureID = 0;

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

// Deklaracja funkcji, która teraz przyjmuje dynamiczne wymiary
void setupFramebuffer(int width, int height);

// NOWA FUNKCJA DO ŁADOWANIA TEKSTUR
unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        else {
            std::cout << "ERROR::TEXTURE::Unsupported number of color components: " << nrComponents << std::endl;
            stbi_image_free(data);
            return 0;
        }

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

    // AKTUALIZACJA GLOBALNYCH ZMIENNYCH
    current_width = width;
    current_height = height;

    // Ponowne skonfigurowanie FBO, aby pasowało do nowej rozdzielczości
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

// Funkcja teraz przyjmuje wymiary okna
void setupFramebuffer(int width, int height) {
    // Jeśli FBO już istnieje, usuwamy jego składowe, aby móc utworzyć nowe o innych wymiarach
    if (FBO_Scene != 0) {
        glDeleteFramebuffers(1, &FBO_Scene);
        glDeleteTextures(1, &textureColorBuffer);
        glDeleteRenderbuffers(1, &RBO_DepthStencil);
    }

    glGenFramebuffers(1, &FBO_Scene);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO_Scene);

    glGenTextures(1, &textureColorBuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
    // Używamy nowych wymiarów
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBuffer, 0);

    glGenRenderbuffers(1, &RBO_DepthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO_DepthStencil);
    // Używamy nowych wymiarów
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO_DepthStencil);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ZMODYFIKOWANA FUNKCJA SPLASH SCREEN
void RenderSplashScreen() {
    // Używamy dynamicznych wymiarów
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(current_width, current_height));
    ImGui::Begin("Splash Screen", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground);

    // Rysowanie tekstury tła, jeśli została załadowana
    if (splashTextureID != 0) {
        ImGui::GetWindowDrawList()->AddImage(
            (void*)(intptr_t)splashTextureID,
            ImVec2(0, 0),
            ImVec2(current_width, current_height)
        );
    }
    else {
        // Fallback: Rysowanie jednolitego tła
        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0, 0), ImVec2(current_width, current_height),
            IM_COL32(20, 20, 20, 255));

        // Pozycjonowanie tytułu
        ImGui::SetCursorPosX((current_width - ImGui::CalcTextSize("RACING 3D").x * 2.0f) * 0.5f);
        ImGui::SetCursorPosY(current_height * 0.4f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.2f, 1.0f));
        ImGui::SetWindowFontScale(2.0f);
        ImGui::Text("RACING 3D");
        ImGui::PopStyleColor();
        ImGui::SetWindowFontScale(1.0f);
    }

    // Dodanie tekstu "Loading..." na dole (opcjonalnie)
    ImGui::SetCursorPosX((current_width - ImGui::CalcTextSize("Loading...").x) * 0.5f);
    ImGui::SetCursorPosY(current_height * 0.9f); // Przesunięte niżej
    ImGui::Text("Loading...");

    ImGui::End();
}

void RenderCarSelectMenu() {
    // Wyśrodkowanie na podstawie dynamicznych wymiarów
    ImGui::SetNextWindowPos(ImVec2(current_width * 0.5f, current_height * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 450));
    ImGui::Begin("Car Select Menu", &showCarSelect,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

    // Aby zachować styl sub-menu, użyjemy jawnego koloru tła i górnej belki
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

    // Używamy stylu przycisku z głównego menu (został on globalnie ustawiony w main())
    if (ImGui::Button("BACK", ImVec2(200, 35))) {
        showCarSelect = false;
    }

    ImGui::End();
}

void RenderTrackSelectMenu() {
    // Wyśrodkowanie na podstawie dynamicznych wymiarów
    ImGui::SetNextWindowPos(ImVec2(current_width * 0.5f, current_height * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
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

    // Używamy stylu przycisku z głównego menu (został on globalnie ustawiony w main())
    if (ImGui::Button("BACK", ImVec2(200, 35))) {
        showTrackSelect = false;
    }

    ImGui::End();
}

void RenderSettingsMenu() {
    // Wyśrodkowanie na podstawie dynamicznych wymiarów
    ImGui::SetNextWindowPos(ImVec2(current_width * 0.5f, current_height * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
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

    // Używamy stylu przycisku z głównego menu (został on globalnie ustawiony w main())
    if (ImGui::Button("BACK", ImVec2(180, 35))) {
        showSettings = false;
    }

    ImGui::End();
}

void RenderMainMenu() {
    // 1. Fullscreen Background
    // Używamy dynamicznych wymiarów
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(current_width, current_height));
    ImGui::Begin("Fullscreen Background", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);

    // Rysowanie tła
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0, 0), ImVec2(current_width, current_height), IM_COL32(0, 0, 0, 180));

    // Rysowanie Tekstury Tła dla Menu Głównego (zamiast czarnego prostokąta)
    if (splashTextureID != 0) {
        ImGui::GetWindowDrawList()->AddImage(
            (void*)(intptr_t)splashTextureID,
            ImVec2(0, 0),
            ImVec2(current_width, current_height),
            ImVec2(0, 0),
            ImVec2(1, 1),
            IM_COL32(255, 255, 255, 100) // Trochę przezroczystości, żeby było ciemniejsze i widać było elementy 3D
        );
    }

    if (showSettings || showCarSelect || showTrackSelect) {
        ImGui::End();
        if (showSettings) RenderSettingsMenu();
        if (showCarSelect) RenderCarSelectMenu();
        if (showTrackSelect) RenderTrackSelectMenu();
        return;
    }

    // 2. Title Panel
    // Używamy dynamicznych wymiarów
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(current_width, current_height * 0.2f));
    ImGui::Begin("Title Panel", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);

    // Rysowanie tła panelu tytułowego
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(0, 0), ImVec2(current_width, current_height * 0.2f), IM_COL32(0, 0, 0, 150));

    float titleSize = 4.0f;
    ImGui::SetWindowFontScale(titleSize);

    // Pozycjonowanie tytułu - wyśrodkowanie horyzontalne
    float titleWidth = ImGui::CalcTextSize("RACING 3D").x;
    ImGui::SetCursorPosX((current_width - titleWidth) * 0.5f);

    // Pozycjonowanie tytułu - wyśrodkowanie wertykalne
    ImGui::SetCursorPosY((current_height * 0.2f - ImGui::GetTextLineHeightWithSpacing() * titleSize) * 0.5f);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.5f, 1.0f, 1.0f));
    ImGui::Text("RACING 3D");
    ImGui::PopStyleColor();

    // Linia pod tytułem
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(current_width * 0.5f - 150.0f, ImGui::GetCursorScreenPos().y - 10.0f),
        ImVec2(current_width * 0.5f + 150.0f, ImGui::GetCursorScreenPos().y - 10.0f),
        IM_COL32(25, 127, 255, 255), 2.0f
    );

    ImGui::SetWindowFontScale(1.0f);

    ImGui::End();

    // 3. Main Menu Buttons
    float buttonsPanelHeight = 400.0f;
    // Wyśrodkowanie przycisków pod tytułem, w pozostałej części ekranu
    float startY = current_height * 0.2f + (current_height * 0.8f - buttonsPanelHeight) * 0.5f;

    // Pozycjonowanie na podstawie dynamicznej szerokości
    ImGui::SetNextWindowPos(ImVec2(current_width * 0.5f, startY), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(400, buttonsPanelHeight));
    ImGui::Begin("Main Menu Buttons", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground);

    ImVec2 buttonSize(350, 60);
    float buttonX = (ImGui::GetWindowSize().x - buttonSize.x) * 0.5f;

    // Ustawienie czcionki dla przycisków, aby były większe
    ImGui::SetWindowFontScale(1.5f);

    // PRZYCISK START RACE (Akcent kolorystyczny: Pomarańczowy)
    ImGui::SetCursorPosX(buttonX);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.25f, 0.1f, 1.0f)); // Ciemniejszy pomarańcz
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.5f, 0.2f, 1.0f)); // Akcent pomarańczowy
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.4f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.5f, 0.2f, 1.0f)); // Ramka pomarańczowa

    // Musimy użyć flagi ImGuiButtonFlags_PressedOnDragDrop (lub innej) dla cienia, 
    // aby wymusić ponowne przerysowanie Border, ale tu po prostu ręcznie ustawiamy kolory dla tekstu
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    if (ImGui::Button("START RACE", buttonSize)) {
        currentState = RACING;
        if (car) {
            car->Position = glm::vec3(0.0f, 0.5f, 0.0f);
            car->Velocity = glm::vec3(0.0f);
            car->Yaw = 0.0f;
            car->FrontVector = glm::vec3(0.0f, 0.0f, 1.0f);
        }
    }

    ImGui::PopStyleColor(5); // Zdejmujemy 5 stylów

    ImGui::Spacing();
    ImGui::Spacing();

    // PRZYCISKI STANDARDOWE (Akcent kolorystyczny: Niebieski)
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.1f, 0.5f, 1.0f, 1.0f)); // Ramka niebieska

    ImGui::SetCursorPosX(buttonX);
    if (ImGui::Button("SELECT CAR", buttonSize)) showCarSelect = true;

    ImGui::SetCursorPosX(buttonX);
    if (ImGui::Button("SELECT TRACK", buttonSize)) showTrackSelect = true;

    ImGui::SetCursorPosX(buttonX);
    if (ImGui::Button("SETTINGS", buttonSize)) showSettings = true;

    ImGui::PopStyleColor(1); // Zdejmujemy ramkę niebieską

    // PRZYCISK EXIT GAME (Akcent kolorystyczny: Czerwony)
    ImGui::SetCursorPosX(buttonX);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.2f, 0.2f, 1.0f)); // Ramka czerwona

    if (ImGui::Button("EXIT GAME", buttonSize)) {
        GLFWwindow* window = glfwGetCurrentContext();
        glfwSetWindowShouldClose(window, true);
    }

    ImGui::PopStyleColor(4);

    ImGui::SetWindowFontScale(1.0f); // Wracamy do normalnej czcionki

    ImGui::End();
    ImGui::End();
}

int main() {
    // **********************************************
    // UWAGA: stbi_set_flip_vertically_on_load(true);
    //
    // Jeśli ładowane obrazy (w tym Tlo.png) wydają się odwrócone w pionie,
    // odkomentuj poniższą linię (najlepiej w sekcji konfiguracji przed wywołaniem loadTexture).
    // W Twoim projekcie może być to konieczne.
    // stbi_set_flip_vertically_on_load(true);
    // **********************************************


    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Używamy zmiennych dynamicznych jako początkowych wymiarów
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

    // ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    // **********************************************
    // MODYFIKACJA STYLU DLA NOWYCH PRZYCISKÓW
    // **********************************************
    style.FrameRounding = 12.0f; // ZAOKRĄGLENIE!
    style.FrameBorderSize = 2.0f; // Ramka wokół przycisku
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.WindowPadding = ImVec2(0, 0);
    style.ItemSpacing = ImVec2(8, 10);
    style.ItemInnerSpacing = ImVec2(6, 6);
    style.FramePadding = ImVec2(10, 10);
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

    // Główna paleta kolorów
    ImVec4 primaryBgColor = ImVec4(0.05f, 0.05f, 0.05f, 0.9f); // Bardzo ciemne tło (przyciski)
    ImVec4 hoverColor = ImVec4(0.1f, 0.5f, 1.0f, 1.0f); // Standardowy niebieski (hover)
    ImVec4 activeColor = ImVec4(0.05f, 0.45f, 0.9f, 1.0f); // Standardowy niebieski (active)
    ImVec4 borderColor = ImVec4(0.1f, 0.5f, 1.0f, 1.0f); // Ramka - domyślnie niebieska

    // Ustawienia dla okien (pop-upów, sub-menu)
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.94f); // Ciemne dla submenu

    // Ustawienia dla kontrolek i przycisków
    style.Colors[ImGuiCol_FrameBg] = primaryBgColor;
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.07f, 0.07f, 0.07f, 0.9f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.05f, 0.05f, 0.05f, 0.9f);

    // Główny styl przycisków (dla SELECT CAR/TRACK/SETTINGS)
    style.Colors[ImGuiCol_Button] = primaryBgColor;
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.1f, 0.5f, 1.0f, 1.0f); // Niebieski hover
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.05f, 0.45f, 0.9f, 1.0f); // Ciemniejszy niebieski active

    style.Colors[ImGuiCol_Border] = borderColor; // Domyslna ramka (niebieski)

    // Nagłówki (listy, selekty)
    style.Colors[ImGuiCol_Header] = primaryBgColor;
    style.Colors[ImGuiCol_HeaderHovered] = hoverColor;
    style.Colors[ImGuiCol_HeaderActive] = activeColor;

    // Tekst i Selektory (dla submenu)
    style.Colors[ImGuiCol_Text] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.1f, 0.5f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.1f, 0.5f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.1f, 0.5f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.05f, 0.45f, 0.9f, 1.0f);
    // **********************************************
    // KONIEC MODYFIKACJI STYLU
    // **********************************************

    Shader carTrackShader("shaders/phong.vert", "shaders/phong.frag");

    Shader postProcessShader("shaders/postprocess.vert", "shaders/postprocess.frag");
    postProcessShader.use();
    postProcessShader.setInt("screenTexture", 0);

    setupPostProcessingQuad();
    // Używamy zmiennych dynamicznych do pierwszej konfiguracji FBO
    setupFramebuffer(current_width, current_height);

    // *************************************************************
    // ŁADOWANIE NOWEJ TEKSTURY TŁA
    // *************************************************************
    stbi_set_flip_vertically_on_load(false); // Może być wymagane lub nie, w zależności od twojego pliku Tlo.png
    splashTextureID = loadTexture("assets/Tlo.png"); // TUTAJ ZMIENIONO: Tło.png -> Tlo.png

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

        // Zawsze używamy pełnego, aktualnego viewportu
        glViewport(0, 0, current_width, current_height);

        glm::vec3 skyColor = glm::mix(glm::vec3(0.05f, 0.05f, 0.15f), glm::vec3(0.2f, 0.3f, 0.3f), timeOfDay);
        glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        carTrackShader.use();
        if (camera) {
            // Używamy dynamicznych wymiarów do obliczenia proporcji
            carTrackShader.setMat4("view", camera->GetViewMatrix());
            carTrackShader.setMat4("projection", camera->GetProjectionMatrix((float)current_width / (float)current_height));
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

        // Używamy pełnego, aktualnego viewportu
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
            // Pozycjonowanie HUD na podstawie dynamicznej wysokości
            ImGui::SetNextWindowPos(ImVec2(10, current_height - 60));
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

    // Usunięcie tekstury tła
    if (splashTextureID != 0) glDeleteTextures(1, &splashTextureID);

    glDeleteFramebuffers(1, &FBO_Scene);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);

    delete car;
    delete camera;
    delete track;

    glfwTerminate();
    return 0;
}