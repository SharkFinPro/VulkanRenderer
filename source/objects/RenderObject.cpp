#include "RenderObject.h"
#include <stdexcept>
#include <chrono>
#include <utility>
#include <array>
#include "glm/gtc/matrix_transform.hpp"
#include "../components/Camera.h"

#include "Model.h"
#include "Texture.h"
#include "UniformBuffer.h"

#include "../pipelines/objects/Uniforms.h"

constexpr int MAX_FRAMES_IN_FLIGHT = 2; // TODO: link this better

RenderObject::RenderObject(VkDevice& device, VkPhysicalDevice& physicalDevice,
                           const VkDescriptorSetLayout& descriptorSetLayout, std::shared_ptr<Texture> texture,
                           std::shared_ptr<Texture> specularMap, std::shared_ptr<Model> model)
  : device(device), physicalDevice(physicalDevice), descriptorSetLayout(descriptorSetLayout),
    texture(std::move(texture)), specularMap(std::move(specularMap)), model(std::move(model)),
    position(0, 0, 0), scale(1, 1, 1), rotation(0, 0, 0),
    transformUniform(std::make_unique<UniformBuffer>(device, physicalDevice, MAX_FRAMES_IN_FLIGHT, sizeof(TransformUniform))),
    doRendering(false)
{
  createDescriptorPool();
  createDescriptorSets();
}

RenderObject::~RenderObject()
{
  vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void RenderObject::draw(const VkCommandBuffer& commandBuffer, const VkPipelineLayout& pipelineLayout, const uint32_t currentFrame) const
{
  if (!doRendering)
  {
    return;
  }

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &descriptorSets[currentFrame], 0, nullptr);

  model->draw(commandBuffer);
}

void RenderObject::updateUniformBuffer(const uint32_t currentFrame, const VkExtent2D& swapChainExtent, const std::shared_ptr<Camera>& camera) const
{
  if (!doRendering)
  {
    return;
  }

  auto projection = glm::perspective(glm::radians(45.0f),
                             static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height),
                             0.1f, 1000.0f);
  projection[1][1] *= -1;

  const TransformUniform transformUBO {
    .model = glm::translate(glm::mat4(1.0f), position)
      * glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0, 0, 1))
      * glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0, 1, 0))
      * glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1, 0, 0))
      * glm::scale(glm::mat4(1.0f), scale),
    .view = camera->getViewMatrix(),
    .proj = projection
  };

  transformUniform->update(currentFrame, &transformUBO, sizeof(TransformUniform));
}

void RenderObject::createDescriptorPool()
{
  const std::array<VkDescriptorPoolSize, 3> poolSizes {
    transformUniform->getDescriptorPoolSize(),
    Texture::getDescriptorPoolSize(MAX_FRAMES_IN_FLIGHT),
    Texture::getDescriptorPoolSize(MAX_FRAMES_IN_FLIGHT)
  };

  const VkDescriptorPoolCreateInfo poolCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
    .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
    .pPoolSizes = poolSizes.data()
  };

  if (vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &descriptorPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void RenderObject::createDescriptorSets()
{
  const std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
  const VkDescriptorSetAllocateInfo allocateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = descriptorPool,
    .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
    .pSetLayouts = layouts.data()
  };

  descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  if (vkAllocateDescriptorSets(device, &allocateInfo, descriptorSets.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    std::array<VkWriteDescriptorSet, 3> descriptorWrites {
      transformUniform->getDescriptorSet(0, descriptorSets[i], i),
      texture->getDescriptorSet(1, descriptorSets[i]),
      specularMap->getDescriptorSet(4, descriptorSets[i])
    };

    vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(),
                           0, nullptr);
  }
}

void RenderObject::setPosition(const glm::vec3 position)
{
  this->position = position;
}

void RenderObject::setScale(const glm::vec3 scale)
{
  this->scale = scale;
}

void RenderObject::setScale(float scale)
{
  this->scale = { scale, scale, scale };
}

void RenderObject::setRotation(const glm::vec3 rotation)
{
  this->rotation = rotation;
}

void RenderObject::enableRendering()
{
  doRendering = true;
}

void RenderObject::disableRendering()
{
  doRendering = false;
}
