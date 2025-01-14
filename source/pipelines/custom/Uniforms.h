#ifndef VULKANPROJECT_UNIFORMS_H
#define VULKANPROJECT_UNIFORMS_H

#include <glm/glm.hpp>

struct LightMetadataUniform {
  alignas(16) int numLights;
};

struct CameraUniform {
  alignas(16) glm::vec3 position;
};

#endif //VULKANPROJECT_UNIFORMS_H
