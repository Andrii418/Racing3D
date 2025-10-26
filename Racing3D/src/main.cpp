#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

// Callback zmieniaj¹cy viewport
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Funkcja dodaj¹ca prostok¹t do wektora wierzcho³ków
void addRect(std::vector<float>& vertices, float x, float y, float w, float h) {
    float rect[] = {
        x,     y + h, 0.0f,
        x,     y,     0.0f,
        x + w, y,     0.0f,

        x,     y + h, 0.0f,
        x + w, y,     0.0f,
        x + w, y + h, 0.0f
    };
    vertices.insert(vertices.end(), rect, rect + 18);
}

// Funkcja dodaj¹ca ko³o (przybli¿one wielok¹tem)
void addCircle(std::vector<float>& vertices, float cx, float cy, float r, int segments = 20) {
    for (int i = 0; i < segments; ++i) {
        float theta1 = 2.0f * 3.1415926f * i / segments;
        float theta2 = 2.0f * 3.1415926f * (i + 1) / segments;

        vertices.push_back(cx);
        vertices.push_back(cy);
        vertices.push_back(0.0f);

        vertices.push_back(cx + r * cos(theta1));
        vertices.push_back(cy + r * sin(theta1));
        vertices.push_back(0.0f);

        vertices.push_back(cx + r * cos(theta2));
        vertices.push_back(cy + r * sin(theta2));
        vertices.push_back(0.0f);
    }
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Racing3D", nullptr, nullptr);
    if (!window) return -1;
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glViewport(0, 0, 800, 600);

    // Vertex shader
    const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    void main() {
        gl_Position = vec4(aPos, 1.0);
    }
    )";

    // Fragment shader
    const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    void main() {
        FragColor = vec4(1.0, 0.5, 0.2, 1.0); // pomarañczowy samochodzik
    }
    )";

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Przygotowanie wierzcho³ków samochodzika
    std::vector<float> vertices;
    // Korpus samochodu
    addRect(vertices, -0.5f, -0.2f, 1.0f, 0.3f); // prostok¹t korpusu
    addRect(vertices, -0.3f, 0.1f, 0.6f, 0.2f);  // dach samochodu
    // Ko³a
    addCircle(vertices, -0.35f, -0.2f, 0.1f);
    addCircle(vertices, 0.35f, -0.2f, 0.1f);

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Pêtla renderowania
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
