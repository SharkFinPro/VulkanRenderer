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
  Light(glm::vec3 position, glm::vec3 color, float ambient, float diffuse, float specular);

  [[nodiscard]] LightUniform getUniform() const;

  [[nodiscard]] glm::vec3 getPosition() const;
  [[nodiscard]] glm::vec3 getColor() const;
  [[nodiscard]] float getAmbient() const;
  [[nodiscard]] float getDiffuse() const;
  [[nodiscard]] float getSpecular() const;

  void setPosition(glm::vec3 position);
  void setColor(glm::vec3 color);
  void setAmbient(float ambient);
  void setDiffuse(float diffuse);
  void setSpecular(float specular);

private:
  glm::vec3 position;
  glm::vec3 color;
  float ambient;
  float diffuse;
  float specular;
};



#endif //LIGHT_H
