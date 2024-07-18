#ifndef VULKANPROJECT_GRAPHICSPIPELINE_H
#define VULKANPROJECT_GRAPHICSPIPELINE_H

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

class RenderPass;
class RenderObject;
class Camera;
class UniformBuffer;

class GraphicsPipeline {
public:
  GraphicsPipeline(VkDevice& device, VkPhysicalDevice& physicalDevice, const char* vertexShader,
                   const char* fragmentShader, VkExtent2D& swapChainExtent,
                   VkSampleCountFlagBits msaaSamples, std::shared_ptr<RenderPass> renderPass);
  ~GraphicsPipeline();

  VkRenderPass& getRenderPass();

  VkDescriptorSetLayout& getLayout();

  void render(VkCommandBuffer& commandBuffer, uint32_t imageIndex, uint32_t currentFrame,
              const std::vector<VkFramebuffer>& swapChainFramebuffers, std::shared_ptr<Camera> camera);

  void insertRenderObject(std::shared_ptr<RenderObject>& renderObject);

private:
  void createGraphicsPipeline(const char* vertexShader, const char* fragmentShader, VkSampleCountFlagBits msaaSamples);

  VkShaderModule createShaderModule(const char* file);

  void createDescriptorSetLayout();

  void createDescriptorPool();

  void createDescriptorSets();

private:
  VkDevice& device;
  VkPhysicalDevice& physicalDevice;

  std::vector<std::shared_ptr<RenderObject>> renderObjects;

  std::shared_ptr<RenderPass> renderPass;

  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;

  VkExtent2D& swapChainExtent;

  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorSetLayout objectDescriptorSetLayout;

  VkDescriptorPool descriptorPool;
  std::vector<VkDescriptorSet> descriptorSets;

  std::unique_ptr<UniformBuffer> lightUniform;
  std::unique_ptr<UniformBuffer> cameraUniform;
};


#endif //VULKANPROJECT_GRAPHICSPIPELINE_H
