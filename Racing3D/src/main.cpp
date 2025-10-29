#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

// Musisz mieæ w projekcie bibliotekê GLM!
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Nasze klasy
#include "Shader.h"
#include "Camera.h"
#include "Car.h"
#include "Track.h" // Wymagany nag³ówek Toru

// Sta³e
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Globalne zmienne (do kontroli obiektów i czasu)
Car* car;
Camera* camera;
Track* track; // Obiekt Toru
bool keys[1024];
float lastFrame = 0.0f;

// Callback zmieniaj¹cy viewport (pozostaje)
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Funkcja obs³ugi klawiatury (do ci¹g³ego sprawdzania stanu klawiszy)
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }
}

// Funkcja obs³uguj¹ca sterowanie samochodem
void processCarInput(float deltaTime) {
    float currentSpeed = glm::length(car->Velocity);

    if (keys[GLFW_KEY_W]) {
        car->Velocity += car->FrontVector * car->Acceleration * deltaTime;
    }
    if (keys[GLFW_KEY_S]) {
        car->Velocity -= car->FrontVector * car->Braking * deltaTime;
    }

    // Ograniczenie maksymalnej prêdkoœci
    if (glm::length(car->Velocity) > car->MaxSpeed) {
        car->Velocity = glm::normalize(car->Velocity) * car->MaxSpeed;
    }

    // Skrêcanie - tylko gdy samochód siê rusza
    if (currentSpeed > 0.1f || glm::length(car->Velocity) < -0.1f) {
        float turnAmount = car->TurnRate * deltaTime * 50.0f;

        if (keys[GLFW_KEY_A]) {
            car->Yaw += turnAmount;
        }
        if (keys[GLFW_KEY_D]) {
            car->Yaw -= turnAmount;
        }

        // Aktualizacja wektora Front na podstawie nowego k¹ta Yaw
        car->FrontVector.x = cos(glm::radians(car->Yaw)) * cos(glm::radians(0.0f));
        car->FrontVector.z = sin(glm::radians(car->Yaw)) * cos(glm::radians(0.0f));
        car->FrontVector = glm::normalize(car->FrontVector);
    }
}

int main() {
    // 1. Inicjalizacja GLFW
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. Tworzenie okna
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Racing3D", nullptr, nullptr);
    if (!window) { /* B³¹d */ glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    // 3. £adowanie GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { /* B³¹d */ return -1; }

    // Ustawienia 3D
    glEnable(GL_DEPTH_TEST);

    // 4. Inicjalizacja obiektów gry
    Shader carTrackShader("shaders/phong.vert", "shaders/phong.frag");

    car = new Car(glm::vec3(0.0f, 0.5f, 0.0f));
    camera = new Camera(glm::vec3(0.0f, 3.0f, 5.0f));
    track = new Track(); // Inicjalizacja Toru

    // Pêtla renderowania
    while (!glfwWindowShouldClose(window)) {
        // 5. Obliczenie DeltaTime
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // 6. Wejœcie i Aktualizacja
        processCarInput(deltaTime);
        car->Update(deltaTime);
        camera->FollowCar(car->Position, car->FrontVector);

        // 7. Renderowanie
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Ustawienie shadera
        carTrackShader.use();
        carTrackShader.setMat4("view", camera->GetViewMatrix());
        carTrackShader.setMat4("projection", camera->GetProjectionMatrix((float)SCR_WIDTH / (float)SCR_HEIGHT));

        // --- Uniformy Oœwietlenia ---
        glm::vec3 lightPos(5.0f, 10.0f, 5.0f);
        glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
        glm::vec3 viewPos = camera->Position;

        carTrackShader.setVec3("viewPos", viewPos);
        carTrackShader.setVec3("lightPos", lightPos);
        carTrackShader.setVec3("lightColor", lightColor);

        // --- Rysowanie Toru ---
        glm::vec3 trackColor(0.2f, 0.4f, 0.2f);
        carTrackShader.setVec3("objectColor", trackColor);
        track->Draw(carTrackShader);

        // --- Rysowanie Samochodu ---
        glm::vec3 carColor(1.0f, 0.5f, 0.2f);
        carTrackShader.setVec3("objectColor", carColor);
        car->Draw(carTrackShader);

        // 8. Zamiana buforów i obs³uga zdarzeñ
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 9. Sprz¹tanie
    delete car;
    delete camera;
    delete track; // Czyszczenie pamiêci Toru
    glfwTerminate();
    return 0;
}