#ifndef GRAPHICSPIPELINE_H
#define GRAPHICSPIPELINE_H

#include "Pipeline.h"
#include "ShaderModule.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

class CommandBuffer;
class Light;
class RenderObject;

struct RenderInfo {
  std::shared_ptr<CommandBuffer> commandBuffer;
  uint32_t currentFrame;
  glm::vec3 viewPosition;
  const glm::mat4& viewMatrix;
  VkExtent2D extent;

  mutable glm::mat4 projectionMatrix;
  mutable bool shouldCreateProjectionMatrix = true;

  [[nodiscard]] glm::mat4& getProjectionMatrix() const
  {
    if (shouldCreateProjectionMatrix)
    {
      projectionMatrix = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(extent.width) / static_cast<float>(extent.height),
        0.1f,
        1000.0f
      );

      projectionMatrix[1][1] *= -1;
    }

    return projectionMatrix;
  }
};

class GraphicsPipeline : public Pipeline {
public:
  explicit GraphicsPipeline(const std::shared_ptr<LogicalDevice>& logicalDevice);

  virtual void render(const RenderInfo* renderInfo, const std::vector<std::shared_ptr<RenderObject>>* objects);

protected:
  std::vector<std::unique_ptr<ShaderModule>> m_shaderModules;

  void createShader(const char* filename, VkShaderStageFlagBits stage);

  virtual void loadGraphicsShaders() = 0;

  void loadDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout);
  virtual void loadGraphicsDescriptorSetLayouts() {}

  void createPipelineLayout();

  void createPipeline(const VkRenderPass& renderPass);

  void defineColorBlendState(const VkPipelineColorBlendStateCreateInfo& state);

  void defineDepthStencilState(const VkPipelineDepthStencilStateCreateInfo& state);

  void defineDynamicState(const VkPipelineDynamicStateCreateInfo& state);

  void defineInputAssemblyState(const VkPipelineInputAssemblyStateCreateInfo& state);

  void defineMultisampleState(const VkPipelineMultisampleStateCreateInfo& state);

  void defineRasterizationState(const VkPipelineRasterizationStateCreateInfo& state);

  void defineTessellationState(const VkPipelineTessellationStateCreateInfo& state);

  void defineVertexInputState(const VkPipelineVertexInputStateCreateInfo& state);

  void defineViewportState(const VkPipelineViewportStateCreateInfo& state);

  virtual void defineStates() = 0;

  virtual void updateUniformVariables(const RenderInfo* renderInfo) {}

  virtual void bindDescriptorSet(const RenderInfo* renderInfo) {}

  void definePushConstantRange(VkPushConstantRange range);

private:
  std::unique_ptr<VkPipelineColorBlendStateCreateInfo> m_colorBlendState{};
  std::unique_ptr<VkPipelineDepthStencilStateCreateInfo> m_depthStencilState{};
  std::unique_ptr<VkPipelineDynamicStateCreateInfo> m_dynamicState{};
  std::unique_ptr<VkPipelineInputAssemblyStateCreateInfo> m_inputAssemblyState{};
  std::unique_ptr<VkPipelineMultisampleStateCreateInfo> m_multisampleState{};
  std::unique_ptr<VkPipelineRasterizationStateCreateInfo> m_rasterizationState{};
  std::unique_ptr<VkPipelineTessellationStateCreateInfo> m_tessellationState{};
  std::unique_ptr<VkPipelineVertexInputStateCreateInfo> m_vertexInputState{};
  std::unique_ptr<VkPipelineViewportStateCreateInfo> m_viewportState{};

  std::vector<VkPushConstantRange> m_pushConstantRanges{};

  void destroyStates();
};



#endif //GRAPHICSPIPELINE_H
