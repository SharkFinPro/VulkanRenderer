#ifndef VULKANPROJECT_POINTLIGHT_H
#define VULKANPROJECT_POINTLIGHT_H

#include "Light.h"
#include <array>

namespace vke {
  class PointLight : public Light {
  public:
    PointLight(const std::shared_ptr<LogicalDevice>& logicalDevice,
               const glm::vec3& position,
               const glm::vec3& color,
               float ambient,
               float diffuse,
               float specular,
               const VkCommandPool& commandPool);

    [[nodiscard]] LightType getLightType() const override;

    [[nodiscard]] LightUniform getUniform() const override;

    [[nodiscard]] std::array<glm::mat4, 6> getLightViewProjectionMatrices() const;

  private:
    void createShadowMap(const VkCommandPool& commandPool) override;
  };
} // vke

#endif //VULKANPROJECT_POINTLIGHT_H