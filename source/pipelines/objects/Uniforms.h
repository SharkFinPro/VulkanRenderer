#ifndef VULKANPROJECT_UNIFORMS_H
#define VULKANPROJECT_UNIFORMS_H

#include <glm/glm.hpp>

struct TransformUniform {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

struct alignas(16) Light {
  glm::vec3 position;  // 12 bytes
  float padding1;      // 4 bytes to align the next member to a 16-byte boundary
  glm::vec3 color;     // 12 bytes
  float padding2;      // 4 bytes, to make the struct size a multiple of 16 bytes

  float ambient;       // 4 bytes
  float diffuse;       // 4 bytes
  float specular;      // 4 bytes
  float padding3;      // 4 bytes, to make the struct size a multiple of 16 bytes
};

struct LightUniform {
  alignas(16) int numLights;
  Light lights[3];  // Fixed-size array, aligned to 16 bytes
};

struct CameraUniform {
  alignas(16) glm::vec3 position;
};

#endif //VULKANPROJECT_UNIFORMS_H
