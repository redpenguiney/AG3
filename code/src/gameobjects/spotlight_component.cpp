#include "spotlight_component.hpp"

SpotLightComponent::SpotLightComponent()
{
	lightColor = { 1, 1, 1 };
	lightRange = 100;
	innerAngle = 40;
	outerAngle = 50;
}

SpotLightComponent::~SpotLightComponent()
{
}

//void SpotLightComponent::Destroy()
//{
//}
//
//void SpotLightComponent::Init()
//{
//	
//}

float SpotLightComponent::Range()
{
	return lightRange;
}

void SpotLightComponent::SetRange(float range)
{
	lightRange = range;
}

void SpotLightComponent::SetColor(glm::vec3 color)
{
	lightColor = color;
}

glm::vec3 SpotLightComponent::Color()
{
	return lightColor;
}

void SpotLightComponent::SetInnerAngle(float angle)
{
	innerAngle = angle;
}

void SpotLightComponent::SetOuterAngle(float angle)
{
	outerAngle = angle;
}

float SpotLightComponent::InnerAngle()
{
	return innerAngle;
}

float SpotLightComponent::OuterAngle()
{
	return outerAngle;
}