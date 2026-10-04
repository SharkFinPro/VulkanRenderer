#ifndef VULKANPROJECT_POINTLIGHT_H
#define VULKANPROJECT_POINTLIGHT_H

#include "Light.h"
#include <array>

namespace vke {

  class DescriptorSet;
  class UniformBuffer;

  class PointLight final : public Light {
  public:
    PointLight(std::shared_ptr<LogicalDevice> logicalDevice,
               const CommonLightData& commonLightData,
               const vk::CommandPool& commandPool,
               vk::DescriptorPool descriptorPool,
               vk::DescriptorSetLayout descriptorSetLayout);

    [[nodiscard]] LightType getLightType() const override;

    [[nodiscard]] LightUniform getUniform() const override;

    [[nodiscard]] std::array<glm::mat4, 6> getLightViewProjectionMatrices() const;

    void updateUniform(uint32_t currentFrame) const;

    [[nodiscard]] vk::DescriptorSet getDescriptorSet(uint32_t currentFrame) const;

  private:
    std::shared_ptr<DescriptorSet> m_descriptorSet;

    std::shared_ptr<UniformBuffer> m_viewProjectionUniform;

    void createShadowMap(vk::CommandPool commandPool) override;

    void createUniform();

    void createDescriptorSet(vk::DescriptorPool descriptorPool,
                             vk::DescriptorSetLayout descriptorSetLayout);
  };

} // vke

#endif //VULKANPROJECT_POINTLIGHT_H