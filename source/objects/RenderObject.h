#ifndef VULKANPROJECT_RENDEROBJECT_H
#define VULKANPROJECT_RENDEROBJECT_H

#include <vulkan/vulkan.h>
#include <vector>
#include <glm/glm.hpp>
#include <memory>

struct TransformUniform {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

struct LightUniform {
  alignas(16) glm::vec3 position;
  alignas(16) glm::vec3 color;

  alignas(4) float ambient;
  alignas(4) float diffuse;
  alignas(4) float specular;
};

struct CameraUniform {
  alignas(16) glm::vec3 position;
};

class Model;
class Texture;
class Camera;
class UniformBuffer;

class RenderObject {
public:
  RenderObject(VkDevice& device, VkPhysicalDevice& physicalDevice, VkDescriptorSetLayout& descriptorSetLayout,
               std::shared_ptr<Texture> texture, std::shared_ptr<Texture> specularMap, std::shared_ptr<Model> model);
  ~RenderObject();

  void draw(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t currentFrame);

  void updateUniformBuffer(uint32_t currentFrame, VkExtent2D& swapChainExtent, std::shared_ptr<Camera>& camera);

  void setPosition(glm::vec3 position);

private:
  void createDescriptorPool();
  void createDescriptorSets();

private:
  VkDevice& device;
  VkPhysicalDevice& physicalDevice;

  glm::vec3 position;

  std::shared_ptr<Texture> texture;
  std::shared_ptr<Texture> specularMap;
  std::shared_ptr<Model> model;

  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorPool descriptorPool;
  std::vector<VkDescriptorSet> descriptorSets;

  std::unique_ptr<UniformBuffer> transformUniform;
};


#endif //VULKANPROJECT_RENDEROBJECT_H
