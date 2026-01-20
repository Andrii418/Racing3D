#include "RaceCar.h"
#include "Shader.h"
#include "stb_image.h" 
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

// Metoda konfigurująca bufory OpenGL dla pojedynczej siatki (Mesh).
// Przesyła dane wierzchołków, normalnych i UV do karty graficznej.
void CarMesh::setupMesh() {
    if (vertices.empty()) return;

    // Usuwamy stare bufory, jeśli istnieją, aby uniknąć wycieków pamięci przy przeładowywaniu.
    if (VAO != 0) { glDeleteVertexArrays(1, &VAO); glDeleteBuffers(1, &VBO); glDeleteBuffers(1, &EBO); }

    // Generujemy nowe bufory.
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO); glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // Pakujemy dane do jednego wektora interleaved: [x,y,z, nx,ny,nz, u,v, ...]
    std::vector<float> data;
    for (size_t i = 0; i < vertices.size(); ++i) {
        data.push_back(vertices[i].x); data.push_back(vertices[i].y); data.push_back(vertices[i].z);
        data.push_back(normals[i].x);  data.push_back(normals[i].y);  data.push_back(normals[i].z);
        data.push_back(texCoords[i].x); data.push_back(texCoords[i].y);
    }

    // Przesyłamy dane wierzchołków (VBO).
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(data.size() * sizeof(float)), data.data(), GL_STATIC_DRAW);

    // Przesyłamy indeksy (EBO).
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);

    // Konfigurujemy atrybuty wierzchołków (mówimy shaderom, jak czytać dane).
    // Atrybut 0: Pozycja (vec3)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    // Atrybut 1: Normalna (vec3) - przesunięcie o 3 floaty
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    // Atrybut 2: Tekstura UV (vec2) - przesunięcie o 6 floatów
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);

    glBindVertexArray(0); // Odpinamy VAO.
}

// Konstruktor: Inicjalizuje pozycję startową, zeruje prędkość i ustawia domyślny wektor przodu.
RaceCar::RaceCar(glm::vec3 startPosition) : Position(startPosition), PreviousPosition(startPosition), Velocity(0.0f), Yaw(0.0f), textureID(0) { FrontVector = glm::vec3(0, 0, 1); }

// Metoda czyszcząca pamięć po siatkach samochodu.
void RaceCar::cleanup() {
    auto del = [](CarMesh& m) {
        if (m.VAO) { glDeleteVertexArrays(1, &m.VAO); glDeleteBuffers(1, &m.VBO); glDeleteBuffers(1, &m.EBO); m.VAO = 0; }
        m.vertices.clear(); m.indices.clear(); m.normals.clear(); m.texCoords.clear();
        };
    del(bodyMesh); del(wheelFrontMesh); del(wheelBackMesh);
}

// Ładuje modele i tekstury dla samochodu. Obsługuje różne ścieżki dla kół przednich/tylnych.
bool RaceCar::loadAssets(const std::string& bodyPath, const std::string& wheelFrontPath, const std::string& wheelBackPath) {
    cleanup(); // Czyścimy stare modele.

    // Ładujemy teksturę tylko raz (jeśli jeszcze nie załadowana).
    if (textureID == 0) textureID = loadTexture("assets/cars/OBJ format/Textures/colormap.png");

    // Ładujemy poszczególne części samochodu.
    if (!loadObj(bodyPath, bodyMesh, true)) return false;
    if (!loadObj(wheelFrontPath, wheelFrontMesh, false)) return false;

    // Jeśli nie podano osobnego modelu dla tylnych kół, używamy modelu przednich.
    std::string bPath = wheelBackPath.empty() ? wheelFrontPath : wheelBackPath;
    if (!loadObj(bPath, wheelBackMesh, false)) return false;

    return true;
}

// Główna funkcja aktualizacji fizyki samochodu (wywoływana co klatkę).
void RaceCar::Update(float deltaTime) {
    PreviousPosition = Position; // Zapamiętujemy pozycję dla kolizji.

    // 1. Aktualizacja wektora przodu na podstawie kąta skrętu (Yaw).
    FrontVector.x = sin(glm::radians(Yaw));
    FrontVector.z = cos(glm::radians(Yaw));
    FrontVector.y = 0.0f; // Samochód jeździ tylko w poziomie.
    FrontVector = glm::normalize(FrontVector);

    // 2. Wygładzanie wejścia przepustnicy (symulacja bezwładności silnika).
    float tResponse = glm::clamp(ThrottleResponse * deltaTime, 0.0f, 1.0f);
    Throttle = glm::mix(Throttle, ThrottleInput, tResponse);

    glm::vec3 forward = FrontVector;

    // 3. Rozdzielenie prędkości na składową wzdłużną (w przód/tył) i boczną (poślizg).
    float forwardSpeed = glm::dot(Velocity, forward);
    glm::vec3 velLong = forward * forwardSpeed;
    glm::vec3 velLat = Velocity - velLong;

    // 4. Aplikowanie siły silnika (Przyspieszanie / Hamowanie).
    if (Throttle > 0.01f && !Handbrake) {
        // Gaz: Przyspieszamy w przód.
        forwardSpeed += (Throttle * Acceleration) * deltaTime;
    }
    else if (Throttle < -0.01f) {
        // Hamulec/Wsteczny: Zwalniamy lub cofamy.
        forwardSpeed -= (-Throttle * Braking) * deltaTime;
    }

    // 5. Ogranicznik prędkości silnika (nie pozwala przekroczyć MaxSpeed).
    forwardSpeed = glm::clamp(forwardSpeed, -MaxSpeed, MaxSpeed);
    velLong = forward * forwardSpeed;

    // 6. Fizyka Hamulca Ręcznego i Driftu.
    float effectiveGrip = Grip;
    if (Handbrake) {
        // Silne hamowanie prędkości wzdłużnej.
        forwardSpeed *= powf(HandbrakeDeceleration, deltaTime * 60.0f);
        velLong = forward * forwardSpeed;

        // Zmniejszenie przyczepności (łatwiejsze wpadanie w poślizg).
        effectiveGrip *= (1.0f - HandbrakeGripReduction);

        // Dodanie sztucznej siły bocznej, aby zarzucić tyłem przy skręcie na ręcznym.
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
        float slideForce = (-SteeringInput) * 0.5f * (glm::clamp(std::abs(forwardSpeed), 0.0f, MaxSpeed) / MaxSpeed);
        velLat += right * slideForce * 10.0f * deltaTime;
    }

    // 7. Ponowne złożenie wektora prędkości i aplikowanie oporu powietrza/toczenia.
    Velocity = velLong + velLat;
    float currentSpeed = glm::length(Velocity);

    // Opór aerodynamiczny (kwadratowy, działa mocniej przy dużej prędkości).
    if (currentSpeed > 0.001f) {
        Velocity += (-glm::normalize(Velocity) * (AerodynamicDrag * currentSpeed * currentSpeed)) * deltaTime;
    }
    // Opór toczenia (liniowy, działa zawsze, zatrzymuje samochód).
    Velocity -= Velocity * RollingResistance * deltaTime;

    // 8. Tarcie Boczne (Zapobieganie niekontrolowanemu ślizganiu na boki).
    // Jeśli przyczepność jest wysoka, niwelujemy prędkość boczną. Ręczny zmniejsza ten efekt.
    velLong = forward * glm::dot(Velocity, forward);
    velLat = Velocity - velLong;
    velLat *= (1.0f - glm::clamp(effectiveGrip * deltaTime * 2.0f, 0.0f, 1.0f));
    Velocity = velLong + velLat;

    // 9. Sterowanie (Zmiana kierunku prędkości w stronę skręconych kół).
    // Działa tylko gdy samochód jedzie i nie ma zaciągniętego ręcznego.
    currentSpeed = glm::length(Velocity);
    if (currentSpeed > 0.5f && !Handbrake) {
        // Im szybciej jedziemy, tym słabiej skręcamy (symulacja utraty przyczepności/podsterowności przy V-max).
        float alignFactor = SteeringResponsiveness * deltaTime * glm::clamp(Grip * (1.0f - currentSpeed / (MaxSpeed + 0.1f)), 0.0f, 1.0f);
        glm::vec3 velDir = glm::normalize(Velocity);
        // Mieszamy wektor aktualnej prędkości z wektorem przodu auta.
        Velocity = glm::normalize(glm::mix(velDir, forward, alignFactor)) * currentSpeed * 0.99f;
    }

    // 10. *** TWARDE OGRANICZENIE PRĘDKOŚCI DO 18 KM/H (5 M/S) ***
    // Zabezpieczenie na wypadek, gdyby suma sił przekroczyła limit.
    if (glm::length(Velocity) > MaxSpeed) {
        Velocity = glm::normalize(Velocity) * MaxSpeed;
    }

    // 11. Całkowanie pozycji (Fizyka Eulera).
    Position += Velocity * deltaTime;
    Position.y = 0.0f; // Trzymamy auto na ziemi.
    HandleCollision();

    // Obracanie kół w zależności od przebytej drogi (efekt wizualny).
    WheelRotation += glm::dot(Velocity, forward) * deltaTime * 10.0f;
}

// Prosta obsługa kolizji: jeśli samochód wypadnie za daleko poza mapę (>500 jednostek),
// resetujemy jego prędkość i cofamy do poprzedniej klatki (choć tutaj resetuje się nie do końca poprawnie, bo velocity=0).
void RaceCar::HandleCollision() {
    if (glm::length(Position) > 500.0f) {
        Position = PreviousPosition;
        Velocity = glm::vec3(0);
    }
}

// Tworzy macierz transformacji modelu (skalowanie, rotacja, translacja) do wysłania do shadera.
glm::mat4 RaceCar::GetModelMatrix() const {
    glm::mat4 m = glm::translate(glm::mat4(1.0f), Position);
    m = glm::rotate(m, glm::radians(Yaw), glm::vec3(0, 1, 0));
    return glm::scale(m, glm::vec3(0.15f)); // Skalujemy model w dół (do 15% oryginału).
}

// Rysuje cały samochód (karoseria + 4 koła).
// Parametry pos i yaw pozwalają na narysowanie "kopii" w innym miejscu (np. w menu).
void RaceCar::Draw(const Shader& shader, glm::vec3 pos, float yaw) const {
    // Obliczamy macierz modelu dla karoserii. Jeśli podano niestandardową pozycję (>0.001f od 0), używamy jej.
    glm::mat4 m = (glm::length(pos) > 0.001f) ? glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), pos), glm::radians(yaw), glm::vec3(0, 1, 0)), glm::vec3(0.15f)) : GetModelMatrix();

    // Aktywujemy teksturę samochodu.
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, textureID);
    shader.setBool("useTexture", true); shader.setMat4("model", m);

    // Rysujemy karoserię.
    glBindVertexArray(bodyMesh.VAO); glDrawElements(GL_TRIANGLES, (GLsizei)bodyMesh.indices.size(), GL_UNSIGNED_INT, 0);

    // Definiujemy przesunięcia (offsety) dla 4 kół względem środka samochodu.
    glm::vec3 wOffs[] = { {WheelFrontX, 0.25f, WheelZ}, {-WheelFrontX, 0.25f, WheelZ}, {WheelBackX, 0.25f, -WheelZ}, {-WheelBackX, 0.25f, -WheelZ} };

    // Pętla rysująca każde z 4 kół.
    for (int i = 0; i < 4; i++) {
        const CarMesh& currentWheel = (i < 2) ? wheelFrontMesh : wheelBackMesh;
        if (currentWheel.VAO == 0) continue;

        // Transformacja koła: najpierw obrót wokół własnej osi (toczenie), potem przesunięcie na miejsce na osi.
        glm::mat4 wM = glm::rotate(glm::translate(m, wOffs[i]), glm::radians(WheelRotation), glm::vec3(1, 0, 0));

        // Tutaj można by dodać obrót przednich kół przy skręcaniu (brak w tym kodzie).

        shader.setMat4("model", wM);
        glBindVertexArray(currentWheel.VAO); glDrawElements(GL_TRIANGLES, (GLsizei)currentWheel.indices.size(), GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
}

// Parser plików OBJ (tylko wierzchołki, normalne, UV i trójkąty).
bool RaceCar::loadObj(const std::string& path, CarMesh& mesh, bool isBody) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    // Tymczasowe kontenery na dane z pliku.
    std::vector<glm::vec3> t_v; std::vector<glm::vec3> t_vn; std::vector<glm::vec2> t_vt;
    std::string line;
    bool skipPart = false; // Flaga do pomijania obiektów (np. kół w pliku body).

    while (std::getline(file, line)) {
        std::stringstream ss(line); std::string pr; ss >> pr;

        // Obsługa grup/obiektów ('o' lub 'g').
        if (pr == "o" || pr == "g") {
            std::string name; ss >> name; std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            // Jeśli ładujemy karoserię (isBody=true), pomijamy obiekty zawierające "wheel" w nazwie.
            skipPart = (isBody && (name.find("wheel") != std::string::npos));
        }

        // Parsowanie wierzchołków (v), normalnych (vn), UV (vt).
        if (pr == "v") { glm::vec3 v; ss >> v.x >> v.y >> v.z; t_v.push_back(v); }
        else if (pr == "vn") { glm::vec3 vn; ss >> vn.x >> vn.y >> vn.z; t_vn.push_back(vn); }
        else if (pr == "vt") { glm::vec2 vt; ss >> vt.x >> vt.y; t_vt.push_back(vt); }

        // Parsowanie ścian (f - faces).
        else if (pr == "f") {
            if (skipPart) continue; // Pomiń, jeśli to niechciana część.

            std::string v_s; std::vector<std::string> face;
            while (ss >> v_s) face.push_back(v_s); // Czytaj wszystkie wierzchołki ściany.
            if (face.size() < 3) continue; // Ignoruj linie/punkty.

            // Triangulacja (zamiana wielokątów na trójkąty: wachlarz 0, i, i+1).
            for (size_t i = 1; i < face.size() - 1; i++) {
                std::string tri[] = { face[0], face[i], face[i + 1] };
                for (auto& f_str : tri) {
                    std::stringstream fss(f_str); std::string seg; std::vector<int> ids;
                    // Parsowanie formatu v/vt/vn.
                    while (std::getline(fss, seg, '/')) {
                        try { ids.push_back(seg.empty() ? 0 : std::stoi(seg)); }
                        catch (...) { ids.push_back(0); }
                    }
                    if (!ids.empty()) {
                        int v_idx = ids[0];
                        // Obsługa ujemnych indeksów (liczonych od końca).
                        if (v_idx < 0) v_idx = (int)t_v.size() + v_idx + 1;

                        if (v_idx > 0 && v_idx <= (int)t_v.size()) {
                            // Dodaj wierzchołek do docelowej siatki.
                            mesh.vertices.push_back(t_v[v_idx - 1]);
                            // Dodaj UV (jeśli istnieje).
                            mesh.texCoords.push_back((ids.size() > 1 && ids[1] > 0 && ids[1] <= (int)t_vt.size()) ? t_vt[ids[1] - 1] : glm::vec2(0));
                            // Dodaj normalną (jeśli istnieje, w przeciwnym razie domyślna w górę).
                            mesh.normals.push_back((ids.size() > 2 && ids[2] > 0 && ids[2] <= (int)t_vn.size()) ? t_vn[ids[2] - 1] : glm::vec3(0, 1, 0));
                            // Dodaj nowy indeks sekwencyjny.
                            mesh.indices.push_back((unsigned int)mesh.indices.size());
                        }
                    }
                }
            }
        }
    }
    // Po wczytaniu danych skonfiguruj bufory OpenGL.
    mesh.setupMesh(); return true;
}

// Funkcja pomocnicza ładująca teksturę PNG/JPG przy użyciu biblioteki stb_image.
unsigned int RaceCar::loadTexture(const char* path) {
    unsigned int id; glGenTextures(1, &id); int w, h, nrComponents;
    stbi_set_flip_vertically_on_load(true); // Odwróć oś Y (OpenGL ma (0,0) na dole).
    unsigned char* data = stbi_load(path, &w, &h, &nrComponents, 0);
    if (data) {
        GLenum format = (nrComponents == 4) ? GL_RGBA : GL_RGB;
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
    }
    return id;
}