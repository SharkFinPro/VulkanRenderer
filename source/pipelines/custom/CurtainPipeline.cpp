#include "CurtainPipeline.h"
#include <stdexcept>

#include "../Vertex.h"
#include "../RenderPass.h"

#include "../../objects/RenderObject.h"
#include "../../components/Camera.h"
#include "../../objects/UniformBuffer.h"
#include "../../objects/Light.h"

#include <imgui.h>

constexpr int MAX_FRAMES_IN_FLIGHT = 2; // TODO: link this better

CurtainPipeline::CurtainPipeline(const std::shared_ptr<PhysicalDevice>& physicalDevice,
                                 const std::shared_ptr<LogicalDevice>& logicalDevice,
                                 const std::shared_ptr<RenderPass>& renderPass)
  : GraphicsPipeline(physicalDevice, logicalDevice)
{
  createUniforms();

  createDescriptorSetLayouts();

  createDescriptorPool();

  createDescriptorSets();

  createPipeline(renderPass->getRenderPass());
}

CurtainPipeline::~CurtainPipeline()
{
  vkDestroyDescriptorPool(logicalDevice->getDevice(), descriptorPool, nullptr);

  vkDestroyDescriptorSetLayout(logicalDevice->getDevice(), objectDescriptorSetLayout, nullptr);

  vkDestroyDescriptorSetLayout(logicalDevice->getDevice(), globalDescriptorSetLayout, nullptr);
}

VkDescriptorSetLayout& CurtainPipeline::getLayout()
{
  return objectDescriptorSetLayout;
}

void CurtainPipeline::render(const VkCommandBuffer &commandBuffer, const uint32_t currentFrame,
                            const glm::vec3 viewPosition, const glm::mat4& viewMatrix, const VkExtent2D swapChainExtent,
                            const std::vector<std::shared_ptr<Light>>& lights,
                            const std::vector<std::shared_ptr<RenderObject>>& objects)
{
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  const VkViewport viewport {
    .x = 0.0f,
    .y = 0.0f,
    .width = static_cast<float>(swapChainExtent.width),
    .height = static_cast<float>(swapChainExtent.height),
    .minDepth = 0.0f,
    .maxDepth = 1.0f
  };
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  const VkRect2D scissor {
    .offset = {0, 0},
    .extent = swapChainExtent
  };
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  const CameraUniform cameraUBO {
    .position = viewPosition
  };
  cameraUniform->update(currentFrame, &cameraUBO, sizeof(CameraUniform));

  updateLightUniforms(lights, currentFrame);

  ImGui::Begin("Curtain");

  ImGui::SliderFloat("Amplitude", &curtainUBO.amplitude, 0.001f, 3.0f);
  ImGui::SliderFloat("Period", &curtainUBO.period, 0.1f, 10.0f);
  ImGui::SliderFloat("Shininess", &curtainUBO.shininess, 1.0f, 100.0f);

  ImGui::End();
  curtainUniform->update(currentFrame, &curtainUBO, sizeof(CurtainUniform));

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                          &descriptorSets[currentFrame], 0, nullptr);

  for (const auto& object : objects)
  {
    object->updateUniformBuffer(currentFrame, swapChainExtent, viewMatrix);

    object->draw(commandBuffer, pipelineLayout, currentFrame);
  }
}

void CurtainPipeline::loadGraphicsShaders()
{
  createShader("assets/shaders/Curtain.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
  createShader("assets/shaders/Curtain.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
}

void CurtainPipeline::loadGraphicsDescriptorSetLayouts()
{
  loadDescriptorSetLayout(globalDescriptorSetLayout);
  loadDescriptorSetLayout(objectDescriptorSetLayout);
}

std::unique_ptr<VkPipelineColorBlendStateCreateInfo> CurtainPipeline::defineColorBlendState()
{
  colorBlendAttachment = {
    .blendEnable = VK_FALSE,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
  };

  VkPipelineColorBlendStateCreateInfo colorBlendState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments = &colorBlendAttachment,
    .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
  };

  return std::make_unique<VkPipelineColorBlendStateCreateInfo>(colorBlendState);
}

std::unique_ptr<VkPipelineDepthStencilStateCreateInfo> CurtainPipeline::defineDepthStencilState()
{
  VkPipelineDepthStencilStateCreateInfo depthStencilState {
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

std::unique_ptr<VkPipelineDynamicStateCreateInfo> CurtainPipeline::defineDynamicState()
{
  dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };

  VkPipelineDynamicStateCreateInfo dynamicState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
    .pDynamicStates = dynamicStates.data()
  };

  return std::make_unique<VkPipelineDynamicStateCreateInfo>(dynamicState);
}

std::unique_ptr<VkPipelineInputAssemblyStateCreateInfo> CurtainPipeline::defineInputAssemblyState()
{
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE
  };

  return std::make_unique<VkPipelineInputAssemblyStateCreateInfo>(inputAssemblyState);
}

std::unique_ptr<VkPipelineMultisampleStateCreateInfo> CurtainPipeline::defineMultisampleState()
{
  VkPipelineMultisampleStateCreateInfo multisampleState {
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

std::unique_ptr<VkPipelineRasterizationStateCreateInfo> CurtainPipeline::defineRasterizationState()
{
  VkPipelineRasterizationStateCreateInfo rasterizationState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_NONE,
    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .lineWidth = 1.0f
  };

  return std::make_unique<VkPipelineRasterizationStateCreateInfo>(rasterizationState);
}

std::unique_ptr<VkPipelineVertexInputStateCreateInfo> CurtainPipeline::defineVertexInputState()
{
  vertexBindingDescription = Vertex::getBindingDescription();
  vertexAttributeDescriptions = Vertex::getAttributeDescriptions();

  VkPipelineVertexInputStateCreateInfo vertexInputState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &vertexBindingDescription,
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size()),
    .pVertexAttributeDescriptions = vertexAttributeDescriptions.data()
  };

  return std::make_unique<VkPipelineVertexInputStateCreateInfo>(vertexInputState);
}

std::unique_ptr<VkPipelineViewportStateCreateInfo> CurtainPipeline::defineViewportState()
{
  VkPipelineViewportStateCreateInfo viewportState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .scissorCount = 1
  };

  return std::make_unique<VkPipelineViewportStateCreateInfo>(viewportState);
}

void CurtainPipeline::createDescriptorSetLayouts()
{
  createGlobalDescriptorSetLayout();
  createObjectDescriptorSetLayout();
}

void CurtainPipeline::createGlobalDescriptorSetLayout()
{
  constexpr VkDescriptorSetLayoutBinding lightMetadataLayout {
    .binding = 2,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding lightsLayout {
    .binding = 5,
    .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding cameraLayout {
    .binding = 3,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding curtainLayout {
    .binding = 4,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr std::array<VkDescriptorSetLayoutBinding, 4> globalBindings {
    lightMetadataLayout,
    lightsLayout,
    cameraLayout,
    curtainLayout
  };

  const VkDescriptorSetLayoutCreateInfo globalLayoutCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = static_cast<uint32_t>(globalBindings.size()),
    .pBindings = globalBindings.data()
  };

  if (vkCreateDescriptorSetLayout(logicalDevice->getDevice(), &globalLayoutCreateInfo, nullptr, &globalDescriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create global descriptor set layout!");
  }
}

void CurtainPipeline::createObjectDescriptorSetLayout()
{
  constexpr VkDescriptorSetLayoutBinding transformLayout {
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding textureLayout {
    .binding = 1,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr VkDescriptorSetLayoutBinding specularLayout {
    .binding = 4,
    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
  };

  constexpr std::array<VkDescriptorSetLayoutBinding, 3> objectBindings {
    transformLayout,
    textureLayout,
    specularLayout
  };

  const VkDescriptorSetLayoutCreateInfo objectLayoutCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = static_cast<uint32_t>(objectBindings.size()),
    .pBindings = objectBindings.data()
  };

  if (vkCreateDescriptorSetLayout(logicalDevice->getDevice(), &objectLayoutCreateInfo, nullptr,
                                  &objectDescriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create object descriptor set layout!");
  }
}

void CurtainPipeline::createDescriptorPool()
{
  constexpr std::array<VkDescriptorPoolSize, 2> poolSizes {{
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT * 3},
    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_FRAMES_IN_FLIGHT * 1}
  }};

  const VkDescriptorPoolCreateInfo poolCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .maxSets = MAX_FRAMES_IN_FLIGHT,
    .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
    .pPoolSizes = poolSizes.data()
  };

  if (vkCreateDescriptorPool(logicalDevice->getDevice(), &poolCreateInfo, nullptr, &descriptorPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void CurtainPipeline::createDescriptorSets()
{
  const std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, globalDescriptorSetLayout);
  const VkDescriptorSetAllocateInfo allocateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = descriptorPool,
    .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
    .pSetLayouts = layouts.data()
  };

  descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  if (vkAllocateDescriptorSets(logicalDevice->getDevice(), &allocateInfo, descriptorSets.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    std::array<VkWriteDescriptorSet, 3> descriptorWrites{{
      lightMetadataUniform->getDescriptorSet(2, descriptorSets[i], i),
      cameraUniform->getDescriptorSet(3, descriptorSets[i], i),
      curtainUniform->getDescriptorSet(4, descriptorSets[i], i)
    }};

    vkUpdateDescriptorSets(logicalDevice->getDevice(), descriptorWrites.size(),
                           descriptorWrites.data(), 0, nullptr);
  }
}

void CurtainPipeline::createUniforms()
{
  lightMetadataUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, MAX_FRAMES_IN_FLIGHT,
                                                         sizeof(LightMetadataUniform));

  lightsUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, MAX_FRAMES_IN_FLIGHT,
                                                  sizeof(LightUniform));

  cameraUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, MAX_FRAMES_IN_FLIGHT,
                                                  sizeof(CameraUniform));

  curtainUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, MAX_FRAMES_IN_FLIGHT,
                                                   sizeof(CurtainUniform));
}

void CurtainPipeline::updateLightUniforms(const std::vector<std::shared_ptr<Light>>& lights, const uint32_t currentFrame)
{
  if (lights.empty())
  {
    return;
  }

  if (prevNumLights != lights.size())
  {
    logicalDevice->waitIdle();

    const LightMetadataUniform lightMetadataUBO {
      .numLights = static_cast<int>(lights.size())
    };

    lightsUniform.reset();

    lightsUniformBufferSize = sizeof(LightUniform) * lights.size();

    lightsUniform = std::make_unique<UniformBuffer>(logicalDevice, physicalDevice, MAX_FRAMES_IN_FLIGHT,
                                                    lightsUniformBufferSize);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
      lightMetadataUniform->update(i, &lightMetadataUBO, sizeof(lightMetadataUBO));

      auto descriptorSet = lightsUniform->getDescriptorSet(5, descriptorSets[i], i);
      descriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

      vkUpdateDescriptorSets(this->logicalDevice->getDevice(), 1, &descriptorSet, 0, nullptr);
    }

    prevNumLights = static_cast<int>(lights.size());
  }

  std::vector<LightUniform> lightUniforms;
  lightUniforms.resize(lights.size());
  for (int i = 0; i < lights.size(); i++)
  {
    lightUniforms[i] = lights[i]->getUniform();
  }

  lightsUniform->update(currentFrame, lightUniforms.data(), lightsUniformBufferSize);
}
