#include "GraphicsPipeline.h"
#include <stdexcept>
#include <fstream>

#include "Vertex.h"
#include "RenderPass.h"
#include "Uniforms.h"

#include "../objects/RenderObject.h"
#include "../components/Camera.h"
#include "../objects/UniformBuffer.h"

#include <imgui.h>

const int MAX_FRAMES_IN_FLIGHT = 2; // TODO: link this better

static std::vector<char> readFile(const std::string& filename)
{
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open())
  {
    throw std::runtime_error("failed to open file!");
  }

  size_t fileSize = (size_t) file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();

  return buffer;
}

GraphicsPipeline::GraphicsPipeline(VkDevice& device, VkPhysicalDevice& physicalDevice, const char* vertexShader,
                                   const char* fragmentShader, VkExtent2D& swapChainExtent,
                                   VkSampleCountFlagBits msaaSamples, std::shared_ptr<RenderPass> renderPass)
  : device(device), physicalDevice(physicalDevice), swapChainExtent(swapChainExtent),
    lightUniform(std::make_unique<UniformBuffer>(device, physicalDevice, MAX_FRAMES_IN_FLIGHT, sizeof(LightUniform))),
    cameraUniform(std::make_unique<UniformBuffer>(device, physicalDevice, MAX_FRAMES_IN_FLIGHT, sizeof(CameraUniform))),
    color{1, 1, 1}, ambient(0.2f), diffuse(0.5f)
{
  createDescriptorSetLayout();

  createGraphicsPipeline(vertexShader, fragmentShader, msaaSamples, renderPass);

  createDescriptorPool();

  createDescriptorSets();
}

GraphicsPipeline::~GraphicsPipeline()
{
  vkDestroyDescriptorPool(device, descriptorPool, nullptr);

  vkDestroyDescriptorSetLayout(device, objectDescriptorSetLayout, nullptr);

  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

  vkDestroyPipeline(device, graphicsPipeline, nullptr);

  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
}

VkDescriptorSetLayout& GraphicsPipeline::getLayout()
{
  return objectDescriptorSetLayout;
}

void GraphicsPipeline::render(VkCommandBuffer& commandBuffer, uint32_t currentFrame,std::shared_ptr<Camera> camera)
{
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

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
  ImGui::Text("Control Light Color:");
  ImGui::ColorEdit3("Color", color);
  ImGui::SliderFloat("Ambient", &ambient, 0.0f, 1.0f);
  ImGui::SliderFloat("Diffuse", &diffuse, 0.0f, 1.0f);
  ImGui::End();

  LightUniform lightUBO{};
  lightUBO.position = {0, 3.0f, 0};
  lightUBO.color = {color[0], color[1], color[2]};
  lightUBO.ambient = ambient;
  lightUBO.diffuse = diffuse;
  lightUBO.specular = 1.0f;
  lightUniform->update(currentFrame, &lightUBO, sizeof(LightUniform));

  CameraUniform cameraUBO{};
  cameraUBO.position = camera->getPosition();
  cameraUniform->update(currentFrame, &cameraUBO, sizeof(CameraUniform));

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

  for (auto& object : renderObjects)
  {
    object->updateUniformBuffer(currentFrame, swapChainExtent, camera);

    object->draw(commandBuffer, pipelineLayout, currentFrame);
  }
}

void GraphicsPipeline::insertRenderObject(std::shared_ptr<RenderObject>& renderObject)
{
  renderObjects.push_back(renderObject);
}

void GraphicsPipeline::createGraphicsPipeline(const char* vertexShader, const char* fragmentShader,
                                              VkSampleCountFlagBits msaaSamples, std::shared_ptr<RenderPass>& renderPass)
{
  VkShaderModule vertShaderModule = createShaderModule(vertexShader);
  VkShaderModule fragShaderModule = createShaderModule(fragmentShader);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  auto bindingDescription = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  std::vector<VkDynamicState> dynamicStates = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };

  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasClamp = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f;
  rasterizer.depthBiasClamp = 0.0f;
  rasterizer.depthBiasSlopeFactor = 0.0f;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = msaaSamples;
  multisampling.minSampleShading = 1.0f;
  multisampling.pSampleMask = nullptr;
  multisampling.alphaToCoverageEnable = VK_FALSE;
  multisampling.alphaToOneEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  std::array<VkDescriptorSetLayout, 2> layouts = {descriptorSetLayout, objectDescriptorSetLayout};
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
  pipelineLayoutInfo.pSetLayouts = layouts.data();
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = nullptr;

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create pipeline layout!");
  }

  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.minDepthBounds = 0.0f;
  depthStencil.maxDepthBounds = 1.0f;
  depthStencil.stencilTestEnable = VK_FALSE;
  depthStencil.front = {};
  depthStencil.back = {};

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass->getRenderPass();
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  vkDestroyShaderModule(device, fragShaderModule, nullptr);
  vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

VkShaderModule GraphicsPipeline::createShaderModule(const char* file)
{
  auto code = readFile(file);

  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create shader module!");
  }

  return shaderModule;
}

void GraphicsPipeline::createDescriptorSetLayout()
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

  std::array<VkDescriptorSetLayoutBinding, 2> globalBindings = {lightLayout, cameraLayout};

  VkDescriptorSetLayoutCreateInfo globalLayoutCreateInfo{};
  globalLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  globalLayoutCreateInfo.bindingCount = static_cast<uint32_t>(globalBindings.size());
  globalLayoutCreateInfo.pBindings = globalBindings.data();

  if (vkCreateDescriptorSetLayout(device, &globalLayoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
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


  std::array<VkDescriptorSetLayoutBinding, 3> objectBindings = {transformLayout, textureLayout, specularLayout};

  VkDescriptorSetLayoutCreateInfo objectLayoutCreateInfo{};
  objectLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  objectLayoutCreateInfo.bindingCount = static_cast<uint32_t>(objectBindings.size());
  objectLayoutCreateInfo.pBindings = objectBindings.data();

  if (vkCreateDescriptorSetLayout(device, &objectLayoutCreateInfo, nullptr, &objectDescriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}

void GraphicsPipeline::createDescriptorPool()
{
  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  poolSizes[0] = lightUniform->getDescriptorPoolSize();
  poolSizes[1] = cameraUniform->getDescriptorPoolSize();

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

void GraphicsPipeline::createDescriptorSets()
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
    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
    descriptorWrites[0] = lightUniform->getDescriptorSet(2, descriptorSets[i], i);
    descriptorWrites[1] = cameraUniform->getDescriptorSet(3, descriptorSets[i], i);

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
  }
}
