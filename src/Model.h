#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Shader.h"

#include <string>
#include <vector>

/**
 * @file Model.h
 * @brief Deklaracje struktur i klas do ładowania modeli 3D (Assimp) oraz renderowania siatek (OpenGL).
 */

 /**
  * @brief Struktura pojedynczego wierzchołka siatki.
  *
  * Zawiera pozycję, normalną oraz współrzędne UV.
  */
struct Vertex {
    /** @brief Pozycja w przestrzeni modelu. */
    glm::vec3 Position;

    /** @brief Normalna wierzchołka do obliczeń oświetlenia. */
    glm::vec3 Normal;

    /** @brief Współrzędne tekstury (UV). */
    glm::vec2 TexCoords;
};

/**
 * @brief Opis tekstury powiązanej z materiałem/siatką.
 */
struct Texture {
    /** @brief Identyfikator tekstury OpenGL. */
    unsigned int id = 0;

    /** @brief Typ tekstury (np. `texture_diffuse`). */
    std::string type;

    /** @brief Ścieżka (względna w materiale) do pliku tekstury. */
    std::string path;
};

/**
 * @brief Pojedyncza siatka (mesh) modelu: geometria + tekstury + obiekty OpenGL.
 *
 * `Mesh` przechowuje wierzchołki i indeksy oraz konfiguruje VAO/VBO/EBO.
 * Renderowanie odbywa się przez `Draw()`.
 */
class Mesh {
public:
    /** @brief Lista wierzchołków siatki. */
    std::vector<Vertex>       vertices;

    /** @brief Lista indeksów (EBO) tworzących trójkąty. */
    std::vector<unsigned int> indices;

    /** @brief Lista tekstur przypisanych do siatki. */
    std::vector<Texture>      textures;

    /**
     * @brief Tworzy siatkę i inicjalizuje bufory OpenGL.
     * @param vertices Wierzchołki siatki.
     * @param indices Indeksy siatki.
     * @param textures Tekstury przypisane do siatki.
     */
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);

    /**
     * @brief Renderuje siatkę, ustawiając tekstury i wywołując `glDrawElements`.
     * @param shader Shader używany do renderowania (ustawiane są uniformy samplerów).
     */
    void Draw(const Shader& shader);

private:
    /** @brief Bufory OpenGL siatki. */
    unsigned int VAO = 0, VBO = 0, EBO = 0;

    /**
     * @brief Tworzy VAO/VBO/EBO i konfiguruje atrybuty wierzchołka (pos/norm/uv).
     */
    void setupMesh();
};

/**
 * @brief Model 3D składający się z wielu siatek wczytanych przez Assimp.
 *
 * `Model` wczytuje scenę z pliku (np. OBJ), iteruje po węzłach i siatkach Assimp,
 * buduje obiekty `Mesh` oraz ładuje tekstury materiałów (z cache).
 */
class Model {
public:
    /**
     * @brief Tworzy model i wczytuje dane z pliku.
     * @param path Ścieżka do pliku modelu (np. OBJ).
     */
    Model(const std::string& path);

    /**
     * @brief Renderuje wszystkie siatki modelu.
     * @param shader Shader używany do renderowania.
     */
    void Draw(const Shader& shader);

private:
    /** @brief Lista siatek zbudowanych na podstawie sceny Assimp. */
    std::vector<Mesh> meshes;

    /** @brief Katalog bazowy modelu służący do wyszukiwania tekstur. */
    std::string directory;

    /** @brief Cache wczytanych tekstur (żeby nie ładować duplikatów). */
    std::vector<Texture> textures_loaded;

    /**
     * @brief Wczytuje model przez Assimp i rozpoczyna przetwarzanie drzewa węzłów.
     * @param path Ścieżka do pliku modelu.
     */
    void loadModel(std::string path);

    /**
     * @brief Rekurencyjnie przetwarza węzeł Assimp i jego dzieci, dodając siatki do `meshes`.
     * @param node Węzeł sceny Assimp.
     * @param scene Scena Assimp zawierająca siatki i materiały.
     */
    void processNode(aiNode* node, const aiScene* scene);

    /**
     * @brief Konwertuje pojedynczą siatkę Assimp (`aiMesh`) na `Mesh`.
     * @param mesh Siatka Assimp.
     * @param scene Scena Assimp (potrzebna m.in. do materiałów).
     * @return Zbudowana siatka `Mesh`.
     */
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);

    /**
     * @brief Ładuje tekstury materiału Assimp danego typu, z użyciem cache.
     * @param mat Materiał Assimp.
     * @param type Typ tekstury Assimp (np. `aiTextureType_DIFFUSE`).
     * @param typeName Nazwa typu tekstury (np. `texture_diffuse`).
     * @return Lista tekstur dla danego typu.
     */
    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};