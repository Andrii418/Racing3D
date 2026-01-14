#include "MiniMap.h"
#include "TrackCollision.h"
#include <algorithm>
#include <cfloat>

namespace MiniMap {

    void DrawMiniMap(ImDrawList* draw, const ImVec2& topLeft, const ImVec2& size,
        const glm::vec3& playerPos, const glm::vec3& aiPos)
    {
        using namespace glm;

        const auto& walls = TrackCollision::GetWalls();
        if (walls.empty()) return;

        float minX = FLT_MAX, maxX = -FLT_MAX;
        float minZ = FLT_MAX, maxZ = -FLT_MAX;

        for (const auto& w : walls) {
            minX = std::min(minX, w.start.x);
            minX = std::min(minX, w.end.x);
            maxX = std::max(maxX, w.start.x);
            maxX = std::max(maxX, w.end.x);

            minZ = std::min(minZ, w.start.y);
            minZ = std::min(minZ, w.end.y);
            maxZ = std::max(maxZ, w.start.y);
            maxZ = std::max(maxZ, w.end.y);
        }

        // include player/ai
        minX = std::min(minX, playerPos.x);
        maxX = std::max(maxX, playerPos.x);
        minZ = std::min(minZ, playerPos.z);
        maxZ = std::max(maxZ, playerPos.z);

        minX = std::min(minX, aiPos.x);
        maxX = std::max(maxX, aiPos.x);
        minZ = std::min(minZ, aiPos.z);
        maxZ = std::max(maxZ, aiPos.z);

        float worldW = std::max(0.001f, maxX - minX);
        float worldH = std::max(0.001f, maxZ - minZ);

        float pad = 0.05f;
        float scaleX = (size.x * (1.0f - 2.0f * pad)) / worldW;
        float scaleY = (size.y * (1.0f - 2.0f * pad)) / worldH;
        float scale = std::min(scaleX, scaleY);

        float extraX = (size.x - worldW * scale) * 0.5f;
        float extraY = (size.y - worldH * scale) * 0.5f;

        auto WorldToScreen = [&](float wx, float wz) -> ImVec2 {
            // 1. Normalizujemy współrzędne świata do zakresu 0.0 - 1.0
            float normX = (wx - minX) / worldW;
            float normZ = (wz - minZ) / worldH;

            // 2. OBRÓT O 90 STOPNI W PRAWO:
            // Nowe X zależy od starego Z (odwróconego)
            // Nowe Z (pionowe) zależy od starego X
            float rotatedX = (1.0f - normZ);
            float rotatedZ = normX;

            // 3. Skalowanie do rozmiaru okna minimapy (z uwzględnieniem paddingu i centrowania)
            float nx = rotatedX * (worldH * scale) + extraX;
            float ny = rotatedZ * (worldW * scale) + extraY;

            // 4. Mapowanie na współrzędne ImGui
            // topLeft.x + nx to pozycja pozioma
            // topLeft.y + ny to pozycja pionowa (bez odwracania size.y, jeśli chcemy poziomy układ)
            return ImVec2(topLeft.x + nx, topLeft.y + ny);
            };



        // ⭐ OKRĄGŁE TŁO MINIMAPY ⭐
        ImVec2 center(topLeft.x + size.x * 0.5f,
            topLeft.y + size.y * 0.5f);

        float radius = std::min(size.x, size.y) * 0.5f;

        draw->AddCircleFilled(center, radius, IM_COL32(0, 0, 0, 180), 64);
        draw->AddCircle(center, radius, IM_COL32(255, 255, 255, 80), 64, 2.0f);

        // ⭐ CLIPPING DO OKRĘGU (technicznie kwadrat bounding) ⭐
        draw->PushClipRect(
            ImVec2(center.x - radius, center.y - radius),
            ImVec2(center.x + radius, center.y + radius),
            true
        );

        // ⭐ ŚCIANY TORU ⭐
        for (const auto& w : walls) {
            ImVec2 a = WorldToScreen(w.start.x, w.start.y);
            ImVec2 b = WorldToScreen(w.end.x, w.end.y);
            draw->AddLine(a, b, IM_COL32(180, 180, 180, 220), 2.0f);
        }

        // ⭐ GRACZ I AI ⭐
        ImVec2 pPos = WorldToScreen(playerPos.x, playerPos.z);
        ImVec2 aiP = WorldToScreen(aiPos.x, aiPos.z);

        float markerR = std::max(3.0f, std::min(size.x, size.y) * 0.03f);

        draw->AddCircleFilled(pPos, markerR, IM_COL32(255, 160, 40, 255), 16);
        draw->AddCircleFilled(aiP, markerR, IM_COL32(40, 220, 100, 220), 16);

        draw->PopClipRect();
    }

} // namespace MiniMap
