#ifndef VULKANPROJECT_OBJECTSPIPELINE_H
#define VULKANPROJECT_OBJECTSPIPELINE_H

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

class RenderPass;
class RenderObject;
class Camera;
class UniformBuffer;

class ObjectsPipeline {
public:
  ObjectsPipeline(VkDevice& device, VkPhysicalDevice& physicalDevice, const char* vertexShader,
                   const char* fragmentShader, VkSampleCountFlagBits msaaSamples, std::shared_ptr<RenderPass> renderPass);
  ~ObjectsPipeline();

  VkDescriptorSetLayout& getLayout();

  void render(const VkCommandBuffer& commandBuffer, uint32_t currentFrame, const std::shared_ptr<Camera>& camera,
              VkExtent2D swapChainExtent);

  void insertRenderObject(const std::shared_ptr<RenderObject>& renderObject);

private:
  void createGraphicsPipeline(const char* vertexShader, const char* fragmentShader, VkSampleCountFlagBits msaaSamples,
                              std::shared_ptr<RenderPass>& renderPass);

  void createDescriptorSetLayout();

  void createDescriptorPool();

  void createDescriptorSets();

private:
  VkDevice& device;
  VkPhysicalDevice& physicalDevice;

  std::vector<std::shared_ptr<RenderObject>> renderObjects;

  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;

  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorSetLayout objectDescriptorSetLayout;

  VkDescriptorPool descriptorPool;
  std::vector<VkDescriptorSet> descriptorSets;

  std::unique_ptr<UniformBuffer> lightUniform;
  std::unique_ptr<UniformBuffer> cameraUniform;

  float position[3];
  float color[3];
  float ambient;
  float diffuse;
};


#endif //VULKANPROJECT_GRAPHICSPIPELINE_H