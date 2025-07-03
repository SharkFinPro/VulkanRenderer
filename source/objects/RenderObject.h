#ifndef VULKANPROJECT_RENDEROBJECT_H
#define VULKANPROJECT_RENDEROBJECT_H

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <memory>

class CommandBuffer;
class Model;
class Texture;
class Camera;
class UniformBuffer;
class LogicalDevice;
class PhysicalDevice;

class RenderObject {
public:
  RenderObject(const std::shared_ptr<LogicalDevice>& logicalDevice,
               const std::shared_ptr<PhysicalDevice>& physicalDevice, const VkDescriptorSetLayout& descriptorSetLayout,
               std::shared_ptr<Texture> texture, std::shared_ptr<Texture> specularMap, std::shared_ptr<Model> model);
  ~RenderObject();

  void draw(const std::shared_ptr<CommandBuffer>& commandBuffer, const VkPipelineLayout& pipelineLayout, uint32_t currentFrame,
            uint32_t descriptorSet = 1) const;

  void updateUniformBuffer(uint32_t currentFrame, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) const;

  void setPosition(glm::vec3 position);
  void setScale(glm::vec3 scale);
  void setScale(float scale);
  void setOrientationEuler(glm::vec3 orientation);
  void setOrientationQuat(glm::quat orientation);

  [[nodiscard]] glm::vec3 getPosition() const;
  [[nodiscard]] glm::vec3 getScale() const;
  [[nodiscard]] glm::vec3 getOrientationEuler() const;
  [[nodiscard]] glm::quat getOrientationQuat() const;

private:
  std::shared_ptr<LogicalDevice> logicalDevice;
  std::shared_ptr<PhysicalDevice> physicalDevice;

  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> descriptorSets;

  std::shared_ptr<Texture> texture;
  std::shared_ptr<Texture> specularMap;
  std::shared_ptr<Model> model;

  glm::vec3 position;
  glm::vec3 scale;
  glm::quat orientation;

  std::unique_ptr<UniformBuffer> transformUniform;

  void createDescriptorPool();
  void createDescriptorSets();

  [[nodiscard]] glm::mat4 createModelMatrix() const;
};


#endif //VULKANPROJECT_RENDEROBJECT_H
