#ifndef VULKANPROJECT_RENDEROBJECT_H
#define VULKANPROJECT_RENDEROBJECT_H

#include <vulkan/vulkan.h>
#include <vector>
#include <glm/glm.hpp>
#include <memory>

struct UniformBufferObject {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

class Model;
class Texture;
class Camera;

class RenderObject {
public:
  RenderObject(VkDevice& device, VkPhysicalDevice& physicalDevice, VkDescriptorSetLayout& descriptorSetLayout,
               std::shared_ptr<Texture> texture, std::shared_ptr<Model> model);
  ~RenderObject();

  void draw(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t currentFrame);

  void updateUniformBuffer(uint32_t currentFrame, VkExtent2D& swapChainExtent, std::shared_ptr<Camera>& camera);

  void setPosition(glm::vec3 position);

private:
  void createUniformBuffers();

  void createDescriptorPool();
  void createDescriptorSets(VkDescriptorSetLayout& descriptorSetLayout);

private:
  VkDevice& device;
  VkPhysicalDevice& physicalDevice;

  glm::vec3 position;

  std::shared_ptr<Texture> texture;
  std::shared_ptr<Model> model;

  std::vector<VkBuffer> uniformBuffers;
  std::vector<VkDeviceMemory> uniformBuffersMemory;
  std::vector<void*> uniformBuffersMapped;
  VkDescriptorPool descriptorPool;
  std::vector<VkDescriptorSet> descriptorSets;
};


#endif //VULKANPROJECT_RENDEROBJECT_H
