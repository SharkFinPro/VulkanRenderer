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

struct TransformUniform {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

class RenderObject {
public:
  RenderObject(VkDevice& device, VkPhysicalDevice& physicalDevice, const VkDescriptorSetLayout& descriptorSetLayout,
               std::shared_ptr<Texture> texture, std::shared_ptr<Texture> specularMap, std::shared_ptr<Model> model);
  ~RenderObject();

  void draw(const VkCommandBuffer& commandBuffer, const VkPipelineLayout& pipelineLayout, uint32_t currentFrame) const;

  void updateUniformBuffer(uint32_t currentFrame, const VkExtent2D& swapChainExtent, const glm::mat4& viewMatrix) const;

  void setPosition(glm::vec3 position);
  void setScale(glm::vec3 scale);
  void setScale(float scale);
  void setRotation(glm::vec3 rotation);

  [[nodiscard]] glm::vec3 getPosition() const;
  [[nodiscard]] glm::vec3 getScale() const;
  [[nodiscard]] glm::vec3 getRotation() const;

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

  void createDescriptorPool();
  void createDescriptorSets();
};


#endif //VULKANPROJECT_RENDEROBJECT_H
