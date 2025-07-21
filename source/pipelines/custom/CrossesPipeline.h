#ifndef CROSSESPIPELINE_H
#define CROSSESPIPELINE_H

#include "config/Uniforms.h"
#include "../GraphicsPipeline.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

class DescriptorSet;
class LogicalDevice;
class RenderPass;
class RenderObject;
class Camera;
class UniformBuffer;
class Light;

class CrossesPipeline final : public GraphicsPipeline {
public:
  CrossesPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice,
                  const std::shared_ptr<RenderPass>& renderPass,
                  VkDescriptorPool descriptorPool,
                  VkDescriptorSetLayout objectDescriptorSetLayout);

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

  VkDescriptorSetLayout m_objectDescriptorSetLayout = VK_NULL_HANDLE;

  std::shared_ptr<UniformBuffer> m_lightMetadataUniform;
  std::shared_ptr<UniformBuffer> m_lightsUniform;
  std::shared_ptr<UniformBuffer> m_cameraUniform;

  std::shared_ptr<UniformBuffer> m_crossesUniform;

  std::shared_ptr<UniformBuffer> m_chromaDepthUniform;

  int m_prevNumLights = 0;

  size_t m_lightsUniformBufferSize = 0;

  void loadGraphicsShaders() override;

  void loadGraphicsDescriptorSetLayouts() override;

  void defineStates() override;

  void createUniforms();

  void createDescriptorSets(VkDescriptorPool descriptorPool);

  void updateLightUniforms(const std::vector<std::shared_ptr<Light>>& lights, uint32_t currentFrame);

  void updateUniformVariables(const RenderInfo* renderInfo) override;

  void bindDescriptorSet(const RenderInfo* renderInfo) override;
};



#endif //CROSSESPIPELINE_H
