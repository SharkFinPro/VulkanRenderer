#include "ObjectsPipeline.h"
#include <stdexcept>
#include <utility>

#include "../Vertex.h"
#include "../RenderPass.h"
#include "Uniforms.h"

#include "../../objects/RenderObject.h"
#include "../../components/Camera.h"
#include "../../objects/UniformBuffer.h"

#include <imgui.h>

constexpr int MAX_FRAMES_IN_FLIGHT = 2; // TODO: link this better

ObjectsPipeline::ObjectsPipeline(std::shared_ptr<PhysicalDevice> physicalDevice, std::shared_ptr<LogicalDevice> logicalDevice,
                  const std::shared_ptr<RenderPass>& renderPass)
  : GraphicsPipeline(std::move(physicalDevice), std::move(logicalDevice)),
    position{0, 3, 0}, color{1, 1, 1}, ambient(0.2f), diffuse(0.5f)
{
  createUniforms();

  createDescriptorSetLayout();

  createDescriptorPool();

  createDescriptorSets();

  createPipeline(renderPass->getRenderPass());
}

ObjectsPipeline::~ObjectsPipeline()
{
  vkDestroyDescriptorPool(logicalDevice->getDevice(), descriptorPool, nullptr);

  vkDestroyDescriptorSetLayout(logicalDevice->getDevice(), objectDescriptorSetLayout, nullptr);

  vkDestroyDescriptorSetLayout(logicalDevice->getDevice(), descriptorSetLayout, nullptr);
}

VkDescriptorSetLayout& ObjectsPipeline::getLayout()
{
  return objectDescriptorSetLayout;
}

void ObjectsPipeline::render(const VkCommandBuffer& commandBuffer, const uint32_t currentFrame,
                              const std::shared_ptr<Camera>& camera, const VkExtent2D swapChainExtent)
{
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(swapChainExtent.width);
  viewport.height = static_cast<float>(swapChainExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapChainExtent;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  ImGui::Begin("Colors");
  ImGui::Text("Control Light:");
  ImGui::ColorEdit3("Color", color);
  ImGui::SliderFloat("Ambient", &ambient, 0.0f, 1.0f);
  ImGui::SliderFloat("Diffuse", &diffuse, 0.0f, 1.0f);
  ImGui::SliderFloat3("Position", position, -50.0f, 50.0f);
  ImGui::End();

  Light light{};
  light.position = {position[0], position[1], position[2]};
  light.color = {color[0], color[1], color[2]};
  light.ambient = ambient / 2.0f;
  light.diffuse = diffuse;
  light.specular = 1.0f;

  Light light2{};
  light2.position = {5, -3.5f, 5};
  light2.color = {1, 1, 0};
  light2.ambient = ambient / 2.0f;
  light2.diffuse = diffuse;
  light2.specular = 1.0f;

  Light light3{};
  light3.position = {-5.0f, -3.6f, -5.0f};
  light3.color = {0.5, 0.5, 1};
  light3.ambient = ambient / 2.0f;
  light3.diffuse = diffuse;
  light3.specular = 1.0f;

  constexpr int numLights = 3;

  LightUniform lightUBO{};
  lightUBO.numLights = numLights;
  lightUBO.lights[0] = light;
  lightUBO.lights[1] = light2;
  lightUBO.lights[2] = light3;

  lightUniform->update(currentFrame, &lightUBO, sizeof(lightUBO));

  CameraUniform cameraUBO{};
  cameraUBO.position = camera->getPosition();
  cameraUniform->update(currentFrame, &cameraUBO, sizeof(CameraUniform));

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

  for (const auto& object : renderObjects)
  {
    object->updateUniformBuffer(currentFrame, swapChainExtent, camera);

    object->draw(commandBuffer, pipelineLayout, currentFrame);
  }
}

void ObjectsPipeline::insertRenderObject(const std::shared_ptr<RenderObject>& renderObject)
{
  renderObjects.push_back(renderObject);
}

void ObjectsPipeline::loadShaders()
{
  createShader("assets/shaders/objects.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/shaders/objects.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void ObjectsPipeline::loadDescriptorSetLayouts()
{
  loadDescriptorSetLayout(descriptorSetLayout);
  loadDescriptorSetLayout(objectDescriptorSetLayout);
}

std::unique_ptr<VkPipelineColorBlendStateCreateInfo> ObjectsPipeline::defineColorBlendState()
{
  colorBlendAttachment = {
    .blendEnable = VK_FALSE,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
  };

  VkPipelineColorBlendStateCreateInfo colorBlendState = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments = &colorBlendAttachment,
    .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
  };

  return std::make_unique<VkPipelineColorBlendStateCreateInfo>(colorBlendState);
}

std::unique_ptr<VkPipelineDepthStencilStateCreateInfo> ObjectsPipeline::defineDepthStencilState()
{
  VkPipelineDepthStencilStateCreateInfo depthStencilState = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .depthTestEnable = VK_TRUE,
    .depthWriteEnable = VK_TRUE,
    .depthCompareOp = VK_COMPARE_OP_LESS,
    .depthBoundsTestEnable = VK_FALSE,
    .stencilTestEnable = VK_FALSE,
    .minDepthBounds = 0.0f,
    .maxDepthBounds = 1.0f
  };

  return std::make_unique<VkPipelineDepthStencilStateCreateInfo>(depthStencilState);
}

std::unique_ptr<VkPipelineDynamicStateCreateInfo> ObjectsPipeline::defineDynamicState()
{
  dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };

  VkPipelineDynamicStateCreateInfo dynamicState = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
    .pDynamicStates = dynamicStates.data()
  };

  return std::make_unique<VkPipelineDynamicStateCreateInfo>(dynamicState);
}

std::unique_ptr<VkPipelineInputAssemblyStateCreateInfo> ObjectsPipeline::defineInputAssemblyState()
{
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE
  };

  return std::make_unique<VkPipelineInputAssemblyStateCreateInfo>(inputAssemblyState);
}

std::unique_ptr<VkPipelineMultisampleStateCreateInfo> ObjectsPipeline::defineMultisampleState()
{
  VkPipelineMultisampleStateCreateInfo multisampleState = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples = physicalDevice->getMsaaSamples(),
    .sampleShadingEnable = VK_FALSE,
    .minSampleShading = 1.0f,
    .pSampleMask = VK_NULL_HANDLE,
    .alphaToCoverageEnable = VK_FALSE,
    .alphaToOneEnable = VK_FALSE
  };

  return std::make_unique<VkPipelineMultisampleStateCreateInfo>(multisampleState);
}

std::unique_ptr<VkPipelineRasterizationStateCreateInfo> ObjectsPipeline::defineRasterizationState()
{
  VkPipelineRasterizationStateCreateInfo rasterizationState = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .lineWidth = 1.0f
  };

  return std::make_unique<VkPipelineRasterizationStateCreateInfo>(rasterizationState);
}

std::unique_ptr<VkPipelineVertexInputStateCreateInfo> ObjectsPipeline::defineVertexInputState()
{
  vertexBindingDescription = Vertex::getBindingDescription();
  vertexAttributeDescriptions = Vertex::getAttributeDescriptions();

  VkPipelineVertexInputStateCreateInfo vertexInputState = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &vertexBindingDescription,
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size()),
    .pVertexAttributeDescriptions = vertexAttributeDescriptions.data()
  };

  return std::make_unique<VkPipelineVertexInputStateCreateInfo>(vertexInputState);
}

std::unique_ptr<VkPipelineViewportStateCreateInfo> ObjectsPipeline::defineViewportState()
{
  VkPipelineViewportStateCreateInfo viewportState = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .scissorCount = 1
  };

  return std::make_unique<VkPipelineViewportStateCreateInfo>(viewportState);
}

void ObjectsPipeline::createDescriptorSetLayout()
{
  VkDescriptorSetLayoutBinding lightLayout{};
  lightLayout.binding = 2;
  lightLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  lightLayout.descriptorCount = 1;
  lightLayout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  lightLayout.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutBinding cameraLayout{};
  cameraLayout.binding = 3;
  cameraLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  cameraLayout.descriptorCount = 1;
  cameraLayout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  cameraLayout.pImmutableSamplers = nullptr;

  const std::array<VkDescriptorSetLayoutBinding, 2> globalBindings = {lightLayout, cameraLayout};

  VkDescriptorSetLayoutCreateInfo globalLayoutCreateInfo{};
  globalLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  globalLayoutCreateInfo.bindingCount = static_cast<uint32_t>(globalBindings.size());
  globalLayoutCreateInfo.pBindings = globalBindings.data();

  if (vkCreateDescriptorSetLayout(logicalDevice->getDevice(), &globalLayoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor set layout!");
  }

  VkDescriptorSetLayoutBinding transformLayout{};
  transformLayout.binding = 0;
  transformLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  transformLayout.descriptorCount = 1;
  transformLayout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  transformLayout.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutBinding textureLayout{};
  textureLayout.binding = 1;
  textureLayout.descriptorCount = 1;
  textureLayout.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  textureLayout.pImmutableSamplers = nullptr;
  textureLayout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutBinding specularLayout{};
  specularLayout.binding = 4;
  specularLayout.descriptorCount = 1;
  specularLayout.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  specularLayout.pImmutableSamplers = nullptr;
  specularLayout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


  const std::array<VkDescriptorSetLayoutBinding, 3> objectBindings = {
    transformLayout,
    textureLayout,
    specularLayout
  };

  VkDescriptorSetLayoutCreateInfo objectLayoutCreateInfo{};
  objectLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  objectLayoutCreateInfo.bindingCount = static_cast<uint32_t>(objectBindings.size());
  objectLayoutCreateInfo.pBindings = objectBindings.data();

  if (vkCreateDescriptorSetLayout(logicalDevice->getDevice(), &objectLayoutCreateInfo, nullptr,
                                  &objectDescriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}

void ObjectsPipeline::createDescriptorPool()
{
  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  poolSizes[0] = lightUniform->getDescriptorPoolSize();
  poolSizes[1] = cameraUniform->getDescriptorPoolSize();

  VkDescriptorPoolCreateInfo poolCreateInfo{};
  poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolCreateInfo.pPoolSizes = poolSizes.data();
  poolCreateInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  if (vkCreateDescriptorPool(logicalDevice->getDevice(), &poolCreateInfo, nullptr,
                             &descriptorPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void ObjectsPipeline::createDescriptorSets()
{
  const std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
  VkDescriptorSetAllocateInfo allocateInfo{};
  allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocateInfo.descriptorPool = descriptorPool;
  allocateInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  allocateInfo.pSetLayouts = layouts.data();

  descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  if (vkAllocateDescriptorSets(logicalDevice->getDevice(), &allocateInfo,
                               descriptorSets.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
    descriptorWrites[0] = lightUniform->getDescriptorSet(2, descriptorSets[i], i);
    descriptorWrites[1] = cameraUniform->getDescriptorSet(3, descriptorSets[i], i);

    vkUpdateDescriptorSets(logicalDevice->getDevice(), descriptorWrites.size(),
                           descriptorWrites.data(), 0, nullptr);
  }
}

void ObjectsPipeline::createUniforms()
{
  lightUniform = std::make_unique<UniformBuffer>(logicalDevice->getDevice(),
    physicalDevice->getPhysicalDevice(), MAX_FRAMES_IN_FLIGHT, sizeof(LightUniform));

  cameraUniform = std::make_unique<UniformBuffer>(logicalDevice->getDevice(),
    physicalDevice->getPhysicalDevice(), MAX_FRAMES_IN_FLIGHT, sizeof(CameraUniform));
}
