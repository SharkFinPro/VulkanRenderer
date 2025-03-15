#ifndef GRAPHICSPIPELINE_H
#define GRAPHICSPIPELINE_H

#include "Pipeline.h"
#include "ShaderModule.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

class Light;
class RenderObject;

struct RenderInfo {
  const VkCommandBuffer& commandBuffer;
  uint32_t currentFrame;
  glm::vec3 viewPosition;
  const glm::mat4& viewMatrix;
  VkExtent2D extent;
  const std::vector<std::shared_ptr<Light>>& lights;

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
  GraphicsPipeline(const std::shared_ptr<PhysicalDevice> &physicalDevice, const std::shared_ptr<LogicalDevice> &logicalDevice);

  virtual void render(const RenderInfo* renderInfo, const std::vector<std::shared_ptr<RenderObject>>* objects) = 0;

protected:
  std::vector<std::unique_ptr<ShaderModule>> shaderModules;

  void createShader(const char* filename, VkShaderStageFlagBits stage);

  virtual void loadGraphicsShaders() = 0;

  void loadDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout);
  virtual void loadGraphicsDescriptorSetLayouts() {};

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

private:
  std::unique_ptr<VkPipelineColorBlendStateCreateInfo> colorBlendState{};
  std::unique_ptr<VkPipelineDepthStencilStateCreateInfo> depthStencilState{};
  std::unique_ptr<VkPipelineDynamicStateCreateInfo> dynamicState{};
  std::unique_ptr<VkPipelineInputAssemblyStateCreateInfo> inputAssemblyState{};
  std::unique_ptr<VkPipelineMultisampleStateCreateInfo> multisampleState{};
  std::unique_ptr<VkPipelineRasterizationStateCreateInfo> rasterizationState{};
  std::unique_ptr<VkPipelineTessellationStateCreateInfo> tessellationState{};
  std::unique_ptr<VkPipelineVertexInputStateCreateInfo> vertexInputState{};
  std::unique_ptr<VkPipelineViewportStateCreateInfo> viewportState{};

  void destroyStates();
};



#endif //GRAPHICSPIPELINE_H
