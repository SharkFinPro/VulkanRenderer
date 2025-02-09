#ifndef VULKANPROJECT_OBJECTSPIPELINE_H
#define VULKANPROJECT_OBJECTSPIPELINE_H

#include <vector>
#include <memory>
#include <array>
#include <glm/glm.hpp>
#include "../GraphicsPipeline.h"

class RenderPass;
class RenderObject;
class Camera;
class UniformBuffer;
class Light;

class ObjectsPipeline final : public GraphicsPipeline {
public:
  ObjectsPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice, const std::shared_ptr<LogicalDevice>& logicalDevice,
                  const std::shared_ptr<RenderPass>& renderPass);
  ~ObjectsPipeline() override;

  VkDescriptorSetLayout& getLayout();

  void render(const VkCommandBuffer& commandBuffer, uint32_t currentFrame, glm::vec3 viewPosition,
              const glm::mat4& viewMatrix, VkExtent2D swapChainExtent,
              const std::vector<std::shared_ptr<Light>>& lights,
              const std::vector<std::shared_ptr<RenderObject>>& objects);

private:
  VkDescriptorSetLayout globalDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout objectDescriptorSetLayout = VK_NULL_HANDLE;

  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptorSets;

  std::unique_ptr<UniformBuffer> lightMetadataUniform;
  std::unique_ptr<UniformBuffer> lightsUniform;
  std::unique_ptr<UniformBuffer> cameraUniform;

  int prevNumLights = 0;

  size_t lightsUniformBufferSize;

  VkPipelineColorBlendAttachmentState colorBlendAttachment;

  std::array<VkDynamicState, 2> dynamicStates;

  VkVertexInputBindingDescription vertexBindingDescription;
  std::array<VkVertexInputAttributeDescription, 3> vertexAttributeDescriptions;

  void loadGraphicsShaders() override;

  void loadGraphicsDescriptorSetLayouts() override;

  std::unique_ptr<VkPipelineColorBlendStateCreateInfo> defineColorBlendState() override;
  std::unique_ptr<VkPipelineDepthStencilStateCreateInfo> defineDepthStencilState() override;
  std::unique_ptr<VkPipelineDynamicStateCreateInfo> defineDynamicState() override;
  std::unique_ptr<VkPipelineInputAssemblyStateCreateInfo> defineInputAssemblyState() override;
  std::unique_ptr<VkPipelineMultisampleStateCreateInfo> defineMultisampleState() override;
  std::unique_ptr<VkPipelineRasterizationStateCreateInfo> defineRasterizationState() override;
  std::unique_ptr<VkPipelineVertexInputStateCreateInfo> defineVertexInputState() override;
  std::unique_ptr<VkPipelineViewportStateCreateInfo> defineViewportState() override;

  void createDescriptorSetLayouts();

  void createGlobalDescriptorSetLayout();
  void createObjectDescriptorSetLayout();

  void createDescriptorPool();

  void createDescriptorSets();

  void createUniforms();

  void updateLightUniforms(const std::vector<std::shared_ptr<Light>>& lights, uint32_t currentFrame);
};


#endif //VULKANPROJECT_OBJECTSPIPELINE_H
