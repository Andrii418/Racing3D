// TrackCollision.cpp
#define GLM_ENABLE_EXPERIMENTAL
#include "TrackCollision.h"
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp> // glm::length2
#include <algorithm>
#include <limits>

// -----------------------------
// Surowe punkty (przykład: wklejone przez Ciebie dane)
// Uwaga: nazwy pól Point::x = X (map X), Point::y = Z (map Z)
// -----------------------------
const std::vector<Point> TrackCollision::leftSideRaw = {
    {-19.124f, 18.8322f},
    {-18.3621f, 18.8469f},
    {-17.6714f, 18.8488f},
    {-16.8747f, 18.8506f},
    {-16.2283f, 18.8567f},
    {-15.5062f, 18.858f},
    {-14.1185f, 18.8603f},
    {-12.893f, 18.8958f},
    {-12.0677f, 18.9086f},
    {-11.2704f, 18.9115f},
    {-10.6814f, 18.9118f},
    {-9.85236f, 18.9223f},
    {-9.63031f, 18.9251f},
    {-8.4968f, 18.9216f},
    {-7.26345f, 18.9217f},
    {-6.1978f, 18.8438f},
    {-5.73204f, 18.7423f},
    {-5.402f, 18.6305f},
    {-5.25867f, 18.5914f},
    {-5.0577f, 18.4617f},

    {-4.77516f, 18.1089f},
    {-4.68288f, 17.8622f},
    {-4.58455f, 17.5147f},
    {-4.54266f, 17.1048f},
    {-4.50888f, 16.4617f},
    {-4.48243f, 15.8646f},
    {-4.51048f, 15.3542f},
    {-4.54622f, 14.9129f},
    {-4.69389f, 14.0208f},
    {-4.77225f, 13.4978f},
    {-4.81595f, 13.2626f},
    {-4.92984f, 12.9069f},
    {-5.06403f, 12.6799f},
    {-5.27429f, 12.5007f},
    {-5.35112f, 12.4434f},
    {-5.4939f, 12.37f},
    {-5.68045f, 12.2954f},
    {-5.95135f, 12.2002f},
    {-6.34424f, 12.1094f},
    {-6.82521f, 12.0222f},
    {-7.55358f, 11.957f},
    {-8.13267f, 11.9284f},
    {-8.91564f, 11.9291f},
    {-9.3021f, 11.9204f},
    {-9.73761f, 11.9086f},
    {-10.9278f, 11.9042f},
    {-11.4305f, 11.8958f},
    {-11.9022f, 11.888f},
    {-12.702f, 11.8969f},
    {-13.191f, 11.9001f},
    {-13.4825f, 11.902f},
    {-13.7938f, 11.9156f},
    {-14.2568f, 12.1599f},
    {-14.3979f, 12.3703f},
    {-14.5319f, 12.7511f},
    {-14.6053f, 13.031f},
    {-14.7869f, 13.5014f},
    {-15.1243f, 14.1439f},
    {-15.3439f, 14.4343f},
    {-15.5466f, 14.7023f},
    {-15.8768f, 15.074f},
    {-16.1012f, 15.3113f},
    {-16.3495f, 15.5249f},
    {-16.795f, 15.9261f},
    {-17.1708f, 16.2141f},
    {-17.4748f, 16.4357f},
    {-17.8592f, 16.6934f},
    {-18.3888f, 17.0483f},
    {-18.6903f, 17.2504f},
    {-19.1994f, 17.5572f},
    {-19.6411f, 17.7864f},
    {-20.4637f, 18.1701f},
    {-21.0645f, 18.4191f},
    {-21.6013f, 18.6457f},
    {-22.029f, 18.8157f},
    {-23.2749f, 19.1941f},
    {-23.8423f, 19.3292f},
    {-24.3288f, 19.3911f},
    {-24.9889f, 19.4182f},
    {-25.2943f, 19.4251f},
    {-25.909f, 19.3745f},
    {-26.5054f, 19.2556f},
    {-26.8851f, 19.1396f},
    {-27.3622f, 18.9409f},
    {-27.6447f, 18.7164f},
    {-27.8374f, 18.5475f},
    {-28.1196f, 18.1423f},
    {-28.3207f, 17.7729f},
    {-28.4852f, 17.1507f},
    {-28.507f, 16.772f},
    {-28.4287f, 16.1707f},
    {-28.2442f, 15.6542f},
    {-27.9741f, 15.2241f},
    {-27.6777f, 14.9493f},
    {-27.5921f, 14.8776f},
    {-27.0558f, 14.6008f},
    {-26.8131f, 14.5164f},
    {-26.3298f, 14.4288f},
    {-25.9726f, 14.4115f},
    {-25.7082f, 14.4027f},
    {-25.4815f, 14.4033f},
    {-24.8631f, 14.4552f},
    {-24.3586f, 14.5271f},
    {-23.8636f, 14.5837f},
    {-23.3885f, 14.615f},
    {-23.0965f, 14.6164f},
    {-22.8236f, 14.6046f},
    {-22.4854f, 14.5371f},
    {-22.2302f, 14.4626f},
    {-21.8112f, 14.2362f},
    {-21.4991f, 13.9665f},
    {-21.3626f, 13.7881f},
    {-21.1947f, 13.4951f},
    {-21.0055f, 13.0737f},
    {-20.9304f, 12.8086f},
    {-20.892f, 12.5106f},
    {-20.9026f, 12.2051f},
    {-21.0489f, 11.8393f},
    {-21.3954f, 11.5123f},
    {-21.5975f, 11.4632f},
    {-21.9012f, 11.4293f},
    {-22.4123f, 11.4359f},
    {-22.8403f, 11.4617f},
    {-23.2021f, 11.5264f},
    {-23.5989f, 11.6106f},
    {-24.1108f, 11.7336f},
    {-24.5699f, 11.8615f},
    {-25.2532f, 12.0575f},
    {-25.6764f, 12.1896f},
    {-25.9823f, 12.2863f},
    {-26.4664f, 12.4543f},
    {-27.1173f, 12.7034f},
    {-27.6896f, 12.9433f},
    {-28.2835f, 13.2663f},
    {-28.6902f, 13.4864f},
    {-29.083f, 13.7267f},
    {-29.2723f, 13.8481f},
    {-29.7213f, 14.2125f},
    {-30.0157f, 14.538f},
    {-30.345f, 15.1541f},
    {-30.4951f, 15.6302f},
    {-30.5404f, 16.495f},
    {-30.5395f, 16.3574f},
    {-30.4762f, 16.6626f},
    {-30.4034f, 17.0136f},
    {-30.3102f, 17.3273f},
    {-30.1644f, 17.7889f},
    {-30.0627f, 18.096f},
    {-29.7879f, 18.6433f},
    {-29.6163f, 18.9502f},
    {-29.1521f, 19.5569f},
    {-28.7327f, 19.9858f},
    {-28.6744f, 20.0404f},
    {-28.3386f, 20.2991f},
    {-27.9476f, 20.5428f},
    {-27.6145f, 20.6841f},
    {-27.0821f, 20.8307f},
    {-26.5616f, 20.9406f},
    {-26.15f, 21.0091f},
    {-25.6162f, 21.0194f},
    {-25.0396f, 20.9904f},
    {-24.5336f, 20.8864f},
    {-24.1584f, 20.7799f},
    {-23.7088f, 20.6305f},
    {-22.1137f, 19.8101f},
    {-21.3104f, 19.4231f},
    {-20.7568f, 19.1936f},
    {-20.3852f, 19.0498f},
    {-20.0332f, 18.9383f},
    {-19.6369f, 18.8733f},
    {-19.3346f, 18.8403f},
    {-19.0613f, 18.8139f},
    {-18.7617f, 18.8101f},
    {-18.6596f, 18.821f}
};

// Prawa strona drogi (mogę dodać w kolejnym kroku)
const std::vector<Point> TrackCollision::rightSideRaw = {
{-19.563, 20.7479},
{ -19.2772, 20.7266 },
{ -19.0216, 20.7007 },
{ -18.5008, 20.6983 },
{ -17.8802, 20.7051 },
{ -17.2214, 20.7049 },
{ -16.7152, 20.7048 },
{ -16.1244, 20.7227 },
{ -15.3718, 20.7291 },
{ -14.8953, 20.7291 },
{ -14.1196, 20.7505 },
{ -13.4908, 20.7594 },
{ -13.075, 20.759 },
{ -12.7121, 20.7586 },
{ -12.2469, 20.7581 },
{ -11.595, 20.7709 },
{ -11.0347, 20.7923 },
{ -10.4963, 20.7764 },
{ -10.1131, 20.8033 },
{ -9.3348, 20.7846 },
{ -8.79905, 20.7855 },
{ -8.31909, 20.7864 },
{ -7.7051, 20.7876 },
{ -6.95864, 20.7691 },
{ -6.62954, 20.7509 },
{ -5.64774, 20.5923 },
{ -5.23162, 20.5199 },
{ -4.98464, 20.4339 },
{ -4.70835, 20.3453 },
{ -4.46931, 20.2389 },
{ -4.25381, 20.1431 },
{ -4.06837, 20.029 },
{ -3.85239, 19.8597 },
{ -3.64104, 19.6877 },
{ -3.56586, 19.6088 },
{ -3.45311, 19.4568 },
{ -3.35313, 19.3221 },
{ -3.20793, 19.0924 },
{ -3.0409, 18.7752 },
{ -2.9166, 18.4976 },
{ -2.76232, 17.9864 },
{ -2.66695, 17.3816 },
{ -2.6283, 16.7282 },
{ -2.64615, 15.9371 },
{ -2.71154, 14.8986 },
{ -2.84509, 13.713 },
{ -2.92453, 13.256 },
{ -2.96957, 12.9969 },
{ -3.0341, 12.7821 },
{ -3.10352, 12.5218 },
{ -3.32262, 12.0189 },
{ -3.43282, 11.8334 },
{ -3.64544, 11.5554 },
{ -4.20215, 11.0435 },
{ -4.62532, 10.7589 },
{ -5.36218, 10.457 },
{ -5.4288, 10.4108 },
{ -5.85565, 10.3023 },
{ -6.41962, 10.1832 },
{ -6.80617, 10.1579 },
{ -7.18334, 10.1048 },
{ -7.67473, 10.0959 },
{ -8.65258, 10.0776 },
{ -9.6524, 10.0647 },
{ -10.2374, 10.0687 },
{ -11.0211, 10.0498 },
{ -11.7909, 10.044 },
{ -12.4136, 10.0604 },
{ -13.1213, 10.0586 },
{ -13.6217, 10.0624 },
{ -13.9874, 10.1001 },
{ -14.5264, 10.18 },
{ -14.8839, 10.335 },
{ -15.2486, 10.5465 },
{ -15.4396, 10.6924 },
{ -15.8932, 11.2169 },
{ -15.8044, 11.0791 },
{ -16.0235, 11.4162 },
{ -16.0585, 11.5292 },
{ -16.2582, 11.9785 },
{ -16.3366, 12.3208 },
{ -16.4962, 12.6741 },
{ -16.8361, 13.3111 },
{ -17.1268, 13.6648 },
{ -17.4153, 13.986 },
{ -17.9392, 14.4611 },
{ -18.1853, 14.6508 },
{ -18.5292, 14.9159 },
{ -18.9823, 15.2393 },
{ -19.4707, 15.5581 },
{ -19.7623, 15.7483 },
{ -19.9737, 15.8863 },
{ -20.6176, 16.2079 },
{ -20.9729, 16.3653 },
{ -21.3622, 16.541 },
{ -22.0961, 16.8378 },
{ -22.3575, 16.9408 },
{ -22.6852, 17.0681 },
{ -23.3233, 17.2712 },
{ -24.2617, 17.4677 },
{ -24.9034, 17.5542 },
{ -25.6521, 17.5651 },
{ -26.2893, 17.4069 },
{ -26.5852, 17.1372 },
{ -26.6176, 16.9313 },
{ -26.6401, 16.697 },
{ -26.4896, 16.3546 },
{ -26.188, 16.2781 },
{ -25.9817, 16.256 },
{ -25.7086, 16.2398 },
{ -25.4778, 16.2668 },
{ -25.0148, 16.3078 },
{ -25.0053, 16.3091 },
{ -24.4944, 16.3813 },
{ -24.1747, 16.4264 },
{ -23.8435, 16.4507 },
{ -23.4299, 16.482 },
{ -22.9996, 16.4638 },
{ -22.5539, 16.4252 },
{ -22.2442, 16.353 },
{ -21.8892, 16.2767 },
{ -21.3995, 16.0899 },
{ -21.03, 15.9323 },
{ -20.5591, 15.6164 },
{ -20.267, 15.3391 },
{ -19.7947, 14.7851 },
{ -19.6097, 14.4595 },
{ -19.4778, 14.1693 },
{ -19.368, 13.9267 },
{ -19.2182, 13.5957 },
{ -19.1068, 13.1937 },
{ -19.0532, 12.6847 },
{ -19.0472, 12.4408 },
{ -19.0834, 11.9565 },
{ -19.1234, 11.633 },
{ -19.3004, 11.1728 },
{ -19.6051, 10.6963 },
{ -19.9863, 10.1921 },
{ -20.4111, 9.92814 },
{ -20.7106, 9.77946 },
{ -21.1154, 9.65315 },
{ -21.5703, 9.57718 },
{ -22.0754, 9.54411 },
{ -22.6274, 9.59496 },
{ -23.2368, 9.67248 },
{ -23.5621, 9.70908 },
{ -24.0979, 9.83229 },
{ -24.5347, 9.92592 },
{ -24.9881, 10.0231 },
{ -25.3744, 10.1469 },
{ -25.9343, 10.3206 },
{ -26.3687, 10.4802 },
{ -27.0976, 10.7436 },
{ -27.3857, 10.8477 },
{ -27.6619, 10.9475 },
{ -27.9602, 11.0553 },
{ -28.3686, 11.2468 },
{ -28.9367, 11.5256 },
{ -29.5288, 11.8548 },
{ -29.9706, 12.1212 },
{ -30.3368, 12.3839 },
{ -30.6953, 12.6778 },
{ -31.1968, 13.1448 },
{ -31.4442, 13.4324 },
{ -31.7365, 13.8139 },
{ -31.9514, 14.2228 },
{ -32.1451, 14.631 },
{ -32.3478, 15.3621 },
{ -32.4488, 15.9042 },
{ -32.427, 16.3068 },
{ -32.3803, 16.6192 },
{ -32.2773, 17.2676 },
{ -32.0689, 17.966 },
{ -31.5539, 19.2152 },
{ -31.3037, 19.7416 },
{ -30.9415, 20.2781 },
{ -30.6748, 20.6483 },
{ -30.3747, 20.986 },
{ -29.7589, 21.5274 },
{ -29.1988, 21.9715 },
{ -28.7716, 22.1902 },
{ -28.3503, 22.3762 },
{ -27.9872, 22.5194 },
{ -27.4556, 22.6598 },
{ -26.7801, 22.7897 },
{ -26.8391, 22.7903 },
{ -26.4692, 22.8263 },
{ -25.9558, 22.8687 },
{ -25.6035, 22.8715 },
{ -25.2702, 22.8742 },
{ -25.0177, 22.8472 },
{ -24.6023, 22.8106 },
{ -24.0232, 22.701 },
{ -23.6134, 22.5849 },
{ -23.3187, 22.4853 },
{ -22.9459, 22.356 },
{ -22.5371, 22.1571 },
{ -22.1111, 21.95 },
{ -21.691, 21.7139 },
{ -21.5459, 21.6379 },
{ -21.3783, 21.5499 },
{ -21.073, 21.3898 },
{ -20.7732, 21.2325 },
{ -20.576, 21.1609 },
{ -20.1873, 20.9954 },
{ -19.7469, 20.8141 },
{ -19.519, 20.7319 },
{ -19.29, 20.6897 },
{ -18.9545, 20.6843 },
{ -18.5497, 20.6779 },
{ -18.223, 20.7076 },


};
// wynikowe ściany
std::vector<WallSegment> TrackCollision::walls;

// -----------------------------
// pomocnicze funkcje przetwarzania polilinii
// -----------------------------
static void RemoveClosePoints(std::vector<glm::vec2>& pts, float minDist = 1e-4f) {
    if (pts.empty()) return;
    std::vector<glm::vec2> out;
    out.reserve(pts.size());
    out.push_back(pts[0]);
    for (size_t i = 1; i < pts.size(); ++i) {
        if (glm::distance2(pts[i], out.back()) > minDist * minDist)
            out.push_back(pts[i]);
    }
    pts.swap(out);
}

static float PerpDist(const glm::vec2& a, const glm::vec2& b, const glm::vec2& p) {
    glm::vec2 ab = b - a;
    float ab2 = glm::dot(ab, ab);
    if (ab2 == 0.0f) return glm::length(p - a);
    float t = glm::dot(p - a, ab) / ab2;
    t = glm::clamp(t, 0.0f, 1.0f);
    glm::vec2 proj = a + t * ab;
    return glm::length(p - proj);
}

static void RDP(const std::vector<glm::vec2>& pts, float eps, std::vector<glm::vec2>& out) {
    if (pts.size() < 2) { out = pts; return; }
    float maxd = 0.0f;
    size_t idx = 0;
    for (size_t i = 1; i + 1 < pts.size(); ++i) {
        float d = PerpDist(pts.front(), pts.back(), pts[i]);
        if (d > maxd) { maxd = d; idx = i; }
    }
    if (maxd > eps) {
        std::vector<glm::vec2> left, right;
        std::vector<glm::vec2> a(pts.begin(), pts.begin() + idx + 1);
        std::vector<glm::vec2> b(pts.begin() + idx, pts.end());
        RDP(a, eps, left);
        RDP(b, eps, right);
        out = left;
        out.insert(out.end(), right.begin() + 1, right.end());
    } else {
        out.clear();
        out.push_back(pts.front());
        out.push_back(pts.back());
    }
}

static void SmoothPolyline(std::vector<glm::vec2>& pts, int radius = 1) {
    if (pts.size() < 3 || radius <= 0) return;
    std::vector<glm::vec2> tmp = pts;
    for (size_t i = 0; i < pts.size(); ++i) {
        glm::vec2 sum(0.0f);
        int cnt = 0;
        for (int k = -radius; k <= radius; ++k) {
            int j = (int)i + k;
            if (j < 0) j = 0;
            if (j >= (int)pts.size()) j = (int)pts.size() - 1;
            sum += tmp[j];
            ++cnt;
        }
        pts[i] = sum / (float)cnt;
    }
}

// Buduje ściany z dwóch stron, ale pomija fragmenty gdzie odległość między stronami jest zbyt mała (np. pod mostem)
static void BuildWallsFromSides(const std::vector<Point>& leftRaw, const std::vector<Point>& rightRaw, std::vector<WallSegment>& outWalls, float minTrackWidth) {
    outWalls.clear();

    // konwersja do glm::vec2
    std::vector<glm::vec2> left, right;
    left.reserve(leftRaw.size());
    right.reserve(rightRaw.size());
    for (const auto& p : leftRaw) left.emplace_back(p.x, p.y);
    for (const auto& p : rightRaw) right.emplace_back(p.x, p.y);

    // usuń bardzo bliskie punkty
    RemoveClosePoints(left, 1e-5f);
    RemoveClosePoints(right, 1e-5f);

    // wygładź (opcjonalne)
    SmoothPolyline(left, 1);
    SmoothPolyline(right, 1);



    // uprość poliliny RDP (dostosuj eps jeśli trzeba)
    std::vector<glm::vec2> leftS, rightS;
    RDP(left, 0.02f, leftS);
    RDP(right, 0.02f, rightS);

    // funkcja licząca minimalną odległość środka segmentu do przeciwnej poliliny
    auto avgDistToOther = [&](const glm::vec2& a, const glm::vec2& b, const std::vector<glm::vec2>& other) {
        glm::vec2 mid = (a + b) * 0.5f;
        float minD = std::numeric_limits<float>::max();
        for (const auto& p : other) {
            float d = glm::distance(mid, p);
            if (d < minD) minD = d;
        }
        return minD;
    };

    const float MIN_TRACK_WIDTH = minTrackWidth;   // np. 2.0f
    const float MAX_TRACK_WIDTH = 100.0f;

    // dodajemy segmenty lewej tylko jeśli są "na zewnątrz" jezdni
    for (size_t i = 0; i + 1 < leftS.size(); ++i) {
        glm::vec2 a = leftS[i];
        glm::vec2 b = leftS[i + 1];
        float d = avgDistToOther(a, b, rightS);
        // jeśli odległość do przeciwnej strony jest wystarczająca, traktujemy segment jako bandę
        if (d > (MIN_TRACK_WIDTH * 0.5f) && d < MAX_TRACK_WIDTH) {
            outWalls.push_back({ a, b });
        }
    }

    // analogicznie dla prawej
    for (size_t i = 0; i + 1 < rightS.size(); ++i) {
        glm::vec2 a = rightS[i];
        glm::vec2 b = rightS[i + 1];
        float d = avgDistToOther(a, b, leftS);
        if (d > (MIN_TRACK_WIDTH * 0.5f) && d < MAX_TRACK_WIDTH) {
            outWalls.push_back({ a, b });
        }
    }

    // opcjonalnie: posortuj/usuń krótkie segmenty (stabilność)
    std::vector<WallSegment> filtered;
    filtered.reserve(outWalls.size());
    for (auto &s : outWalls) {
        if (glm::distance(s.start, s.end) > 1e-4f) filtered.push_back(s);
    }
    outWalls.swap(filtered);
}

// -----------------------------
// public API
// -----------------------------
void TrackCollision::Init(float minTrackWidth) {
    walls.clear();
    BuildWallsFromSides(leftSideRaw, rightSideRaw, walls, minTrackWidth);
}

// Sprawdzenie kolizji: okrąg (carPos.x, carPos.z, radius) vs wszystkie segmenty
bool TrackCollision::CheckCollision(const glm::vec3& carPos, float radius) {
    glm::vec2 p(carPos.x, carPos.z);
    float r2 = radius * radius;

    for (const auto& wall : walls) {
        glm::vec2 a = wall.start;
        glm::vec2 b = wall.end;
        glm::vec2 ab = b - a;
        float abLen2 = glm::dot(ab, ab);
        if (abLen2 <= 1e-8f) continue;

        float t = glm::clamp(glm::dot(p - a, ab) / abLen2, 0.0f, 1.0f);
        glm::vec2 closest = a + t * ab;
        float dist2 = glm::length2(p - closest);
        if (dist2 < r2) return true;
    }
    return false;
}

// Znajdź wektor wypchnięcia (minimalny wektor, który usuwa penetrację).
bool TrackCollision::FindCollisionPush(const glm::vec3& carPos, float radius, glm::vec2& outPush) {
    glm::vec2 p(carPos.x, carPos.z);
    float r2 = radius * radius;

    bool collided = false;
    float minPenetration = std::numeric_limits<float>::max();
    glm::vec2 bestPush(0.0f);

    for (const auto& wall : walls) {
        glm::vec2 a = wall.start;
        glm::vec2 b = wall.end;
        glm::vec2 ab = b - a;
        float abLen2 = glm::dot(ab, ab);
        if (abLen2 <= 1e-8f) continue;

        float t = glm::clamp(glm::dot(p - a, ab) / abLen2, 0.0f, 1.0f);
        glm::vec2 closest = a + t * ab;
        glm::vec2 diff = p - closest;
        float dist2 = glm::length2(diff);

        if (dist2 < r2) {
            float dist = sqrt(dist2);
            float penetration = radius - dist;
            glm::vec2 normal;
            if (dist > 1e-6f) normal = diff / dist;
            else {
                glm::vec2 dir = ab;
                float len = glm::length(dir);
                if (len < 1e-6f) continue;
                dir /= len;
                normal = glm::vec2(-dir.y, dir.x);
            }
            glm::vec2 push = normal * penetration;
            if (penetration < minPenetration) {
                minPenetration = penetration;
                bestPush = push;
            }
            collided = true;
        }
    }

    if (collided) {
        outPush = bestPush;
        return true;
    }
    outPush = glm::vec2(0.0f);
    return false;
}

const std::vector<WallSegment>& TrackCollision::GetWalls() {
    return walls;
}
