#include "Model.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <glad/glad.h>

Model::Model(const std::string& path) {
    loadModel(path);
    // Upewnij siê, ¿e setupMesh jest wywo³ywany tylko, gdy wczytano dane
    if (!vertices.empty()) {
        setupMesh();
    }
}

Model::~Model() {
    // Zabezpieczenie przed usuniêciem niezainicjowanych obiektów OpenGL
    if (VAO != 0) glDeleteVertexArrays(1, &VAO);
    if (VBO != 0) glDeleteBuffers(1, &VBO);
    if (EBO != 0) glDeleteBuffers(1, &EBO);
}

void Model::loadModel(const std::string& path) {
    Assimp::Importer importer;
    // DODANO aiProcess_GenNormals, aby upewniæ siê, ¿e model ZAWSZE bêdzie mia³ normalne.
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_GenNormals);

    if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        std::cerr << "ERROR::ASSIMP::Failed to load model: " << path << std::endl;
        return;
    }

    // Prosta obs³uga tylko pierwszej siatki
    if (scene->mNumMeshes == 0) {
        std::cerr << "ERROR::ASSIMP::Model has no meshes: " << path << std::endl;
        return;
    }

    aiMesh* mesh = scene->mMeshes[0];
    vertices.reserve(mesh->mNumVertices);

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        
        // 1. Pozycje (Zawsze istniej¹)
        vertex.Position = glm::vec3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        );
        
        // 2. Normale (Normal): Poprawka b³êdu! Sprawdzamy, czy wskaŸnik jest poprawny.
        // Jeœli dodaliœmy aiProcess_GenNormals, ten warunek powinien byæ ZAWSZE prawdziwy.
        if (mesh->mNormals) { 
            vertex.Normal = glm::vec3(
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            );
        } else {
            // Bezpieczna wartoœæ domyœlna
            vertex.Normal = glm::vec3(0.0f, 0.0f, 0.0f);
        }

        // 3. Wspó³rzêdne tekstur (TexCoords)
        if (mesh->mTextureCoords[0])
            vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        else
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);

        vertices.push_back(vertex);
    }

    indices.reserve(mesh->mNumFaces * 3);
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
}

void Model::setupMesh() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // U¿ywanie .data() jest preferowane dla uzyskania wskaŸnika
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Pozycje
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    // Normale
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    // TexCoords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

    glBindVertexArray(0);
}

void Model::Draw(Shader& shader) {
    if (VAO == 0) return; // Zapobieganie rysowaniu niezainicjowanych modeli
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}