#ifndef VULKANPROJECT_SPOTLIGHT_H
#define VULKANPROJECT_SPOTLIGHT_H

#include "Light.h"
#include <vulkan/vulkan.h>

namespace vke {
  class SpotLight : public Light {
  public:
    SpotLight(const std::shared_ptr<LogicalDevice>& logicalDevice,
              const glm::vec3& position,
              const glm::vec3& color,
              float ambient,
              float diffuse,
              float specular,
              const VkCommandPool& commandPool);

    ~SpotLight() override;

    [[nodiscard]] glm::vec3 getDirection() const;
    [[nodiscard]] float getConeAngle() const;

    void setDirection(const glm::vec3& direction);
    void setConeAngle(float coneAngle);

    [[nodiscard]] LightType getLightType() const override;

    [[nodiscard]] LightUniform getUniform() const override;

    [[nodiscard]] VkImage getShadowMap() const;
    [[nodiscard]] VkImageView getShadowMapView() const;
    [[nodiscard]] uint32_t getShadowMapSize() const;
    [[nodiscard]] bool castsShadows() const;

    [[nodiscard]] glm::mat4 getLightViewProjectionMatrix() const;

  private:
    VkImage m_shadowMap = VK_NULL_HANDLE;
    VkImageView m_shadowMapView = VK_NULL_HANDLE;
    VkDeviceMemory m_shadowMapMemory = VK_NULL_HANDLE;

    bool m_castsShadows = true;
    uint32_t m_shadowMapSize = 1024;

    glm::vec3 m_direction = glm::vec3(0, -1, 0);
    float m_coneAngle = 15;

    void createShadowMap(const VkCommandPool& commandPool);

    void destroyShadowMap();
  };
} // vke

#endif //VULKANPROJECT_SPOTLIGHT_H