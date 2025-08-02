#ifndef VULKANPROJECT_RENDEROBJECT_H
#define VULKANPROJECT_RENDEROBJECT_H

#include <glm/gtc/quaternion.hpp>
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

class CommandBuffer;
class Model;
class Texture;
class UniformBuffer;
class LogicalDevice;

class RenderObject {
public:
  RenderObject(const std::shared_ptr<LogicalDevice>& logicalDevice,
               const VkDescriptorSetLayout& descriptorSetLayout,
               const std::shared_ptr<Texture>& texture,
               const std::shared_ptr<Texture>& specularMap,
               const std::shared_ptr<Model>& model);

  ~RenderObject();

  void draw(const std::shared_ptr<CommandBuffer>& commandBuffer,
            const VkPipelineLayout& pipelineLayout,
            uint32_t currentFrame,
            uint32_t descriptorSet = 1) const;

  void updateUniformBuffer(uint32_t currentFrame,
                           const glm::mat4& viewMatrix,
                           const glm::mat4& projectionMatrix) const;

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
  std::shared_ptr<LogicalDevice> m_logicalDevice;

  VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> m_descriptorSets;

  std::shared_ptr<Texture> m_texture;
  std::shared_ptr<Texture> m_specularMap;
  std::shared_ptr<Model> m_model;

  glm::vec3 m_position = glm::vec3(0);
  glm::vec3 m_scale = glm::vec3(1);
  glm::quat m_orientation = glm::quat(1, 0, 0, 0);

  std::unique_ptr<UniformBuffer> m_transformUniform;

  void createDescriptorPool();
  void createDescriptorSets();

  [[nodiscard]] glm::mat4 createModelMatrix() const;
};


#endif //VULKANPROJECT_RENDEROBJECT_H
