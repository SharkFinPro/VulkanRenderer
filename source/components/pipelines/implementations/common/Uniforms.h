#ifndef VKE_UNIFORMS_H
#define VKE_UNIFORMS_H

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace vke {

  struct LightMetadataUniform {
    int numPointLights;
    int numSpotLights;
  };

  struct CameraUniform {
    alignas(16) glm::vec3 position;
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

  struct SnakeUniform {
    float wiggle;
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

  struct BendyUniform {
    float time;
  };

  struct BendyPlantInfo {
    glm::mat4 model;
    int leafLength;
    float pitch;
    float bendStrength;
  };

} // namespace vke

#endif //VKE_UNIFORMS_H
