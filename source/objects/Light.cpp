#include "Light.h"

Light::Light(const glm::vec3& position,
             const glm::vec3& color,
             const float ambient,
             const float diffuse,
             const float specular)
  : m_position(position), m_color(color), m_ambient(ambient), m_diffuse(diffuse), m_specular(specular)
{}

LightUniform Light::getUniform() const
{
  return {
    .position = m_position,
    .color = m_color,
    .ambient = m_ambient,
    .diffuse = m_diffuse,
    .specular = m_specular
  };
}

glm::vec3 Light::getPosition() const
{
  return m_position;
}

glm::vec3 Light::getColor() const
{
  return m_color;
}

float Light::getAmbient() const
{
  return m_ambient;
}

float Light::getDiffuse() const
{
  return m_diffuse;
}

float Light::getSpecular() const
{
  return m_specular;
}

void Light::setPosition(const glm::vec3& position)
{
  m_position = position;
}

void Light::setColor(const glm::vec3& color)
{
  m_color = color;
}

void Light::setAmbient(const float ambient)
{
  m_ambient = ambient;
}

void Light::setDiffuse(const float diffuse)
{
  m_diffuse = diffuse;
}

void Light::setSpecular(const float specular)
{
  m_specular = specular;
}
