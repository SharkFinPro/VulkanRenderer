#ifndef VULKANPROJECT_POINTLIGHT_H
#define VULKANPROJECT_POINTLIGHT_H

#include "Light.h"
#include <array>

namespace vke {

  class PointLight final : public Light {
  public:
    PointLight(std::shared_ptr<LogicalDevice> logicalDevice,
               const CommonLightData& commonLightData,
               const vk::CommandPool& commandPool);

    [[nodiscard]] LightType getLightType() const override;

    [[nodiscard]] LightUniform getUniform() const override;

    // One view-projection per cube face, in face order. Reversed depth: the near plane maps to 1 and the shadow range to
    // 0, so the 32-bit float depth keeps its precision across the whole range (see pointLightShadow in
    // shaders/common/SceneLighting.glsl, which rebuilds the same depth).
    [[nodiscard]] std::array<glm::mat4, 6> getLightViewProjectionMatrices() const;

  private:
    void createShadowMap(vk::CommandPool commandPool) override;
  };

} // vke

#endif //VULKANPROJECT_POINTLIGHT_H