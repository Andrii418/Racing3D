#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
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
#pragma comment(lib, "winmm.lib")

#include "ProfileManager.h"

/**
 * @brief Struktura przechowująca pełną konfigurację pojedynczego samochodu.
 *
 * Zawiera identyfikator, nazwę wyświetlaną, ścieżki do modeli 3D oraz parametry fizyczne/wizualne kół.
 */
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

/**
 * @brief Baza danych dostępnych samochodów w grze (garaż).
 *
 * Każdy wpis definiuje unikalny pojazd z jego specyficznymi ustawieniami osi i ceną.
 */
std::vector<CarData> garage = {
    {"race", "Racer GT", "assets/cars/OBJ format/race.obj", "assets/cars/OBJ format/wheel-racing.obj", "", 0, 0.45f, 0.45f, 0.48f},
    {"police", "Police Car", "assets/cars/OBJ format/police.obj", "assets/cars/OBJ format/wheel-default.obj", "", 1000, 0.48f, 0.48f, 0.83f},
    {"ambulance", "Ambulance", "assets/cars/OBJ format/ambulance.obj", "assets/cars/OBJ format/wheel-default.obj", "", 1200, 0.52f, 0.52f, 0.95f},
    {"taxi", "Taxi", "assets/cars/OBJ format/taxi.obj", "assets/cars/OBJ format/wheel-default.obj", "", 800, 0.48f, 0.48f, 0.75f},
    {"van", "Cargo Van", "assets/cars/OBJ format/van.obj", "assets/cars/OBJ format/wheel-truck.obj", "", 900, 0.52f, 0.52f, 0.78f},
    {"truck", "Heavy Truck", "assets/cars/OBJ format/truck.obj", "assets/cars/OBJ format/wheel-truck.obj", "", 1500, 0.54f, 0.54f, 0.82f},
    {"hatchback", "Sport Hatch", "assets/cars/OBJ format/hatchback-sports.obj", "assets/cars/OBJ format/wheel-racing.obj", "", 500, 0.45f, 0.45f, 0.82f},
    {"tractor", "Tractor 500", "assets/cars/OBJ format/tractor.obj", "assets/cars/OBJ format/wheel-tractor-front.obj", "assets/cars/OBJ format/wheel-tractor-back.obj", 2000, 0.55f, 0.75f, 0.60f}
};

/** @brief Profil gracza (stan konta, odblokowania, aktualny wybór). */
ProfileManager playerProfile;

/**
 * @brief Zabezpieczenie przed konfliktami makr `min/max` z `Windows.h` i standardową biblioteką C++.
 */
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

 /** @brief Domyślne wymiary okna aplikacji. */
int current_width = 1280;
int current_height = 720;

/**
 * @brief Zmienne obsługujące Framebuffer Object (FBO) do post-processingu i renderowania sceny do tekstury.
 */
unsigned int FBO_Scene;
unsigned int textureColorBuffer;
unsigned int RBO_DepthStencil;
unsigned int quadVAO, quadVBO;

/** @brief Identyfikatory tekstur dla elementów UI. */
unsigned int grassTextureID = 0;
unsigned int splashTextureID = 0;

/** @brief Zmienne globalne odpowiedzialne za rendering skyboxa (tła nieba). */
unsigned int skyboxVAO, skyboxVBO;
unsigned int skyboxTexture;
unsigned int skyboxShaderID;

/**
 * @brief Maszyna stanów gry zarządzająca przepływem sterowania.
 */
enum GameState {
    SPLASH_SCREEN,
    MAIN_MENU,
    RACING
};

/** @brief Obiekty gracza i przeciwnika AI. */
RaceCar* car = nullptr;
RaceCar* aiCar = nullptr;

/**
 * @brief System nawigacji AI oparty na liście punktów kontrolnych (waypoints).
 */
std::vector<glm::vec3> aiWaypoints;
int aiCurrentWaypoint = 0;
float aiWaypointRadius = 2.7f;

/** @brief Główne obiekty sceny: kamera, tory, miasto. */
Camera* camera = nullptr;
Track* track = nullptr;
City* city = nullptr;
Model* kartingMap = nullptr;

/** @brief Tablica stanu klawiszy do obsługi wejścia. */
bool keys[1024] = { false };
float lastFrame = 0.0f;
GameState currentState = SPLASH_SCREEN;
float splashTimer = 0.0f;
glm::vec3 carCustomColor = glm::vec3(1.0f, 0.5f, 0.2f);

/** @brief Ustawienia widoku i dźwięku. */
bool cockpitView = false;
ma_engine audio_engine;
ma_sound car_sound;
bool isAudioInit = false;
float masterVolume = 0.5f;

/** @brief Flagi do sterowania interfejsem UI (ImGui). */
bool showSettings = false;
bool showCarSelect = false;
bool showTrackSelect = false;
int selectedCar = 0;
int selectedTrack = 2;
float timeOfDay = 0.75f;

/** @brief Zmienne do wizualizacji w menu głównym (obracający się samochód). */
float carMenuRotation = 0.0f;
glm::vec3 menuCarPosition = glm::vec3(0.0f, 0.5f, 0.0f);
float backgroundYaw = 0.0f;

/** @brief Logika licznika czasu wyścigu. */
bool raceTimerActive = false;
float raceTimeLeft = 300.0f;
float raceElapsedTime = 0.0f;
bool raceFinished = false;
bool raceWon = false;

/** @brief System okrążeń. */
int currentLap = 1;
int selectedLapOption = 0;
int totalLaps = 1;

/** @brief System ekonomii w trakcie sesji. */
int sessionMoney = 0;

/** @brief Logika startu i mety. */
glm::vec3 lapStartPosition;
float lapFinishRadius = 2.0f;
bool leftStartZone = false;

/** @brief Stan wyścigu dla AI. */
int aiCurrentLap = 1;
bool aiLeftStartZone = false;
bool aiRaceFinished = false;
bool aiRaceWon = false;

/** @brief Wektor kierunku toru do sprawdzania jazdy pod prąd. */
glm::vec3 trackForward = glm::vec3(0.0f, 0.0f, 1.0f);

/** @brief Logika odliczania przed startem (3-2-1-GO). */
bool raceCountdownActive = false;
float raceCountdown = 0.0f;
bool showGoAnimation = false;
float goTimer = 0.0f;
const float goDuration = 0.8f;

/**
 * @brief Konfiguruje bufor ramki (FBO) dla renderowania sceny do tekstury.
 * @param width Szerokość render targetu.
 * @param height Wysokość render targetu.
 */
void setupFramebuffer(int width, int height);

/**
 * @brief Dane wierzchołków dla kostki skyboxa.
 */
float skyboxVertices[] = {
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

/**
 * @brief Ładuje 6 tekstur tworzących cubemap (skybox).
 * @param faces Lista ścieżek do tekstur w kolejności odpowiadającej ścianom cubemapy.
 * @return Identyfikator tekstury cubemap.
 */
unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

/**
 * @brief Buduje i linkuje prosty program shaderów dla skyboxa.
 * @return Identyfikator programu shaderów OpenGL.
 */
unsigned int createSkyboxShaderProgram() {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        out vec3 TexCoords;
        uniform mat4 projection;
        uniform mat4 view;
        void main()
        {
            TexCoords = aPos;
            vec4 pos = projection * view * vec4(aPos, 1.0);
            gl_Position = pos.xyww;
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        in vec3 TexCoords;
        uniform samplerCube skybox;
        void main()
        {
            FragColor = texture(skybox, TexCoords);
        }
    )";

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

/**
 * @brief Wczytuje teksturę 2D z pliku.
 * @param path Ścieżka do pliku.
 * @return Identyfikator tekstury OpenGL lub 0 w przypadku błędu.
 */
unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format = GL_RGB;
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

/**
 * @brief Callback wywoływany przy zmianie rozmiaru okna.
 *
 * Aktualizuje viewport oraz odtwarza FBO, jeśli jest aktywne.
 *
 * @param window Okno GLFW.
 * @param width Nowa szerokość.
 * @param height Nowa wysokość.
 */
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    current_width = width;
    current_height = height;
    if (FBO_Scene != 0) {
        setupFramebuffer(width, height);
    }
}

/**
 * @brief Callback klawiatury.
 *
 * Zarządza stanem klawiszy, skrótami globalnymi oraz przełączaniem trybów w UI.
 *
 * @param window Okno GLFW.
 * @param key Kod klawisza.
 * @param scancode Kod skanowania.
 * @param action Akcja (wciśnięcie/zwolnienie).
 * @param mods Modyfikatory.
 */
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    static bool handling = false;
    if (handling) return;
    handling = true;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (currentState == RACING) {
            currentState = MAIN_MENU;

            if (isAudioInit) {
                ma_sound_set_volume(&car_sound, 0.0f);
            }

            showSettings = false;
            showCarSelect = false;
            showTrackSelect = false;
        }
        else if (currentState == MAIN_MENU) {
            if (isAudioInit) {
                ma_sound_set_volume(&car_sound, 0.0f);
            }
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

/**
 * @brief Logika sterowania samochodem gracza.
 *
 * Mapuje klawisze WASD/SPACE na parametry fizyczne obiektu `RaceCar`
 * i uwzględnia blokadę sterowania podczas odliczania oraz animacji startu.
 *
 * @param deltaTime Czas trwania klatki.
 */
void processCarInput(float deltaTime) {
    if (!car || currentState != RACING) {
        return;
    }

    if (raceCountdownActive || showGoAnimation) {
        if (car) {
            car->ThrottleInput = 0.0f;
            car->SteeringInput = 0.0f;
            car->Handbrake = true;
        }
        return;
    }

    float currentSpeed = glm::length(car->Velocity);
    bool handbrakePressed = keys[GLFW_KEY_SPACE];

    float desiredThrottle = 0.0f;
    if (handbrakePressed) {
        desiredThrottle = 0.0f;
    }
    else {
        if (keys[GLFW_KEY_W]) {
            desiredThrottle = 1.0f;
        }
        else if (keys[GLFW_KEY_S]) {
            desiredThrottle = -1.0f;
        }
    }

    car->ThrottleInput = desiredThrottle;
    car->Handbrake = handbrakePressed;

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

/**
 * @brief Inicjalizuje prostokąt (quad) do post-processingu.
 *
 * Tworzy VAO/VBO prostokąta ekranowego z pozycją i UV.
 */
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

/**
 * @brief Konfiguracja bufora ramki (FBO) - tekstura koloru + bufor głębokości i szablonu.
 * @param width Szerokość render targetu.
 * @param height Wysokość render targetu.
 */
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

/**
 * @brief Renderuje ekran powitalny (Splash Screen) przy użyciu ImGui.
 */
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

/**
 * @brief Renderuje menu wyboru samochodu (garaż).
 *
 * Zawiera logikę kupowania i wybierania samochodów.
 */
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

/**
 * @brief Renderuje menu wyboru trasy.
 */
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

/**
 * @brief Renderuje menu ustawień (dźwięk, grafika, strojenie fizyki).
 */
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

    if (ImGui::SliderFloat("Volume", &masterVolume, 0.0f, 1.0f)) {
        if (isAudioInit) {
            ma_engine_set_volume(&audio_engine, masterVolume);
        }
    }

    static bool vsync = true;
    if (ImGui::Checkbox("V-Sync Enabled", &vsync)) {
        glfwSwapInterval(vsync ? 1 : 0);
    }

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

/**
 * @brief Renderuje główne menu gry.
 */
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
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);

    ImVec2 buttonSize(350, 60);
    float buttonX = (ImGui::GetWindowSize().x - buttonSize.x) * 0.5f;
    ImGui::SetWindowFontScale(1.5f);

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

    ImGui::SetCursorPosX(buttonX);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.25f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.5f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.4f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.5f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    if (ImGui::Button("START RACE", buttonSize)) {

        currentState = RACING;

        if (selectedLapOption == 0) {
            totalLaps = 1;
            raceTimeLeft = 60.0f;
        }
        else if (selectedLapOption == 1) {
            totalLaps = 3;
            raceTimeLeft = 180.0f;
        }
        else if (selectedLapOption == 2) {
            totalLaps = 10;
            raceTimeLeft = 600.0f;
        }
        else {
            totalLaps = 1;
            raceTimeLeft = 60.0f;
        }

        if (car) {
            glm::vec3 scaleFactor(0.1f);

            glm::vec3 startLeft(-127.81f, 0.0f, 204.38f);
            glm::vec3 startRight(-58.75f, 0.0f, 203.17f);
            glm::vec3 startPos = (startLeft + startRight) * 0.5f;
            glm::vec3 dir = glm::normalize(startRight - startLeft);

            float offsetBack = 95.0f;
            startPos -= dir * offsetBack;

            glm::vec3 rightVec = glm::normalize(glm::cross(dir, glm::vec3(0.0f, 1.0f, 0.0f)));
            float offsetSide = 5.0f;
            startPos -= rightVec * offsetSide;

            startPos *= scaleFactor;
            startPos -= rightVec * 0.3f;

            float startYaw = glm::degrees(atan2(dir.x, dir.z));
            car->Position = startPos;

            lapStartPosition = car->Position;
            raceElapsedTime = 0.0f;
            sessionMoney = 0;

            raceFinished = false;
            raceWon = false;
            leftStartZone = false;
            raceTimerActive = false;

            currentLap = 1;

            aiCurrentLap = 1;
            aiLeftStartZone = false;
            aiRaceFinished = false;
            aiRaceWon = false;

            trackForward = glm::normalize(dir);

            car->Velocity = glm::vec3(0.0f);
            car->Yaw = startYaw;
            car->FrontVector = glm::normalize(glm::vec3(sin(glm::radians(startYaw)), 0.0f, cos(glm::radians(startYaw))));

            glm::vec3 forward = glm::normalize(car->FrontVector);
            glm::vec3 leftVec = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), forward));

            float sideOffset = 0.3f;
            float backOffset = 0.0f;
            glm::vec3 aiStartPos = car->Position + leftVec * sideOffset - forward * backOffset;

            aiCar->Position = aiStartPos;
            aiCar->Velocity = glm::vec3(0.0f);
            aiCar->Yaw = car->Yaw;
            aiCar->FrontVector = car->FrontVector;

            aiCurrentWaypoint = 0;
        }

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

/**
 * @brief Rysuje mini-mapę w interfejsie.
 *
 * Przelicza współrzędne świata na współrzędne ekranowe UI, uwzględniając orientację mapy.
 *
 * @param draw Lista rysowania ImGui.
 * @param topLeft Lewy górny róg obszaru mini-mapy.
 * @param size Rozmiar mini-mapy.
 * @param playerPos Pozycja gracza w świecie.
 * @param aiPos Pozycja AI w świecie.
 */
static void DrawMiniMap(ImDrawList* draw, const ImVec2& topLeft, const ImVec2& size, const glm::vec3& playerPos, const glm::vec3& aiPos) {
    using namespace glm;
    const auto& walls = TrackCollision::GetWalls();
    if (walls.empty()) return;

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

    float pad = 0.05f;
    float scaleX = (size.x * (1.0f - 2.0f * pad)) / worldH;
    float scaleY = (size.y * (1.0f - 2.0f * pad)) / worldW;
    float scale = std::min(scaleX, scaleY);

    float extraX = (size.x - worldH * scale) * 0.5f;
    float extraY = (size.y - worldW * scale) * 0.5f;

    auto WorldToScreen = [&](float wx, float wz) -> ImVec2 {
        float nx = (wx - minX) / worldW;
        float nz = (wz - minZ) / worldH;

        float rx = (1.0f - nz) * (worldH * scale) + extraX;
        float ry = nx * (worldW * scale) + extraY;

        return ImVec2(topLeft.x + rx, topLeft.y + ry);
        };

    for (const auto& w : walls) {
        ImVec2 a = WorldToScreen(w.start.x, w.start.y);
        ImVec2 b = WorldToScreen(w.end.x, w.end.y);
        draw->AddLine(a, b, IM_COL32(180, 180, 180, 220), 2.0f);
    }

    ImVec2 pPos = WorldToScreen(playerPos.x, playerPos.z);
    ImVec2 aiP = WorldToScreen(aiPos.x, aiPos.z);

    float markerR = std::max(3.0f, std::min(size.x, size.y) * 0.03f);
    draw->AddCircleFilled(pPos, markerR, IM_COL32(255, 160, 40, 255));
    draw->AddCircleFilled(aiP, markerR, IM_COL32(40, 220, 100, 220));
}

/**
 * @brief Główna funkcja programu (punkt wejścia).
 * @return Kod zakończenia procesu (0 oznacza poprawne zakończenie).
 */
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

    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    std::vector<std::string> faces = {
        "assets/skybox/Maskonaive2/posx.jpg",
        "assets/skybox/Maskonaive2/negx.jpg",
        "assets/skybox/Maskonaive2/posy.jpg",
        "assets/skybox/Maskonaive2/negy.jpg",
        "assets/skybox/Maskonaive2/posz.jpg",
        "assets/skybox/Maskonaive2/negz.jpg"
    };
    skyboxTexture = loadCubemap(faces);
    skyboxShaderID = createSkyboxShaderProgram();

    splashTextureID = loadTexture("assets/Tlo.png");

    playerProfile.load();

    if (ma_engine_init(NULL, &audio_engine) == MA_SUCCESS) {
        if (ma_sound_init_from_file(&audio_engine, "assets/sound/loop_5.wav", 0, NULL, NULL, &car_sound) == MA_SUCCESS) {
            ma_sound_set_looping(&car_sound, MA_TRUE);
            ma_sound_set_volume(&car_sound, 0.0f);
            ma_sound_start(&car_sound);
            isAudioInit = true;
        }
        else {
            std::cout << "Failed to load car sound file!" << std::endl;
        }
    }
    else {
        std::cout << "Failed to init Audio Engine!" << std::endl;
    }

    car = new RaceCar(glm::vec3(0.0f, 0.1f, 0.0f));

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

    aiCar = new RaceCar(car->Position + glm::vec3(-3.0f, 0.0f, -3.0f));
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

    aiCar->MaxSpeed = car->MaxSpeed * 0.95f;

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
            if (isAudioInit) {
                ma_sound_set_volume(&car_sound, 0.0f);
            }
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

            if (isAudioInit && car) {
                float currentSpeed = glm::length(car->Velocity);
                float pitch = 0.5f + (currentSpeed / car->MaxSpeed) * 1.5f;
                ma_sound_set_pitch(&car_sound, pitch);
                ma_sound_set_volume(&car_sound, masterVolume);
            }

            if (raceTimerActive && !raceFinished) {
                raceTimeLeft -= deltaTime;
                raceElapsedTime += deltaTime;
                if (raceTimeLeft <= 0.0f) {
                    raceTimeLeft = 0.0f;
                    raceFinished = true;
                    raceWon = false;
                    raceTimerActive = false;

                    if (sessionMoney > 0) {
                        playerProfile.addMoney(sessionMoney);
                        playerProfile.save();
                    }
                }
            }

            if (!raceCountdownActive && !showGoAnimation) {
                processCarInput(deltaTime);

                glm::vec3 lastSafePos = car->Position;

                car->Update(deltaTime);

                if (aiCar && !aiWaypoints.empty()) {

                    glm::vec3 target = aiWaypoints[aiCurrentWaypoint];
                    glm::vec3 toTarget = target - aiCar->Position;
                    float distance = glm::length(toTarget);

                    if (distance < aiWaypointRadius) {
                        aiCurrentWaypoint++;
                        if (aiCurrentWaypoint >= aiWaypoints.size())
                            aiCurrentWaypoint = 0;
                    }

                    float desiredYaw = glm::degrees(atan2(toTarget.x, toTarget.z));
                    float yawDiff = desiredYaw - aiCar->Yaw;

                    while (yawDiff > 180.0f) yawDiff -= 360.0f;
                    while (yawDiff < -180.0f) yawDiff += 360.0f;

                    aiCar->SteeringInput = glm::clamp(yawDiff / 25.0f, -1.0f, 1.0f);

                    float absYaw = fabs(yawDiff);
                    if (absYaw > 60.0f)
                        aiCar->ThrottleInput = 0.4f;
                    else if (absYaw > 30.0f)
                        aiCar->ThrottleInput = 0.7f;
                    else
                        aiCar->ThrottleInput = 1.0f;

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

                    float aiSpeed = glm::length(aiCar->Velocity);
                    if (aiSpeed > aiCar->MaxSpeed)
                        aiCar->Velocity = glm::normalize(aiCar->Velocity) * aiCar->MaxSpeed;
                }

                float speed = glm::length(car->Velocity);
                if (speed > car->MaxSpeed) {
                    car->Velocity = glm::normalize(car->Velocity) * car->MaxSpeed;
                }

                float dist = glm::distance(aiCar->Position, lapStartPosition);
                if (!aiLeftStartZone && dist > lapFinishRadius * 2.0f) aiLeftStartZone = true;

                if (aiLeftStartZone && dist < lapFinishRadius && !aiRaceFinished) {
                    aiCurrentLap++;
                    if (aiCurrentLap > totalLaps) {
                        aiRaceFinished = true;

                        if (!raceFinished) {
                            aiRaceWon = true;
                            raceWon = false;
                            raceFinished = true;
                            raceTimerActive = false;

                            if (sessionMoney > 0) {
                                playerProfile.addMoney(sessionMoney);
                                playerProfile.save();
                            }
                        }

                        aiCar->Velocity = glm::vec3(0.0f);
                    }
                    aiLeftStartZone = false;
                }

                dist = glm::distance(car->Position, lapStartPosition);
                if (!leftStartZone && dist > lapFinishRadius * 2.0f) leftStartZone = true;

                bool isMovingForward = glm::dot(car->FrontVector, trackForward) > 0.0f;

                if (leftStartZone && dist < lapFinishRadius && raceTimerActive && isMovingForward) {
                    currentLap++;

                    sessionMoney += 50;

                    if (currentLap > totalLaps) {
                        raceFinished = true;

                        if (!aiRaceFinished) {
                            raceWon = true;
                            aiRaceWon = false;

                            int bonus = 0;
                            if (totalLaps == 1) bonus = 100;
                            else if (totalLaps == 3) bonus = 300;
                            else if (totalLaps == 10) bonus = 1000;
                            else bonus = 100 * totalLaps;

                            sessionMoney += bonus;
                        }
                        else {
                            raceWon = false;
                            aiRaceWon = true;
                        }

                        if (sessionMoney > 0) {
                            playerProfile.addMoney(sessionMoney);
                            playerProfile.save();
                        }
                    }

                    leftStartZone = false;
                }

                if (selectedTrack == 2) {
                    TrackCollision track;

                    if (track.CheckCollision(car->Position, 0.35f)) {
                        car->Position = lastSafePos;
                        car->Velocity *= -0.25f;
                    }
                }

                static float logTimer = 0.0f;
                logTimer += deltaTime;
                if (keys[GLFW_KEY_P] && logTimer > 0.2f) {
                    std::cout << "AKTUALNA POZYCJA: X: " << car->Position.x << " Z: " << car->Position.z << std::endl;
                    logTimer = 0.0f;
                }

            }
            else {

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
                    camera->SetFrontCamera(car->Position, car->Yaw);
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
            glm::mat4 projection = camera->GetProjectionMatrix((float)current_width / (float)current_height);
            carTrackShader.setMat4("projection", projection);
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

        if (aiCar && currentState == RACING) {
            carTrackShader.use();
            carTrackShader.setVec3("objectColor", glm::vec3(0.2f, 0.8f, 0.2f));
            aiCar->Draw(carTrackShader);
            carTrackShader.setVec3("objectColor", carCustomColor);
        }

        if (camera) {
            glDepthFunc(GL_LEQUAL);
            glUseProgram(skyboxShaderID);

            glm::mat4 view = glm::mat4(glm::mat3(camera->GetViewMatrix()));
            glm::mat4 projection = camera->GetProjectionMatrix((float)current_width / (float)current_height);

            glUniformMatrix4fv(glGetUniformLocation(skyboxShaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(skyboxShaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

            glBindVertexArray(skyboxVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
            glDepthFunc(GL_LESS);
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

            if (raceCountdownActive) {
                int displayNum = (int)std::ceil(raceCountdown);
                float frac = raceCountdown - std::floor(raceCountdown);
                float progress = 1.0f - frac;
                progress = glm::clamp(progress, 0.0f, 1.0f);

                float scale = glm::mix(2.0f, 6.0f, progress);
                float alpha = glm::mix(0.15f, 1.0f, progress);

                std::string txt = std::to_string(displayNum);

                ImGui::SetNextWindowPos(ImVec2(current_width * 0.5f, current_height * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                ImGui::SetNextWindowSize(ImVec2(800, 480));
                ImGui::Begin("Race Countdown", nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings);

                ImGui::SetWindowFontScale(scale);
                ImVec2 winSize = ImGui::GetWindowSize();
                ImVec2 textSize = ImGui::CalcTextSize(txt.c_str());
                float x = (winSize.x - textSize.x) * 0.5f;
                float y = (winSize.y - textSize.y) * 0.45f;

                ImGui::SetCursorPos(ImVec2(x + 6.0f, y + 6.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, alpha * 0.9f));
                ImGui::TextUnformatted(txt.c_str());
                ImGui::PopStyleColor();

                ImGui::SetCursorPos(ImVec2(x, y));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.1f, alpha));
                ImGui::TextUnformatted(txt.c_str());
                ImGui::PopStyleColor();

                ImGui::SetWindowFontScale(1.0f);
                ImGui::End();

                ImGui::SetNextWindowPos(ImVec2(10, current_height - 60));

            }
            else if (showGoAnimation) {
                float t = glm::clamp(goTimer / goDuration, 0.0f, 1.0f);
                float scale = glm::mix(3.0f, 8.0f, t);
                float alpha = glm::mix(0.0f, 1.0f, t);
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

                ImGui::SetCursorPos(ImVec2(x + 8.0f, y + 8.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, alpha * 0.95f));
                ImGui::TextUnformatted(txt.c_str());
                ImGui::PopStyleColor();

                ImGui::SetCursorPos(ImVec2(x, y));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.95f, 0.25f, alpha));
                ImGui::TextUnformatted(txt.c_str());
                ImGui::PopStyleColor();

                ImGui::SetWindowFontScale(1.0f);
                ImGui::End();

                ImGui::SetNextWindowPos(ImVec2(10, current_height - 60));
            }

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

                draw->AddRectFilled(
                    pos,
                    ImVec2(pos.x + size.x, pos.y + size.y),
                    IM_COL32(10, 10, 10, 200),
                    18.0f
                );

                draw->AddRect(
                    pos,
                    ImVec2(pos.x + size.x, pos.y + size.y),
                    IM_COL32(255, 140, 40, 220),
                    18.0f,
                    0,
                    2.5f
                );

                ImGui::Dummy(ImVec2(0, 8));

                float speedKmh = glm::length(car->Velocity) * 3.6f;
                ImGui::SetWindowFontScale(2.6f);
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.15f, 1.0f), "%.0f km/h", speedKmh);

                ImGui::SetWindowFontScale(1.0f);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "SPEED");

                ImGui::Separator();

                ImGui::SetWindowFontScale(1.8f);
                ImVec4 timeColor =
                    raceTimeLeft < 10.0f
                    ? ImVec4(1.0f, 0.2f, 0.2f, 1.0f)
                    : ImVec4(0.2f, 0.9f, 1.0f, 1.0f);

                int minLeft = (int)raceTimeLeft / 60;
                int secLeft = (int)raceTimeLeft % 60;

                ImGui::TextColored(timeColor, "TIME: %02d:%02d", minLeft, secLeft);

                ImGui::SetWindowFontScale(1.0f);

                ImGui::Separator();

                ImGui::SetWindowFontScale(1.4f);
                ImGui::TextColored(
                    ImVec4(0.9f, 0.9f, 0.9f, 1.0f),
                    "LAP %d / %d",
                    currentLap - 1,
                    totalLaps
                );
                ImGui::SetWindowFontScale(1.0f);

                ImGui::Separator();

                ImGui::TextColored(
                    ImVec4(0.8f, 0.8f, 0.8f, 1.0f),
                    "CAMERA: %s",
                    cockpitView ? "COCKPIT" : "CHASE"
                );

                ImGui::End();

                ImGui::SetNextWindowPos(ImVec2(current_width - 220, current_height - 220), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(200, 200));
                ImGui::Begin("MiniMap", nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);

                ImDrawList* mdraw = ImGui::GetWindowDrawList();
                ImVec2 mpos = ImGui::GetWindowPos();
                ImVec2 msize = ImGui::GetWindowSize();

                mdraw->AddRectFilled(mpos, ImVec2(mpos.x + msize.x, mpos.y + msize.y), IM_COL32(8, 8, 8, 200), 8.0f);
                mdraw->AddRect(mpos, ImVec2(mpos.x + msize.x, mpos.y + msize.y), IM_COL32(160, 160, 160, 120), 8.0f, 0, 2.0f);

                DrawMiniMap(mdraw, ImVec2(mpos.x + 8.0f, mpos.y + 8.0f), ImVec2(msize.x - 16.0f, msize.y - 16.0f), car->Position, aiCar ? aiCar->Position : glm::vec3(0.0f));

                ImGui::End();

                ImGui::SetNextWindowPos(ImVec2(10, 10));
                ImGui::Begin("RaceMenu", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
                if (ImGui::Button("BACK TO MAIN MENU", ImVec2(200, 40))) {
                    currentState = MAIN_MENU;

                    if (isAudioInit) {
                        ma_sound_set_volume(&car_sound, 0.0f);
                    }
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

            draw->AddRectFilled(
                pos,
                ImVec2(pos.x + size.x, pos.y + size.y),
                IM_COL32(10, 15, 20, 220),
                20.0f
            );

            draw->AddRect(
                pos,
                ImVec2(pos.x + size.x, pos.y + size.y),
                IM_COL32(0, 200, 255, (int)(200 * glow)),
                20.0f,
                0,
                3.0f
            );

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
                ? ImVec4(1.0f, 0.75f, 0.2f, alpha)
                : ImVec4(1.0f, 0.2f, 0.2f, alpha)
            );
            ImGui::TextUnformatted(title);
            ImGui::PopStyleColor();

            ImGui::SetWindowFontScale(1.0f);

            ImGui::SetCursorPos(ImVec2(40, 110));
            ImGui::TextColored(ImVec4(0.6f, 0.9f, 1.0f, alpha), "TIME");

            ImGui::SameLine(140);

            int eMin = (int)raceElapsedTime / 60;
            int eSec = (int)raceElapsedTime % 60;

            ImGui::TextColored(
                ImVec4(1.0f, 1.0f, 1.0f, alpha),
                "%02d:%02d",
                eMin, eSec
            );

            ImGui::SetCursorPos(ImVec2(40, 145));
            ImGui::TextColored(ImVec4(0.6f, 0.9f, 1.0f, alpha), "LAPS");

            ImGui::SameLine(140);
            ImGui::TextColored(
                ImVec4(1.0f, 1.0f, 1.0f, alpha),
                "%d / %d",
                std::min(currentLap - 1, totalLaps),
                totalLaps
            );

            ImGui::SetCursorPos(ImVec2(40, 180));
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.1f, alpha), "MONEY");
            ImGui::SameLine(140);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, alpha), "+ %d $", sessionMoney);

            ImGui::SetCursorPos(ImVec2(size.x - 150, size.y - 60));

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.15f, 0.2f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.6f, 1.0f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.4f, 0.8f, 1.0f));

            if (ImGui::Button("MENU", ImVec2(120, 36))) {
                currentState = MAIN_MENU;

                if (isAudioInit) {
                    ma_sound_set_volume(&car_sound, 0.0f);
                }

                raceFinished = false;
                raceWon = false;
                raceTimerActive = false;
                sessionMoney = 0;

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

    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
    glDeleteTextures(1, &skyboxTexture);
    glDeleteProgram(skyboxShaderID);

    glDeleteFramebuffers(1, &FBO_Scene);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);

    delete car;
    delete aiCar;
    delete camera;
    delete track;
    delete city;
    delete kartingMap;

    if (isAudioInit) {
        ma_sound_uninit(&car_sound);
        ma_engine_uninit(&audio_engine);
    }

    glfwTerminate();
    return 0;
}