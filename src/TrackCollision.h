#pragma once
#include <vector>
#include <glm/glm.hpp>

struct Point {
    float x, y;
};

struct WallSegment {
    glm::vec2 start;
    glm::vec2 end;
};

class TrackCollision {
public:
    // surowe poliliny (oryginalne punkty z mapy)
    static const std::vector<Point> leftSideRaw;
    static const std::vector<Point> rightSideRaw;

    // zbudowane œciany do testów kolizji
    static std::vector<WallSegment> walls;

    // inicjalizacja (wywo³a przetworzenie raw -> walls)
    static void Init(float minTrackWidth = 2.0f);

    // test kolizji okr¹g vs œciany
    static bool CheckCollision(const glm::vec3& carPos, float radius);

    // znajdŸ wektor wypchniêcia (jeœli kolizja) w p³aszczyŸnie XZ
    static bool FindCollisionPush(const glm::vec3& carPos, float radius, glm::vec2& outPush);

    // dostêp do œcian (do debug/draw)
    static const std::vector<WallSegment>& GetWalls();
};
