#ifndef VULKANPROJECT_SPOTLIGHT_H
#define VULKANPROJECT_SPOTLIGHT_H

#include "Light.h"

namespace vke {

  class SpotLight final : public Light {
  public:
    SpotLight(std::shared_ptr<LogicalDevice> logicalDevice,
              const CommonLightData& commonLightData,
              const VkCommandPool& commandPool,
              const std::shared_ptr<Renderer>& renderer);

    [[nodiscard]] glm::vec3 getDirection() const;
    [[nodiscard]] float getConeAngle() const;

    void setDirection(const glm::vec3& direction);
    void setConeAngle(float coneAngle);

    [[nodiscard]] LightType getLightType() const override;

    [[nodiscard]] LightUniform getUniform() const override;

    [[nodiscard]] glm::mat4 getLightViewProjectionMatrix() const;

  private:
    glm::vec3 m_direction = glm::vec3(0, -1, 0);
    float m_coneAngle = 15;

    void createShadowMap(const VkCommandPool& commandPool) override;
  };

} // vke

#endif //VULKANPROJECT_SPOTLIGHT_H