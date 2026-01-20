#include "Karting.h"
#include "Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

// Konstruktor inicjalizujący podstawowe wartości transformacji oraz zerujący uchwyty OpenGL.
Karting::Karting(glm::vec3 startPosition)
    : Position(startPosition), Scale(glm::vec3(1.0f)), Yaw(0.0f),
    VAO(0), VBO(0), EBO(0) {
}

// Główna funkcja parsująca plik obj.
// Czyta plik linia po linii i interpretuje dane wierzchołków, normalnych, tekstur oraz ścian (faces).
bool Karting::loadModel(const std::string& modelPath) {
    std::ifstream file(modelPath);
    if (!file.is_open()) {
        std::cout << "Nie mogк otworzyж pliku kartingu: " << modelPath << std::endl;
        return false;
    }

    // Tymczasowe bufory na dane surowe z pliku.
    // Pliki OBJ indeksują te dane osobno, my musimy je "spłaszczyć" do formatu zrozumiałego dla OpenGL.
    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec3> temp_normals;
    std::vector<glm::vec2> temp_texcoords;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix; // Pierwszy token w linii określa typ danych.

        if (prefix == "v") {
            // v x y z -> Pozycja wierzchołka
            glm::vec3 v;
            iss >> v.x >> v.y >> v.z;
            temp_vertices.push_back(v);
        }
        else if (prefix == "vn") {
            // vn x y z -> Wektor normalny
            glm::vec3 n;
            iss >> n.x >> n.y >> n.z;
            temp_normals.push_back(n);
        }
        else if (prefix == "vt") {
            // vt u v -> Współrzędna tekstury
            glm::vec2 t;
            iss >> t.x >> t.y;
            temp_texcoords.push_back(t);
        }
        else if (prefix == "f") {
            // f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3 ... -> Definicja ściany (trójkąta lub czworokąta).
            std::string a, b, c, d;
            iss >> a >> b >> c >> d;

            // Obsługa zarówno trójkątów, jak i czworokątów (triangulacja prostokąta na dwa trójkąty: abc i acd).
            std::vector<std::string> faces = { a, b, c };
            if (!d.empty()) faces = { a, b, c, a, c, d };

            for (auto& f : faces) {
                // Rozdzielanie ciągu "v/vt/vn"
                std::istringstream parts(f);
                std::string v, t, n;
                std::getline(parts, v, '/');
                std::getline(parts, t, '/');
                std::getline(parts, n, '/');

                // Konwersja indeksów (OBJ liczy od 1, C++ od 0).
                int vi = v.empty() ? 0 : std::stoi(v) - 1;
                int ti = t.empty() ? 0 : std::stoi(t) - 1;
                int ni = n.empty() ? 0 : std::stoi(n) - 1;

                // Budowanie finalnej struktury wierzchołka.
                // Kopiujemy dane z tymczasowych buforów do ostatecznych wektorów.
                vertices.push_back(temp_vertices[vi]);

                // Zabezpieczenie przed ujemnymi/błędnymi indeksami tekstur i normalnych.
                texCoords.push_back(ti >= 0 && ti < temp_texcoords.size() ? temp_texcoords[ti] : glm::vec2(0));
                normals.push_back(ni >= 0 && ni < temp_normals.size() ? temp_normals[ni] : glm::vec3(0, 1, 0));

                // Dodajemy nowy kolejny indeks (nie optymalizujemy duplikatów w tym prostym parserze).
                indices.push_back(indices.size());
            }
        }
    }
    file.close();

    // Jeśli model nie posiadał normalnych, obliczamy je ręcznie, aby światło działało.
    if (normals.empty() || normals.size() != vertices.size())
        calculateNormals();

    // Przesłanie gotowych danych do OpenGL.
    setupMesh();
    std::cout << "Zaіadowano trasк karting — " << vertices.size() << " wierzchoіkуw" << std::endl;
    return true;
}

// Algorytm generowania normalnych dla powierzchni (flat/smooth shading).
// Oblicza iloczyn wektorowy dla każdego trójkąta i akumuluje wynik w wierzchołkach.
void Karting::calculateNormals() {
    normals.resize(vertices.size(), glm::vec3(0));
    // Iterujemy po trójkątach (co 3 indeksy).
    for (size_t i = 0; i < indices.size(); i += 3) {
        glm::vec3 v0 = vertices[indices[i]];
        glm::vec3 v1 = vertices[indices[i + 1]];
        glm::vec3 v2 = vertices[indices[i + 2]];

        // Obliczamy wektor normalny dla płaszczyzny trójkąta (iloczyn wektorowy krawędzi).
        glm::vec3 n = glm::normalize(glm::cross(v1 - v0, v2 - v0));

        // Dodajemy ten wektor do każdego wierzchołka trójkąta.
        normals[indices[i]] += n;
        normals[indices[i + 1]] += n;
        normals[indices[i + 2]] += n;
    }
    // Normalizujemy wynikowe wektory (uśrednianie kierunków sąsiednich ścian).
    for (auto& n : normals) n = glm::length(n) > 0 ? glm::normalize(n) : glm::vec3(0, 1, 0);
}

// Konfiguracja buforów VAO/VBO/EBO i atrybutów wierzchołków.
void Karting::setupMesh() {
    if (vertices.empty()) return;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // Pakowanie danych do jednego wektora "interleaved" (przeplatanego).
    // Format: [x, y, z, nx, ny, nz, u, v,   x, y, z, ... ]
    std::vector<float> data;
    for (size_t i = 0; i < vertices.size(); ++i) {
        // Pozycja (3 floaty)
        data.push_back(vertices[i].x);
        data.push_back(vertices[i].y);
        data.push_back(vertices[i].z);

        // Normalna (3 floaty)
        data.push_back(normals[i].x);
        data.push_back(normals[i].y);
        data.push_back(normals[i].z);

        // Tekstura UV (2 floaty)
        data.push_back(texCoords[i].x);
        data.push_back(texCoords[i].y);
    }

    // Przesłanie danych wierzchołków do VBO.
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);

    // Przesłanie indeksów do EBO.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Konfiguracja wskaźników atrybutów (Vertex Attrib Pointers).
    // Stride = 8 * sizeof(float) (3 poz + 3 norm + 2 uv).

    // Atrybut 0: Pozycja (offset 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    // Atrybut 1: Normalna (offset 3 * float)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

    // Atrybut 2: UV (offset 6 * float)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    glBindVertexArray(0); // Odpinamy VAO.
}

// Oblicza macierz modelu: Translacja -> Rotacja -> Skala.
glm::mat4 Karting::GetModelMatrix() const {
    glm::mat4 model(1);
    model = glm::translate(model, Position);
    model = glm::rotate(model, glm::radians(Yaw), glm::vec3(0, 1, 0));
    model = glm::scale(model, Scale);
    return model;
}

// Funkcja rysująca model.
void Karting::Draw(const Shader& shader, glm::vec3 pos, float yaw) const {
    if (!VAO) return;

    // Pobieramy i ustawiamy macierz w shaderze.
    glm::mat4 model = GetModelMatrix();
    shader.setMat4("model", model);
    shader.setBool("useTexture", true); // Włączamy teksturowanie w shaderze.

    // Rysowanie z wykorzystaniem indeksów (DrawElements).
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}