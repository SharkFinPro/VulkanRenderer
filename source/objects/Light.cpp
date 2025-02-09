#include "Light.h"

Light::Light(const glm::vec3 position, const glm::vec3 color, const float ambient, const float diffuse,
             const float specular)
  : position(position), color(color), ambient(ambient), diffuse(diffuse), specular(specular)
{}

LightUniform Light::getUniform() const
{
  return {
    .position = position,
    .color = color,
    .ambient = ambient,
    .diffuse = diffuse,
    .specular = specular
  };
}

glm::vec3 Light::getPosition() const
{
  return position;
}

glm::vec3 Light::getColor() const
{
  return color;
}

float Light::getAmbient() const
{
  return ambient;
}

float Light::getDiffuse() const
{
  return diffuse;
}

float Light::getSpecular() const
{
  return specular;
}

void Light::setPosition(const glm::vec3 position)
{
  this->position = position;
}

void Light::setColor(const glm::vec3 color)
{
  this->color = color;
}

void Light::setAmbient(const float ambient)
{
  this->ambient = ambient;
}

void Light::setDiffuse(const float diffuse)
{
  this->diffuse = diffuse;
}

void Light::setSpecular(const float specular)
{
  this->specular = specular;
}
