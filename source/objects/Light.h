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

  void displayGui(int id);

  [[nodiscard]] LightUniform getUniform() const;

private:
  glm::vec3 position;
  glm::vec3 color;
  float ambient;
  float diffuse;
  float specular;
};



#endif //LIGHT_H
