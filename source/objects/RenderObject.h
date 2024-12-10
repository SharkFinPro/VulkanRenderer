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

  void draw(const VkCommandBuffer& commandBuffer, const VkPipelineLayout& pipelineLayout, uint32_t currentFrame) const;

  void updateUniformBuffer(uint32_t currentFrame, const VkExtent2D& swapChainExtent, const std::shared_ptr<Camera>& camera) const;

  void setPosition(glm::vec3 position);
  void setScale(glm::vec3 scale);
  void setScale(float scale);
  void setRotation(glm::vec3 rotation);

  void enableRendering();
  void disableRendering();

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
  glm::vec3 scale;
  glm::vec3 rotation;

  std::unique_ptr<UniformBuffer> transformUniform;

  bool doRendering;

  void createDescriptorPool();
  void createDescriptorSets();
};


#endif //VULKANPROJECT_RENDEROBJECT_H
