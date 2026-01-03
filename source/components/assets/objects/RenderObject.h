#ifndef VKE_RENDEROBJECT_H
#define VKE_RENDEROBJECT_H

#include "../../pipelines/uniformBuffers/UniformBuffer.h"
#include <glm/gtc/quaternion.hpp>
#include <vulkan/vulkan.h>
#include <memory>

namespace vke {

  class CommandBuffer;
  class DescriptorSet;
  class LogicalDevice;
  class Model;
  class Texture;

  class RenderObject {
  public:
    RenderObject(const std::shared_ptr<LogicalDevice>& logicalDevice,
                 VkDescriptorPool descriptorPool,
                 VkDescriptorSetLayout descriptorSetLayout,
                 std::shared_ptr<Texture> texture,
                 std::shared_ptr<Texture> specularMap,
                 std::shared_ptr<Model> model);

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
    std::shared_ptr<DescriptorSet> m_descriptorSet;

    std::shared_ptr<Texture> m_texture;
    std::shared_ptr<Texture> m_specularMap;
    std::shared_ptr<Model> m_model;

    glm::vec3 m_position = glm::vec3(0);
    glm::vec3 m_scale = glm::vec3(1);
    glm::quat m_orientation = glm::quat(1, 0, 0, 0);

    std::unique_ptr<UniformBuffer> m_transformUniform;

    void createDescriptorSet(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             VkDescriptorPool descriptorPool,
                             VkDescriptorSetLayout descriptorSetLayout);

    [[nodiscard]] glm::mat4 createModelMatrix() const;
  };

} // namespace vke

#endif //VKE_RENDEROBJECT_H
