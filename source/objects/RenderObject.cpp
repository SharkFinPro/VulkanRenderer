#include "RenderObject.h"
#include <stdexcept>
#include <chrono>
#include <utility>
#include "glm/gtc/matrix_transform.hpp"
#include "../Camera.h"

#include "Model.h"
#include "Texture.h"
#include "UniformBuffer.h"

const int MAX_FRAMES_IN_FLIGHT = 2; // TODO: link this better

RenderObject::RenderObject(VkDevice& device, VkPhysicalDevice& physicalDevice,
                           VkDescriptorSetLayout& descriptorSetLayout, std::shared_ptr<Texture> texture,
                           std::shared_ptr<Model> model)
  : device(device), physicalDevice(physicalDevice), texture(std::move(texture)), model(std::move(model)),
    position(0, 0, 0),
    uniformBuffer(std::make_unique<UniformBuffer>(device, physicalDevice, MAX_FRAMES_IN_FLIGHT, sizeof(TransformUniform)))
{
  createDescriptorPool();
  createDescriptorSets(descriptorSetLayout);
}

RenderObject::~RenderObject()
{
  vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void RenderObject::draw(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, uint32_t currentFrame)
{
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

  model->draw(commandBuffer);
}

void RenderObject::updateUniformBuffer(uint32_t currentFrame, VkExtent2D& swapChainExtent, std::shared_ptr<Camera>& camera)
{
  static auto startTime = std::chrono::high_resolution_clock::now();

  auto currentTime = std::chrono::high_resolution_clock::now();
  float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

  TransformUniform ubo{};

  ubo.model = glm::translate(glm::mat4(1.0f), position);
  ubo.model = glm::rotate(ubo.model, glm::radians(90.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
  ubo.model = glm::rotate(ubo.model, time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

  ubo.view = camera->getViewMatrix();
  ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height), 0.1f, 100.0f);
  ubo.proj[1][1] *= -1;

  uniformBuffer->update(currentFrame, &ubo, sizeof(TransformUniform));
}

void RenderObject::createDescriptorPool()
{
  std::vector<VkDescriptorPoolSize> poolSizes{};
  poolSizes.push_back(uniformBuffer->getDescriptorPoolSize());
  poolSizes.push_back(texture->getDescriptorPoolSize(MAX_FRAMES_IN_FLIGHT));

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

void RenderObject::createDescriptorSets(VkDescriptorSetLayout& descriptorSetLayout)
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
    std::vector<VkWriteDescriptorSet> descriptorWrites{};
    descriptorWrites.push_back(uniformBuffer->getDescriptorSet(0, descriptorSets[i], i));
    descriptorWrites.push_back(texture->getDescriptorSet(1, descriptorSets[i]));

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }
}

void RenderObject::setPosition(glm::vec3 position_)
{
  position = position_;
}