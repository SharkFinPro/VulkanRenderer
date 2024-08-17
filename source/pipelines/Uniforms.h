#ifndef VULKANPROJECT_UNIFORMS_H
#define VULKANPROJECT_UNIFORMS_H

#include <glm/glm.hpp>

struct TransformUniform {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

struct LightUniform {
  alignas(16) glm::vec3 position;
  alignas(16) glm::vec3 color;

  alignas(4) float ambient;
  alignas(4) float diffuse;
  alignas(4) float specular;
};

struct CameraUniform {
  alignas(16) glm::vec3 position;
};

#endif //VULKANPROJECT_UNIFORMS_H
