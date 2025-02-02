#ifndef VULKANPROJECT_UNIFORMS_H
#define VULKANPROJECT_UNIFORMS_H

#include <glm/glm.hpp>

struct LightMetadataUniform {
  alignas(16) int numLights;
};

struct CameraUniform {
  alignas(16) glm::vec3 position;
};

struct EllipticalDotsUniform {
  float shininess;

  float sDiameter;
  float tDiameter;

  float blendFactor;
};

struct NoiseOptionsUniform {
  float amplitude;
  float frequency;
};

struct CurtainUniform {
  float amplitude;
  float period;
  float shininess;
};

#endif //VULKANPROJECT_UNIFORMS_H
