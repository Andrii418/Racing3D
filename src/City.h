#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <vector>
#include <string>
#include "Track.h"

class Shader;

class City : public Track {

public:
	glm::vec3 Position;
	glm::vec3 Scale;
	float Yaw;

	
		City(glm::vec3 startPosition = glm::vec3(1.0f, 2.0f, 1.0f));

	bool loadModel(const std::string& modelPath = "assets/city/desert city.obj");
	void Draw(const Shader& shader, glm::vec3 pos = glm::vec3(0.0f), float yaw = 0.0f) const;
	glm::mat4 GetModelMatrix() const;
	

private:
	void setupMesh();
	void calculateNormals();

	
		std::vector<glm::vec3> vertices;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> texCoords;
	std::vector<unsigned int> indices;

	unsigned int VAO, VBO, EBO;
	

};
