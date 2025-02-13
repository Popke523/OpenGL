#ifndef CAR_H
#define CAR_H

#include <glm/glm.hpp>
#include <cmath>

class Car
{
public:
	glm::vec3 position;
	float yaw;
	float spotlightPitch;

	Car(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f)) : position(position), yaw(0.0f), spotlightPitch(0.0f)
	{
	}

	void rotate(float angle)
	{
		yaw = fmodf((yaw + angle), glm::radians(360.0f));
	}
	void move(float distance)
	{
		position.x += distance * sin(yaw);
		position.z += distance * cos(yaw);
	}
	void rotateSpotlight(float angle)
	{
		spotlightPitch += angle;
		if (angle > glm::radians(89.0f)) angle = glm::radians(89.0f);
		if (angle < glm::radians(-89.0f)) angle = glm::radians(-89.0f);
	}
};

#endif