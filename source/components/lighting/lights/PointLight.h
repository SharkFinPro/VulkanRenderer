#ifndef VULKANPROJECT_POINTLIGHT_H
#define VULKANPROJECT_POINTLIGHT_H

#include "Light.h"
#include <vulkan/vulkan.h>

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

    ~PointLight();

    [[nodiscard]] LightType getLightType() const override;

    [[nodiscard]] LightUniform getUniform() const override;

  private:
    VkImage m_shadowMap = VK_NULL_HANDLE;
    VkImageView m_shadowMapView = VK_NULL_HANDLE;
    VkDeviceMemory m_shadowMapMemory = VK_NULL_HANDLE;

    bool m_castsShadows = true;
    uint32_t m_shadowMapSize = 1024;

    void createShadowMap(const VkCommandPool& commandPool);

    void destroyShadowMap();
  };
} // vke

#endif //VULKANPROJECT_POINTLIGHT_H