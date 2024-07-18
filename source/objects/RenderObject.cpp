#include "RenderObject.h"
#include <stdexcept>
#include <chrono>
#include <utility>
#include "glm/gtc/matrix_transform.hpp"
#include "../components/Camera.h"

#include "Model.h"
#include "Texture.h"
#include "UniformBuffer.h"

const int MAX_FRAMES_IN_FLIGHT = 2; // TODO: link this better

RenderObject::RenderObject(VkDevice& device, VkPhysicalDevice& physicalDevice,
                           VkDescriptorSetLayout& descriptorSetLayout, std::shared_ptr<Texture> texture,
                           std::shared_ptr<Texture> specularMap, std::shared_ptr<Model> model)
  : device(device), physicalDevice(physicalDevice), descriptorSetLayout(descriptorSetLayout),
    texture(std::move(texture)), specularMap(std::move(specularMap)), model(std::move(model)), position(0, 0, 0),
    transformUniform(std::make_unique<UniformBuffer>(device, physicalDevice, MAX_FRAMES_IN_FLIGHT, sizeof(TransformUniform)))
{
  createDescriptorPool();
  createDescriptorSets();
}

RenderObject::~RenderObject()
{
  vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void RenderObject::draw(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t currentFrame)
{
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &descriptorSets[currentFrame], 0, nullptr);

  model->draw(commandBuffer);
}

void RenderObject::updateUniformBuffer(uint32_t currentFrame, VkExtent2D& swapChainExtent, std::shared_ptr<Camera>& camera)
{
  TransformUniform transformUBO{};

  transformUBO.model = glm::translate(glm::mat4(1.0f), position);

  transformUBO.view = camera->getViewMatrix();
  transformUBO.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height), 0.1f, 100.0f);
  transformUBO.proj[1][1] *= -1;
  transformUniform->update(currentFrame, &transformUBO, sizeof(TransformUniform));
}

void RenderObject::createDescriptorPool()
{
  std::array<VkDescriptorPoolSize, 3> poolSizes{};
  poolSizes[0] = transformUniform->getDescriptorPoolSize();
  poolSizes[1] = texture->getDescriptorPoolSize(MAX_FRAMES_IN_FLIGHT);
  poolSizes[2] = specularMap->getDescriptorPoolSize(MAX_FRAMES_IN_FLIGHT);

  VkDescriptorPoolCreateInfo poolCreateInfo{};
  poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolCreateInfo.pPoolSizes = poolSizes.data();
  poolCreateInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  if (vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &descriptorPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void RenderObject::createDescriptorSets()
{
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
  VkDescriptorSetAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.descriptorPool = descriptorPool;
  allocateInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  allocateInfo.pSetLayouts = layouts.data();

  descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  if (vkAllocateDescriptorSets(device, &allocateInfo, descriptorSets.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
    descriptorWrites[0] = transformUniform->getDescriptorSet(0, descriptorSets[i], i);
    descriptorWrites[1] = texture->getDescriptorSet(1, descriptorSets[i]);
    descriptorWrites[2] = specularMap->getDescriptorSet(4, descriptorSets[i]);

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }
}

void RenderObject::setPosition(glm::vec3 position_)
{
  position = position_;
}