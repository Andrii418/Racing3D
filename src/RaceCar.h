#pragma once
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <vector>
#include <string>

// Deklaracja zapowiadająca klasy Shader, aby uniknąć problemów z cyklicznymi zależnościami.
class Shader;

// Struktura przechowująca dane geometryczne (siatkę) części samochodu.
struct CarMesh {
    std::vector<glm::vec3> vertices;  // Pozycje wierzchołków.
    std::vector<glm::vec3> normals;   // Wektory normalne (do oświetlenia).
    std::vector<glm::vec2> texCoords; // Współrzędne tekstur.
    std::vector<unsigned int> indices; // Indeksy wierzchołków (dla glDrawElements).
    unsigned int VAO = 0, VBO = 0, EBO = 0; // Uchwyty (ID) buforów OpenGL.

    // Konfiguruje bufory OpenGL (VAO, VBO, EBO) przesyłając dane z wektorów na kartę graficzną.
    void setupMesh();
};

// Klasa reprezentująca samochód wyścigowy w grze (dane fizyczne, logika ruchu, rendering).
class RaceCar {
public:
    // Podstawowe parametry transformacji w świecie 3D (pozycja, kierunek, prędkość, obrót).
    glm::vec3 Position;
    glm::vec3 PreviousPosition; // Służy do prostego cofania w przypadku kolizji (rysowanie po fakcie).
    glm::vec3 Velocity;         // Wektor prędkości (zawiera kierunek i szybkość).
    glm::vec3 FrontVector;      // Wektor jednostkowy wskazujący przód samochodu.
    float Yaw;                  // Kąt obrotu samochodu wokół osi Y (w stopniach).

    // --- STROJENIE FIZYKI (Cel: 18 km/h = 5.0 m/s) ---
    float MaxSpeed = 5.0f;           // Maksymalna prędkość w metrach na sekundę (bardzo niska, jak w poleceniu).
    float Acceleration = 8.0f;       // Moc silnika - jak szybko samochód nabiera prędkości.
    float Braking = 10.0f;           // Siła hamowania - jak szybko samochód się zatrzymuje.
    float TurnRate = 2.5f;           // Prędkość skręcania kół (wpływa na promień skrętu).
    float WheelRotation = 0.0f;      // Bieżący kąt obrotu kół wokół własnej osi (animacja toczenia).

    // --- Parametry Oporu i Przyczepności ---
    float Grip = 1.5f;               // Przyczepność opon (im wyższa, tym mniej poślizgu na zakrętach).
    float AerodynamicDrag = 0.02f;   // Opór powietrza (spowalnia przy wyższych prędkościach, tu niski).
    float RollingResistance = 0.5f;  // Opór toczenia (tarcie opon o asfalt, hamuje przy puszczeniu gazu).
    float SteeringResponsiveness = 2.0f; // Jak szybko samochód reaguje na zmianę wektora prędkości przy skręcie.

    // --- Hamulec Ręczny ---
    bool Handbrake = false;                 // Flaga, czy spacja jest wciśnięta.
    float HandbrakeGripReduction = 0.6f;    // O ile spada przyczepność przy zaciągniętym ręcznym (ułatwia drift).
    float HandbrakeDeceleration = 0.95f;    // Współczynnik spowolnienia na ręcznym (mniejszy niż 1.0 powoduje hamowanie).

    // --- Wejście Gracza ---
    float SteeringInput = 0.0f; // Wartość z klawiszy A/D (-1.0 do 1.0).
    float ThrottleInput = 0.0f; // Wartość z klawiszy W/S (-1.0 do 1.0, gdzie minus to cofanie/hamowanie).
    float Throttle = 0.0f;      // Rzeczywista wartość przepustnicy po wygładzeniu (dla płynniejszego startu).
    float ThrottleResponse = 5.0f; // Szybkość reakcji przepustnicy na wciśnięcie klawisza.

    // --- Ustawienia Wizualne Kół ---
    float WheelFrontX = 0.45f; // Pozycja przednich kół na osi X (szerokość rozstawu).
    float WheelBackX = 0.45f;  // Pozycja tylnych kół na osi X.
    float WheelZ = 0.42f;      // Odległość osi od środka samochodu (rozstaw osi).

    // Konstruktor: Inicjalizuje podstawowe wektory i pozycję startową.
    RaceCar(glm::vec3 startPosition = glm::vec3(0.0f, 0.2f, 0.0f));

    // Ładuje modele 3D (karoseria i koła) z plików .obj.
    // Zwraca false, jeśli wczytywanie się nie powiedzie.
    bool loadAssets(const std::string& bodyPath, const std::string& wheelFrontPath, const std::string& wheelBackPath = "");

    // Zwalnia zasoby OpenGL (usuwa bufory i tekstury).
    void cleanup();

    // Główna funkcja aktualizująca fizykę samochodu w każdej klatce gry.
    // Oblicza nową pozycję na podstawie wejścia, prędkości, oporu i kolizji.
    void Update(float deltaTime);

    // Rysuje samochód na ekranie przy użyciu podanego shadera.
    // Można nadpisać pozycję i obrót (np. do rysowania w menu).
    void Draw(const Shader& shader, glm::vec3 pos = glm::vec3(0.0f), float yaw = 0.0f) const;

    // Oblicza i zwraca macierz modelu (Model Matrix) potrzebną shaderom do transformacji wierzchołków.
    glm::mat4 GetModelMatrix() const;

private:
    // Siatki dla poszczególnych części samochodu.
    CarMesh bodyMesh;
    CarMesh wheelFrontMesh;
    CarMesh wheelBackMesh;

    // ID tekstury nakładanej na samochód (wspólna dla wszystkich części).
    unsigned int textureID = 0;

    // Prosta obsługa kolizji z granicami świata (resetuje pozycję przy wypadnięciu z mapy).
    void HandleCollision();

    // Funkcja pomocnicza parsująca pliki .obj i wypełniająca strukturę CarMesh.
    // Parametr isBody pozwala na filtrowanie części (np. pomijanie kół w pliku body).
    bool loadObj(const std::string& path, CarMesh& mesh, bool isBody);

    // Funkcja pomocnicza ładująca obrazek z dysku i tworząca teksturę OpenGL.
    unsigned int loadTexture(const char* path);
};