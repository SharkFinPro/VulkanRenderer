#include "RenderObject.h"
#include "Model.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../pipelines/descriptorSets/DescriptorSet.h"
#include "../../pipelines/implementations/common/Uniforms.h"
#include "../../assets/textures/Texture.h"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

namespace vke {

  RenderObject::RenderObject(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             VkDescriptorPool descriptorPool,
                             VkDescriptorSetLayout descriptorSetLayout,
                             std::shared_ptr<Texture> texture,
                             std::shared_ptr<Texture> specularMap,
                             std::shared_ptr<Model> model)
    : m_texture(std::move(texture)),
      m_specularMap(std::move(specularMap)),
      m_model(std::move(model)),
      m_transformUniform(std::make_unique<UniformBuffer>(logicalDevice, sizeof(TransformUniform)))
  {
    createDescriptorSet(logicalDevice, descriptorPool, descriptorSetLayout);
  }

  void RenderObject::draw(const std::shared_ptr<CommandBuffer>& commandBuffer,
                          const VkPipelineLayout& pipelineLayout,
                          const uint32_t currentFrame) const
  {
    commandBuffer->bindDescriptorSets(
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipelineLayout,
      0,
      1,
      &m_descriptorSet->getDescriptorSet(currentFrame)
    );

    m_model->draw(commandBuffer);
  }

  void RenderObject::draw(const std::shared_ptr<CommandBuffer>& commandBuffer) const
  {
    m_model->draw(commandBuffer);
  }

  void RenderObject::updateUniformBuffer(const uint32_t currentFrame,
                                         const glm::mat4& viewMatrix,
                                         const glm::mat4& projectionMatrix) const
  {
    const TransformUniform transformUBO {
      .model = getModelMatrix(),
      .view = viewMatrix,
      .proj = projectionMatrix
    };

    m_transformUniform->update(currentFrame, &transformUBO);
  }

  void RenderObject::setPosition(const glm::vec3 position)
  {
    m_position = position;
  }

  void RenderObject::setScale(const glm::vec3 scale)
  {
    m_scale = scale;
  }

  void RenderObject::setScale(const float scale)
  {
    m_scale = glm::vec3(scale);
  }

  void RenderObject::setOrientationEuler(const glm::vec3 orientation)
  {
    m_orientation = glm::quat(glm::radians(orientation));
  }

  void RenderObject::setOrientationQuat(const glm::quat orientation)
  {
    m_orientation = glm::normalize(orientation);
  }

  glm::vec3 RenderObject::getPosition() const
  {
    return m_position;
  }

  glm::vec3 RenderObject::getScale() const
  {
    return m_scale;
  }

  glm::vec3 RenderObject::getOrientationEuler() const
  {
    return glm::degrees(glm::eulerAngles(m_orientation));
  }

  glm::quat RenderObject::getOrientationQuat() const
  {
    return m_orientation;
  }

  VkDescriptorSet RenderObject::getDescriptorSet(const uint32_t currentFrame) const
  {
    return m_descriptorSet->getDescriptorSet(currentFrame);
  }

  std::shared_ptr<Model> RenderObject::getModel() const
  {
    return m_model;
  }

  glm::mat4 RenderObject::getModelMatrix() const
  {
    const glm::mat4 model = glm::translate(glm::mat4(1.0f), m_position)
                            * glm::mat4(m_orientation)
                            * glm::scale(glm::mat4(1.0f), m_scale);

    return model;
  }

  void RenderObject::createDescriptorSet(const std::shared_ptr<LogicalDevice>& logicalDevice,
                                         VkDescriptorPool descriptorPool,
                                         VkDescriptorSetLayout descriptorSetLayout)
  {
    m_descriptorSet = std::make_shared<DescriptorSet>(logicalDevice, descriptorPool, descriptorSetLayout);
    m_descriptorSet->updateDescriptorSets([this](VkDescriptorSet descriptorSet, const size_t frame)
    {
      std::vector<VkWriteDescriptorSet> descriptorWrites{{
        m_transformUniform->getDescriptorSet(0, descriptorSet, frame),
        m_texture->getDescriptorSet(1, descriptorSet),
        m_specularMap->getDescriptorSet(4, descriptorSet)
      }};

      return descriptorWrites;
    });
  }

} // namespace vke