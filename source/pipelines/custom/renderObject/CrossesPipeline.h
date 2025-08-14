#ifndef CROSSESPIPELINE_H
#define CROSSESPIPELINE_H

#include "../config/Uniforms.h"
#include "../../GraphicsPipeline.h"
#include <vulkan/vulkan.h>
#include <memory>

namespace vke {

class DescriptorSet;
class RenderPass;
class UniformBuffer;

class CrossesPipeline final : public GraphicsPipeline {
public:
  CrossesPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                  std::shared_ptr<RenderPass> renderPass,
                  VkDescriptorPool descriptorPool,
                  VkDescriptorSetLayout objectDescriptorSetLayout,
                  const std::shared_ptr<DescriptorSet>& lightingDescriptorSet);

  void displayGui() override;

private:
  CrossesUniform m_crossesUBO {
    .level = 1,
    .quantize = 50.0f,
    .size = 0.01f,
    .shininess = 10.0f
  };

  ChromaDepthUniform m_chromaDepthUBO {
    .use = false,
    .blueDepth = 4.4f,
    .redDepth = 1.0f,
  };

  CameraUniform m_cameraUBO {
    .position = glm::vec3(0)
  };

  std::shared_ptr<DescriptorSet> m_lightingDescriptorSet;
  std::shared_ptr<DescriptorSet> m_crossesDescriptorSet;

  std::shared_ptr<UniformBuffer> m_crossesUniform;

  std::shared_ptr<UniformBuffer> m_chromaDepthUniform;

  void createUniforms();

  void createDescriptorSets(VkDescriptorPool descriptorPool);

  void updateUniformVariables(const RenderInfo* renderInfo) override;

  void bindDescriptorSet(const RenderInfo* renderInfo) override;
};

} // namespace vke

#endif //CROSSESPIPELINE_H
