#ifndef VULKANPROJECT_UNIFORMS_H
#define VULKANPROJECT_UNIFORMS_H

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

struct LightMetadataUniform {
  int numPointLights;
  int numSpotLights;
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

struct CubeMapUniform {
  float mix;
  float refractionIndex;
  float whiteMix;
};

struct MagnifyWhirlMosaicUniform {
  float lensS;
  float lensT;
  float lensRadius;
  float magnification;
  float whirl;
  float mosaic;
};

struct SnakeUniform {
  float wiggle;
};

struct CrossesUniform {
  int level;
  float quantize;
  float size;
  float shininess;
};

struct ChromaDepthUniform {
  alignas(16) bool use;
  char padding[3];
  float blueDepth;
  float redDepth;
};

struct DeltaTimeUniform {
  float deltaTime = 1.0f;
};

struct TransformUniform {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

using VPTransformUniform = glm::mat4;

using MVPTransformUniform = glm::mat4;

struct ViewProjTransformUniform {
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

struct SmokeUniform {
  glm::vec3 systemPosition;
  float spreadFactor;
  float maxSpreadDistance;
  float windStrength;
};

struct MousePickingID {
  uint32_t objectID;
};

struct BendyUniform {
  float time;
};

struct BendyPlantInfo {
  glm::mat4 model;
  int leafLength;
  float pitch;
  float bendStrength;
};

#endif //VULKANPROJECT_UNIFORMS_H
