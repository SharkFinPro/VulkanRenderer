#ifndef VKE_BUMPYCURTAIN_H
#define VKE_BUMPYCURTAIN_H

#include "../common/Uniforms.h"
#include "../../GraphicsPipeline.h"
#include <vulkan/vulkan.h>
#include <memory>

namespace vke {

  class DescriptorSet;
  class RenderPass;
  class UniformBuffer;
  class Texture3D;

  class BumpyCurtain final : public GraphicsPipeline {
  public:
    BumpyCurtain(std::shared_ptr<LogicalDevice> logicalDevice,
                 std::shared_ptr<RenderPass> renderPass,
                 const VkCommandPool& commandPool,
                 VkDescriptorPool descriptorPool,
                 VkDescriptorSetLayout objectDescriptorSetLayout,
                 const std::shared_ptr<DescriptorSet>& lightingDescriptorSet);

    void displayGui() override;

  private:
    CurtainUniform m_curtainUBO {
      .amplitude = 0.1,
      .period = 1,
      .shininess = 10
    };

    NoiseOptionsUniform m_noiseOptionsUBO {
      .amplitude = 0.5f,
      .frequency = 1.0f
    };

    std::shared_ptr<DescriptorSet> m_lightingDescriptorSet;
    std::shared_ptr<DescriptorSet> m_bumpyCurtainDescriptorSet;

    VkDescriptorSetLayout m_objectDescriptorSetLayout = VK_NULL_HANDLE;

    std::shared_ptr<UniformBuffer> m_curtainUniform;
    std::shared_ptr<UniformBuffer> m_noiseOptionsUniform;
    std::shared_ptr<Texture3D> m_noiseTexture;

    void createUniforms(const VkCommandPool& commandPool);

    void createDescriptorSets(VkDescriptorPool descriptorPool);

    void updateUniformVariables(const RenderInfo* renderInfo) override;

    void bindDescriptorSet(const RenderInfo* renderInfo) override;
  };

} // namespace vke

#endif //VKE_BUMPYCURTAIN_H
