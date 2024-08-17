#ifndef VULKANPROJECT_RENDEROBJECT_H
#define VULKANPROJECT_RENDEROBJECT_H

#include <vulkan/vulkan.h>
#include <vector>
#include <glm/glm.hpp>
#include <memory>

class Model;
class Texture;
class Camera;
class UniformBuffer;

class RenderObject {
public:
  RenderObject(VkDevice& device, VkPhysicalDevice& physicalDevice, const VkDescriptorSetLayout& descriptorSetLayout,
               std::shared_ptr<Texture> texture, std::shared_ptr<Texture> specularMap, std::shared_ptr<Model> model);
  ~RenderObject();

  void draw(const VkCommandBuffer& commandBuffer, const VkPipelineLayout& pipelineLayout, uint32_t currentFrame);

  void updateUniformBuffer(uint32_t currentFrame, const VkExtent2D& swapChainExtent, const std::shared_ptr<Camera>& camera);

  void setPosition(glm::vec3 position);

private:
  void createDescriptorPool();
  void createDescriptorSets();

private:
  VkDevice& device;
  VkPhysicalDevice& physicalDevice;

  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorPool descriptorPool;
  std::vector<VkDescriptorSet> descriptorSets;

  std::shared_ptr<Texture> texture;
  std::shared_ptr<Texture> specularMap;
  std::shared_ptr<Model> model;

  glm::vec3 position;

  std::unique_ptr<UniformBuffer> transformUniform;
};


#endif //VULKANPROJECT_RENDEROBJECT_H
