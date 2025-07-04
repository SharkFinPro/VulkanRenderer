#ifndef LIGHT_H
#define LIGHT_H

#include <glm/vec3.hpp>

struct alignas(16) LightUniform {
  glm::vec3 position;
  float padding1;
  glm::vec3 color;
  float padding2;

  float ambient;
  float diffuse;
  float specular;
  float padding3;
};

class Light {
public:
  Light(const glm::vec3& position, const glm::vec3& color, float ambient, float diffuse, float specular);

  [[nodiscard]] LightUniform getUniform() const;

  [[nodiscard]] glm::vec3 getPosition() const;
  [[nodiscard]] glm::vec3 getColor() const;
  [[nodiscard]] float getAmbient() const;
  [[nodiscard]] float getDiffuse() const;
  [[nodiscard]] float getSpecular() const;

  void setPosition(const glm::vec3& position);
  void setColor(const glm::vec3& color);
  void setAmbient(float ambient);
  void setDiffuse(float diffuse);
  void setSpecular(float specular);

private:
  glm::vec3 m_position;
  glm::vec3 m_color;
  float m_ambient;
  float m_diffuse;
  float m_specular;
};



#endif //LIGHT_H
