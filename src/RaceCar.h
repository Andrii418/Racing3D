#pragma once
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <vector>
#include <string>

/**
 * @file RaceCar.h
 * @brief Definicje struktur i klas związanych z samochodem wyścigowym oraz jego siatkami renderingu.
 */

 /** @brief Deklaracja zapowiadająca klasy `Shader` w celu uniknięcia zależności cyklicznych. */
class Shader;

/**
 * @brief Struktura przechowująca dane geometryczne (siatkę) pojedynczej części modelu samochodu.
 *
 * Struktura zawiera dane wierzchołków, normalnych, współrzędnych UV, indeksów oraz uchwyty buforów OpenGL
 * (VAO/VBO/EBO) niezbędne do renderowania metodą `glDrawElements`.
 */
struct CarMesh {
    /** @brief Pozycje wierzchołków w przestrzeni modelu. */
    std::vector<glm::vec3> vertices;

    /** @brief Wektory normalne wierzchołków, wykorzystywane do oświetlenia. */
    std::vector<glm::vec3> normals;

    /** @brief Współrzędne UV przypisane do wierzchołków. */
    std::vector<glm::vec2> texCoords;

    /** @brief Indeksy wierzchołków wykorzystywane przez `glDrawElements`. */
    std::vector<unsigned int> indices;

    /** @brief Uchwyty buforów OpenGL: VAO, VBO, EBO. */
    unsigned int VAO = 0, VBO = 0, EBO = 0;

    /**
     * @brief Konfiguruje bufory OpenGL dla siatki.
     *
     * Tworzy (lub odtwarza) VAO/VBO/EBO oraz przesyła dane z `vertices/normals/texCoords/indices`
     * do pamięci GPU w układzie interleaved.
     */
    void setupMesh();
};

/**
 * @brief Klasa reprezentująca samochód wyścigowy w grze (fizyka, logika ruchu, rendering).
 *
 * Klasa przechowuje parametry ruchu, wejście gracza, dane do renderingu i podstawową obsługę kolizji.
 */
class RaceCar {
public:
    /** @brief Aktualna pozycja samochodu w świecie. */
    glm::vec3 Position;

    /** @brief Pozycja z poprzedniej klatki (używana do prostego cofania przy kolizji). */
    glm::vec3 PreviousPosition;

    /** @brief Aktualny wektor prędkości (kierunek i wartość). */
    glm::vec3 Velocity;

    /** @brief Znormalizowany wektor wskazujący przód samochodu. */
    glm::vec3 FrontVector;

    /** @brief Kąt obrotu wokół osi Y (w stopniach). */
    float Yaw;

    /** @brief Maksymalna prędkość w m/s. */
    float MaxSpeed = 5.0f;

    /** @brief Współczynnik przyspieszenia (moc „silnika”). */
    float Acceleration = 8.0f;

    /** @brief Siła hamowania. */
    float Braking = 10.0f;

    /** @brief Prędkość skrętu (wpływa na zmianę `Yaw`). */
    float TurnRate = 2.5f;

    /** @brief Kąt obrotu kół (animacja toczenia). */
    float WheelRotation = 0.0f;

    /** @brief Przyczepność opon (wpływa na redukcję poślizgu bocznego). */
    float Grip = 1.5f;

    /** @brief Opór aerodynamiczny (zależny od kwadratu prędkości). */
    float AerodynamicDrag = 0.02f;

    /** @brief Opór toczenia (liniowy). */
    float RollingResistance = 0.5f;

    /** @brief Responsywność skrętu (jak szybko prędkość „wyrównuje się” do przodu auta). */
    float SteeringResponsiveness = 2.0f;

    /** @brief Czy hamulec ręczny jest aktywny. */
    bool Handbrake = false;

    /** @brief Redukcja przyczepności podczas hamulca ręcznego. */
    float HandbrakeGripReduction = 0.6f;

    /** @brief Współczynnik wytracania prędkości na ręcznym (<1.0 hamuje). */
    float HandbrakeDeceleration = 0.95f;

    /** @brief Wejście skrętu (np. A/D) w zakresie [-1, 1]. */
    float SteeringInput = 0.0f;

    /** @brief Wejście gazu/hamulca (np. W/S) w zakresie [-1, 1]. */
    float ThrottleInput = 0.0f;

    /** @brief Wygładzona wartość przepustnicy (dla płynności). */
    float Throttle = 0.0f;

    /** @brief Szybkość reakcji `Throttle` na `ThrottleInput`. */
    float ThrottleResponse = 5.0f;

    /** @brief Pozycja przednich kół na osi X. */
    float WheelFrontX = 0.45f;

    /** @brief Pozycja tylnych kół na osi X. */
    float WheelBackX = 0.45f;

    /** @brief Położenie osi kół względem środka na osi Z. */
    float WheelZ = 0.42f;

    /**
     * @brief Tworzy instancję samochodu, inicjalizując stan początkowy.
     * @param startPosition Pozycja początkowa w świecie.
     */
    RaceCar(glm::vec3 startPosition = glm::vec3(0.0f, 0.2f, 0.0f));

    /**
     * @brief Ładuje zasoby modelu samochodu (karoseria i koła) oraz teksturę.
     * @param bodyPath Ścieżka do pliku OBJ karoserii.
     * @param wheelFrontPath Ścieżka do pliku OBJ przednich kół.
     * @param wheelBackPath Ścieżka do pliku OBJ tylnych kół (opcjonalnie; jeśli puste, używa `wheelFrontPath`).
     * @return `true` jeśli wszystkie elementy wczytano poprawnie; w przeciwnym razie `false`.
     */
    bool loadAssets(const std::string& bodyPath, const std::string& wheelFrontPath, const std::string& wheelBackPath = "");

    /**
     * @brief Zwalnia zasoby OpenGL i czyści dane siatek.
     */
    void cleanup();

    /**
     * @brief Aktualizuje fizykę samochodu w czasie.
     * @param deltaTime Czas między klatkami w sekundach.
     */
    void Update(float deltaTime);

    /**
     * @brief Rysuje samochód (karoseria + koła).
     * @param shader Shader użyty do renderowania.
     * @param pos Opcjonalna pozycja nadpisująca `Position` (np. podgląd w menu).
     * @param yaw Opcjonalny obrót nadpisujący `Yaw` (w stopniach).
     */
    void Draw(const Shader& shader, glm::vec3 pos = glm::vec3(0.0f), float yaw = 0.0f) const;

    /**
     * @brief Zwraca macierz modelu (translacja + rotacja + skala).
     * @return Macierz modelu używana w shaderze.
     */
    glm::mat4 GetModelMatrix() const;

private:
    /** @brief Siatka karoserii. */
    CarMesh bodyMesh;

    /** @brief Siatka przednich kół. */
    CarMesh wheelFrontMesh;

    /** @brief Siatka tylnych kół. */
    CarMesh wheelBackMesh;

    /** @brief Identyfikator tekstury wspólnej dla wszystkich części. */
    unsigned int textureID = 0;

    /**
     * @brief Prosta obsługa kolizji z granicami świata.
     *
     * Aktualnie realizuje cofnięcie do `PreviousPosition` jeśli auto wyjedzie poza dozwolony obszar.
     */
    void HandleCollision();

    /**
     * @brief Wczytuje plik OBJ i wypełnia strukturę `CarMesh`.
     * @param path Ścieżka do pliku OBJ.
     * @param mesh Struktura docelowa na wynik.
     * @param isBody Jeśli `true`, parser pominie obiekty zawierające „wheel” w nazwie.
     * @return `true` jeśli wczytanie i konfiguracja buforów zakończyły się sukcesem; inaczej `false`.
     */
    bool loadObj(const std::string& path, CarMesh& mesh, bool isBody);

    /**
     * @brief Wczytuje teksturę z pliku i tworzy teksturę OpenGL.
     * @param path Ścieżka do pliku tekstury.
     * @return Identyfikator tekstury OpenGL.
     */
    unsigned int loadTexture(const char* path);
};