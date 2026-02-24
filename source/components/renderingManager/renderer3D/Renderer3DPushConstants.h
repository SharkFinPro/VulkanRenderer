#ifndef VULKANPROJECT_RENDERER3DPUSHCONSTANTS_H
#define VULKANPROJECT_RENDERER3DPUSHCONSTANTS_H

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace vke {

  struct BumpyCurtainPushConstant {
    float amplitude = 0.1f;
    float period = 1.0f;
    float shininess = 10.0f;
    float noiseAmplitude = 0.5f;
    float noiseFrequency = 1.0f;
  };

  struct CrossesPushConstant {
    glm::vec3 position = glm::vec3(0.0f);
    float quantize = 50.0f;
    float size = 0.01f;
    float shininess = 10.0f;
    float blueDepth = 4.4f;
    float redDepth = 1.0f;
    int level = 1;
    uint32_t useChromaDepth = false;
  };

  struct CubeMapPushConstant {
    glm::vec3 position = glm::vec3(0.0f);
    float mix = 0.0f;
    float refractionIndex = 1.4f;
    float whiteMix = 0.2f;
    float noiseAmplitude = 0.0f;
    float noiseFrequency = 0.1f;
  };

  struct CurtainPushConstant {
    float amplitude = 0.1f;
    float period = 1.0f;
    float shininess = 10.0f;
  };

  struct EllipticalDotsPushConstant {
    float shininess = 10.0f;
    float sDiameter = 0.025f;
    float tDiameter = 0.025f;
    float blendFactor = 0.0f;
  };

  struct GridPushConstant {
    glm::mat4 viewProj;
    glm::vec3 viewPosition;
  };

  struct MagnifyWhirlMosaicPushConstant {
    float lensS = 0.5f;
    float lensT = 0.5f;
    float lensRadius = 0.25f;
    float magnification = 1.0f;
    float whirl = 0.0f;
    float mosaic = 0.001f;
  };

  struct NoisyEllipticalDotsPushConstant {
    float shininess = 10.0f;
    float sDiameter = 0.025f;
    float tDiameter = 0.025f;
    float blendFactor = 0.0f;
    float noiseAmplitude = 0.5f;
    float noiseFrequency = 1.0f;
  };

  struct SnakePushConstant {
    float wiggle = 0.0f;
  };

} // vke

#endif //VULKANPROJECT_RENDERER3DPUSHCONSTANTS_H