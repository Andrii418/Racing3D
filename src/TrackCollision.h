#pragma once
#include <vector>
#include <glm/glm.hpp>

/**
 * @file TrackCollision.h
 * @brief Deklaracje struktur i funkcji do kolizji samochodu z „ścianami” toru w płaszczyźnie XZ.
 */

 /**
  * @brief Punkt 2D wykorzystywany do przechowywania surowych danych toru.
  *
  * @note Zgodnie z danymi mapy: `x` odpowiada osi X, a `y` odpowiada osi Z (świata 3D).
  */
struct Point {
    /** @brief Współrzędna X (świat). */
    float x;

    /** @brief Współrzędna Z (świat), przechowywana jako `y`. */
    float y;
};

/**
 * @brief Odcinek ściany toru w 2D (płaszczyzna XZ).
 *
 * Współrzędne są przechowywane jako `glm::vec2`, gdzie:
 * - `.x` = X
 * - `.y` = Z
 */
struct WallSegment {
    /** @brief Początek odcinka ściany. */
    glm::vec2 start;

    /** @brief Koniec odcinka ściany. */
    glm::vec2 end;
};

/**
 * @brief Statyczny system kolizji toru: budowanie ścian z polilinii i test kolizji okrąg–odcinek.
 *
 * System przechowuje dwie surowe polilinie (lewa i prawa strona toru), a następnie buduje z nich
 * listę odcinków (`walls`) wykorzystywanych do testów kolizji.
 *
 * Kolizja jest liczona w 2D w płaszczyźnie XZ (pozycja auta: `carPos.x` i `carPos.z`).
 */
class TrackCollision {
public:
    /**
     * @brief Surowe punkty lewej strony toru.
     *
     * Dane pochodzą z mapy; `Point::x` = X, `Point::y` = Z.
     */
    static const std::vector<Point> leftSideRaw;

    /**
     * @brief Surowe punkty prawej strony toru.
     *
     * Dane pochodzą z mapy; `Point::x` = X, `Point::y` = Z.
     */
    static const std::vector<Point> rightSideRaw;

    /**
     * @brief Lista odcinków ścian zbudowanych na podstawie surowych polilinii.
     */
    static std::vector<WallSegment> walls;

    /**
     * @brief Inicjalizuje system kolizji i buduje `walls` na podstawie danych surowych.
     * @param minTrackWidth Minimalna szerokość toru używana do odfiltrowania fragmentów (np. pod „mostem”).
     */
    static void Init(float minTrackWidth = 2.0f);

    /**
     * @brief Sprawdza kolizję okręgu (samochodu) z odcinkami ścian.
     *
     * Okrąg jest definiowany przez środek `(carPos.x, carPos.z)` i promień `radius`.
     *
     * @param carPos Pozycja samochodu w 3D (używane są składowe X oraz Z).
     * @param radius Promień okręgu kolizyjnego.
     * @return `true` jeśli okrąg przecina dowolny odcinek ściany; inaczej `false`.
     */
    static bool CheckCollision(const glm::vec3& carPos, float radius);

    /**
     * @brief Wyznacza wektor wypchnięcia (minimalny) usuwający penetrację okręgu ze ścianą.
     *
     * Jeśli wystąpi kolizja, funkcja zwraca najmniejszy wektor, który po dodaniu do pozycji w 2D (XZ)
     * przestaje penetrować ścianę.
     *
     * @param carPos Pozycja samochodu w 3D (używane są składowe X oraz Z).
     * @param radius Promień okręgu kolizyjnego.
     * @param outPush Wektor wypchnięcia w 2D (XZ). Ustawiany tylko gdy wystąpi kolizja.
     * @return `true` jeśli wykryto kolizję i wyznaczono wypchnięcie; inaczej `false`.
     */
    static bool FindCollisionPush(const glm::vec3& carPos, float radius, glm::vec2& outPush);

    /**
     * @brief Zwraca referencję do listy ścian (np. do debugowania lub rysowania minimapy).
     * @return Referencja do wektora `walls`.
     */
    static const std::vector<WallSegment>& GetWalls();
};