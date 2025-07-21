#include "RenderObject.h"
#include "Model.h"
#include "UniformBuffer.h"
#include "../components/textures/Texture.h"
#include "../core/commandBuffer/CommandBuffer.h"
#include "../core/logicalDevice/LogicalDevice.h"
#include "../pipelines/custom/config/Uniforms.h"
#include <glm/gtc/matrix_transform.hpp>
#include <array>

RenderObject::RenderObject(const std::shared_ptr<LogicalDevice>& logicalDevice,
                           const VkDescriptorSetLayout& descriptorSetLayout,
                           const std::shared_ptr<Texture>& texture,
                           const std::shared_ptr<Texture>& specularMap,
                           const std::shared_ptr<Model>& model)
  : m_logicalDevice(logicalDevice),
    m_descriptorSetLayout(descriptorSetLayout),
    m_texture(texture),
    m_specularMap(specularMap),
    m_model(model),
    m_transformUniform(std::make_unique<UniformBuffer>(logicalDevice, sizeof(TransformUniform)))
{
  createDescriptorPool();
  createDescriptorSets();
}

RenderObject::~RenderObject()
{
  m_logicalDevice->destroyDescriptorPool(m_descriptorPool);
}

void RenderObject::draw(const std::shared_ptr<CommandBuffer>& commandBuffer,
                        const VkPipelineLayout& pipelineLayout,
                        const uint32_t currentFrame,
                        const uint32_t descriptorSet) const
{
  commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, descriptorSet,
                                    1, &m_descriptorSets[currentFrame]);

  m_model->draw(commandBuffer);
}

void RenderObject::updateUniformBuffer(const uint32_t currentFrame,
                                       const glm::mat4& viewMatrix,
                                       const glm::mat4& projectionMatrix) const
{
  const TransformUniform transformUBO {
    .model = createModelMatrix(),
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

void RenderObject::createDescriptorPool()
{
  const std::array<VkDescriptorPoolSize, 3> poolSizes {
    m_transformUniform->getDescriptorPoolSize(),
    m_texture->getDescriptorPoolSize(),
    m_specularMap->getDescriptorPoolSize()
  };

  const VkDescriptorPoolCreateInfo poolCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .maxSets = m_logicalDevice->getMaxFramesInFlight(),
    .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
    .pPoolSizes = poolSizes.data()
  };

  m_descriptorPool = m_logicalDevice->createDescriptorPool(poolCreateInfo);
}

void RenderObject::createDescriptorSets()
{
  const auto maxFramesInFlight = m_logicalDevice->getMaxFramesInFlight();

  const std::vector<VkDescriptorSetLayout> layouts(maxFramesInFlight, m_descriptorSetLayout);
  const VkDescriptorSetAllocateInfo allocateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = m_descriptorPool,
    .descriptorSetCount = maxFramesInFlight,
    .pSetLayouts = layouts.data()
  };

  m_descriptorSets.resize(maxFramesInFlight);
  m_logicalDevice->allocateDescriptorSets(allocateInfo, m_descriptorSets.data());

  for (size_t i = 0; i < maxFramesInFlight; i++)
  {
    std::array<VkWriteDescriptorSet, 3> descriptorWrites {
      m_transformUniform->getDescriptorSet(0, m_descriptorSets[i], i),
      m_texture->getDescriptorSet(1, m_descriptorSets[i]),
      m_specularMap->getDescriptorSet(4, m_descriptorSets[i])
    };

    m_logicalDevice->updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data());
  }
}

glm::mat4 RenderObject::createModelMatrix() const
{
  const glm::mat4 model = glm::translate(glm::mat4(1.0f), m_position)
                          * glm::mat4(m_orientation)
                          * glm::scale(glm::mat4(1.0f), m_scale);

  return model;
}
