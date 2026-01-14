#pragma once
#include "imgui.h"
#include <glm/glm.hpp>

namespace MiniMap {
    void DrawMiniMap(ImDrawList* draw, const ImVec2& topLeft, const ImVec2& size,
        const glm::vec3& playerPos, const glm::vec3& aiPos,
        const glm::vec3& startCenter, const glm::vec3& startDir);
}
