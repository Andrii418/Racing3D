#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <vector>
#include <string>
#include "Track.h"

class Shader;

// Klasa reprezentująca mapę gokartową.
// Dziedziczy po klasie Track, co pozwala traktować ją jako specyficzny rodzaj toru.
// Odpowiada za wczytanie modelu mapy z pliku .obj, przetworzenie danych geometrycznych
// i wyświetlenie ich przy użyciu OpenGL.
class Karting : public Track {

public:
    // Parametry transformacji obiektu w świecie gry.
    glm::vec3 Position; // Pozycja środka mapy.
    glm::vec3 Scale;    // Skala (powiększenie/pomniejszenie modelu).
    float Yaw;          // Obrót wokół osi Y (w stopniach).

    // Konstruktor: ustawia pozycję startową.
    Karting(glm::vec3 startPosition = glm::vec3(0.0f));

    // Wczytuje model 3D z pliku OBJ. Domyślnie ładuje "assets/karting/gp.obj".
    // Zwraca true, jeśli operacja się powiedzie.
    bool loadModel(const std::string& modelPath = "assets/karting/gp.obj");

    // Rysuje mapę przy użyciu podanego shadera.
    // Argumenty pos i yaw pozwalają na ewentualne nadpisanie transformacji (rzadko używane dla statycznej mapy).
    void Draw(const Shader& shader, glm::vec3 pos = glm::vec3(0.0f), float yaw = 0.0f) const;

    // Oblicza macierz modelu (Model Matrix), która przekształca wierzchołki z przestrzeni lokalnej do świata.
    glm::mat4 GetModelMatrix() const;

private:
    // Konfiguruje bufory OpenGL (VAO, VBO, EBO) i przesyła dane na kartę graficzną.
    void setupMesh();

    // Oblicza wektory normalne dla powierzchni, jeśli brakuje ich w pliku modelu.
    // Jest to niezbędne do poprawnego obliczenia oświetlenia.
    void calculateNormals();

    // Kontenery na surowe dane geometryczne.
    std::vector<glm::vec3> vertices;  // Pozycje wierzchołków (x, y, z).
    std::vector<glm::vec3> normals;   // Wektory normalne (nx, ny, nz).
    std::vector<glm::vec2> texCoords; // Współrzędne teksturowania (u, v).
    std::vector<unsigned int> indices; // Indeksy wierzchołków tworzące trójkąty (dla EBO).

    // Uchwyty (ID) do obiektów OpenGL.
    unsigned int VAO; // Vertex Array Object - przechowuje konfigurację atrybutów.
    unsigned int VBO; // Vertex Buffer Object - przechowuje dane wierzchołków.
    unsigned int EBO; // Element Buffer Object - przechowuje indeksy.
};