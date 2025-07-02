#include "RenderObject.h"
#include <stdexcept>
#include <chrono>
#include <utility>
#include <array>
#include <glm/gtc/matrix_transform.hpp>
#include "../components/Camera.h"

#include "../pipelines/custom/Uniforms.h"

#include "Model.h"
#include "Texture.h"
#include "UniformBuffer.h"

RenderObject::RenderObject(const std::shared_ptr<LogicalDevice>& logicalDevice,
                           const std::shared_ptr<PhysicalDevice>& physicalDevice,
                           const VkDescriptorSetLayout& descriptorSetLayout, std::shared_ptr<Texture> texture,
                           std::shared_ptr<Texture> specularMap, std::shared_ptr<Model> model)
  : logicalDevice(logicalDevice), physicalDevice(physicalDevice), descriptorSetLayout(descriptorSetLayout),
    texture(std::move(texture)), specularMap(std::move(specularMap)), model(std::move(model)),
    position(0, 0, 0), scale(1, 1, 1), orientation(1, 0, 0, 0),
    transformUniform(std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, sizeof(TransformUniform)))
{
  createDescriptorPool();
  createDescriptorSets();
}

RenderObject::~RenderObject()
{
  logicalDevice->destroyDescriptorPool(descriptorPool);
}

void RenderObject::draw(const VkCommandBuffer& commandBuffer, const VkPipelineLayout& pipelineLayout,
                        const uint32_t currentFrame, const uint32_t descriptorSet) const
{
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, descriptorSet, 1, &descriptorSets[currentFrame], 0, nullptr);

  model->draw(commandBuffer);
}

void RenderObject::updateUniformBuffer(const uint32_t currentFrame, const glm::mat4& viewMatrix,
                                       const glm::mat4& projectionMatrix) const
{
  const TransformUniform transformUBO {
    .model = createModelMatrix(),
    .view = viewMatrix,
    .proj = projectionMatrix
  };

  transformUniform->update(currentFrame, &transformUBO);
}

void RenderObject::setPosition(const glm::vec3 position)
{
  this->position = position;
}

void RenderObject::setScale(const glm::vec3 scale)
{
  this->scale = scale;
}

void RenderObject::setScale(const float scale)
{
  this->scale = glm::vec3(scale);
}

void RenderObject::setOrientationEuler(const glm::vec3 orientation)
{
  this->orientation = glm::quat(glm::radians(orientation));
}

void RenderObject::setOrientationQuat(const glm::quat orientation)
{
  this->orientation = glm::normalize(orientation);
}

glm::vec3 RenderObject::getPosition() const
{
  return position;
}

glm::vec3 RenderObject::getScale() const
{
  return scale;
}

glm::vec3 RenderObject::getOrientationEuler() const
{
  return glm::degrees(glm::eulerAngles(orientation));
}

glm::quat RenderObject::getOrientationQuat() const
{
  return orientation;
}

void RenderObject::createDescriptorPool()
{
  const std::array<VkDescriptorPoolSize, 3> poolSizes {
    transformUniform->getDescriptorPoolSize(),
    Texture::getDescriptorPoolSize(logicalDevice->getMaxFramesInFlight()),
    Texture::getDescriptorPoolSize(logicalDevice->getMaxFramesInFlight())
  };

  const VkDescriptorPoolCreateInfo poolCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .maxSets = logicalDevice->getMaxFramesInFlight(),
    .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
    .pPoolSizes = poolSizes.data()
  };

  descriptorPool = logicalDevice->createDescriptorPool(poolCreateInfo);
}

void RenderObject::createDescriptorSets()
{
  const std::vector<VkDescriptorSetLayout> layouts(logicalDevice->getMaxFramesInFlight(), descriptorSetLayout);
  const VkDescriptorSetAllocateInfo allocateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = descriptorPool,
    .descriptorSetCount = logicalDevice->getMaxFramesInFlight(),
    .pSetLayouts = layouts.data()
  };

  descriptorSets.resize(logicalDevice->getMaxFramesInFlight());
  logicalDevice->allocateDescriptorSets(allocateInfo, descriptorSets.data());

  for (size_t i = 0; i < logicalDevice->getMaxFramesInFlight(); i++)
  {
    std::array<VkWriteDescriptorSet, 3> descriptorWrites {
      transformUniform->getDescriptorSet(0, descriptorSets[i], i),
      texture->getDescriptorSet(1, descriptorSets[i]),
      specularMap->getDescriptorSet(4, descriptorSets[i])
    };

    logicalDevice->updateDescriptorSets(descriptorWrites.size(), descriptorWrites.data());
  }
}

glm::mat4 RenderObject::createModelMatrix() const
{
  const glm::mat4 model = glm::translate(glm::mat4(1.0f), position)
                          * glm::mat4(orientation)
                          * glm::scale(glm::mat4(1.0f), scale);

  return model;
}
