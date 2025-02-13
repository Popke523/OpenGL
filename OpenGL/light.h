#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>
#include <learnopengl/shader.h>

struct dir_light
{
	glm::vec3 direction;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;

	void apply(Shader &shader)
	{
		shader.setVec3("dirLight.direction", direction);
		shader.setVec3("dirLight.ambient", ambient);
		shader.setVec3("dirLight.diffuse", diffuse);
		shader.setVec3("dirLight.specular", specular);
	}
};

struct point_light
{
	glm::vec3 position;

	float constant;
	float linear;
	float quadratic;

	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;

	void apply(Shader &shader, int i)
	{
		shader.setVec3("pointLights[" + std::to_string(i) + "].position", position);
		shader.setFloat("pointLights[" + std::to_string(i) + "].constant", constant);
		shader.setFloat("pointLights[" + std::to_string(i) + "].linear", linear);
		shader.setFloat("pointLights[" + std::to_string(i) + "].quadratic", quadratic);
		shader.setVec3("pointLights[" + std::to_string(i) + "].ambient", ambient);
		shader.setVec3("pointLights[" + std::to_string(i) + "].diffuse", diffuse);
		shader.setVec3("pointLights[" + std::to_string(i) + "].specular", specular);
	}
};

struct spotlight
{
	glm::vec3 position;
	glm::vec3 direction;
	float cutOff;
	float outerCutOff;

	float constant;
	float linear;
	float quadratic;

	glm::vec3 ambient;
	glm::vec3 diffuse;
	glm::vec3 specular;

	void apply(Shader &shader, int i)
	{
		shader.setVec3("spotLights[" + std::to_string(i) + "].position", position);
		shader.setVec3("spotLights[" + std::to_string(i) + "].direction", direction);
		shader.setFloat("spotLights[" + std::to_string(i) + "].cutOff", cutOff);
		shader.setFloat("spotLights[" + std::to_string(i) + "].outerCutOff", outerCutOff);
		shader.setFloat("spotLights[" + std::to_string(i) + "].constant", constant);
		shader.setFloat("spotLights[" + std::to_string(i) + "].linear", linear);
		shader.setFloat("spotLights[" + std::to_string(i) + "].quadratic", quadratic);
		shader.setVec3("spotLights[" + std::to_string(i) + "].ambient", ambient);
		shader.setVec3("spotLights[" + std::to_string(i) + "].diffuse", diffuse);
		shader.setVec3("spotLights[" + std::to_string(i) + "].specular", specular);
	}
};



#endif