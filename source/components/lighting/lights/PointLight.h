#ifndef VULKANPROJECT_POINTLIGHT_H
#define VULKANPROJECT_POINTLIGHT_H

#include "Light.h"
#include <array>

namespace vke {

  class CommandBuffer;
  class DescriptorSet;
  struct RenderInfo;
  class UniformBuffer;

  class PointLight final : public Light {
  public:
    PointLight(std::shared_ptr<LogicalDevice> logicalDevice,
               const CommonLightData& commonLightData,
               const VkCommandPool& commandPool,
               VkDescriptorPool descriptorPool,
               VkDescriptorSetLayout descriptorSetLayout,
               const std::shared_ptr<Renderer>& renderer);

    [[nodiscard]] LightType getLightType() const override;

    [[nodiscard]] LightUniform getUniform() const override;

    [[nodiscard]] std::array<glm::mat4, 6> getLightViewProjectionMatrices() const;

    void updateUniform(uint32_t currentFrame) const;

    [[nodiscard]] VkDescriptorSet getDescriptorSet(uint32_t currentFrame) const;

  private:
    std::shared_ptr<DescriptorSet> m_descriptorSet;

    std::shared_ptr<UniformBuffer> m_viewProjectionUniform;

    void createShadowMap(const VkCommandPool& commandPool) override;

    void createUniform();

    void createDescriptorSet(VkDescriptorPool descriptorPool,
                             VkDescriptorSetLayout descriptorSetLayout);
  };

} // vke

#endif //VULKANPROJECT_POINTLIGHT_H