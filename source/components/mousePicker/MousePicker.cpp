#include "MousePicker.h"
#include "../commandBuffer/CommandBuffer.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../physicalDevice/PhysicalDevice.h"
#include "../pipelines/implementations/common/PipelineTypes.h"
#include "../renderingManager/ImageResource.h"
#include "../renderingManager/RenderTarget.h"
#include "../renderingManager/legacyRenderer/Framebuffer.h"
#include "../renderingManager/legacyRenderer/RenderPass.h"
#include "../window/Window.h"
#include "../../utilities/Buffers.h"
#include "../../utilities/Images.h"

namespace vke {

  MousePicker::MousePicker(const std::shared_ptr<LogicalDevice>& logicalDevice,
                           const std::shared_ptr<Window>& window,
                           const VkCommandPool& commandPool,
                           VkDescriptorSetLayout objectDescriptorSetLayout)
    : m_logicalDevice(logicalDevice), m_window(window), m_commandPool(commandPool)
  {
    m_mousePickingCommandBuffer = std::make_shared<CommandBuffer>(m_logicalDevice, m_commandPool);

    RenderPassConfig mousePickingRenderPassConfig {
      .imageFormat = VK_FORMAT_R8G8B8A8_UNORM,
      .msaaSamples = VK_SAMPLE_COUNT_1_BIT,
      .finalLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .hasColorAttachment = true,
      .hasDepthAttachment = true,
      .hasResolveAttachment = false
    };
    m_mousePickingRenderPass = std::make_shared<RenderPass>(m_logicalDevice, mousePickingRenderPassConfig);

    m_mousePickingPipeline = std::make_unique<MousePickingPipeline>(m_logicalDevice, m_mousePickingRenderPass,
                                                                    objectDescriptorSetLayout);

    constexpr VkDeviceSize bufferSize = 4;
    Buffers::createBuffer(m_logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          m_stagingBuffer, m_stagingBufferMemory);
  }

  MousePicker::~MousePicker()
  {
    Buffers::destroyBuffer(m_logicalDevice, m_stagingBuffer, m_stagingBufferMemory);
  }

  void MousePicker::clearObjectsToMousePick()
  {
    m_renderObjectsToMousePick.clear();
  }

  void MousePicker::recreateFramebuffer(const VkExtent2D viewportExtent)
  {
    m_viewportExtent = viewportExtent;

    m_renderTarget.reset();

    ImageResourceConfig imageResourceConfig {
      .logicalDevice = m_logicalDevice,
      .extent = m_viewportExtent,
      .commandPool = m_commandPool,
      .colorFormat = VK_FORMAT_R8G8B8A8_UNORM,
      .depthFormat = m_logicalDevice->getPhysicalDevice()->findDepthFormat(),
      .numSamples = VK_SAMPLE_COUNT_1_BIT
    };

    m_renderTarget = std::make_shared<RenderTarget>(imageResourceConfig);

    m_mousePickingFramebuffer.reset();

    m_mousePickingFramebuffer = std::make_shared<Framebuffer>(
      m_logicalDevice,
      m_renderTarget,
      m_mousePickingRenderPass,
      m_viewportExtent
    );
  }

  void MousePicker::doMousePicking(const uint32_t imageIndex,
                                   const uint32_t currentFrame,
                                   const glm::vec3 viewPosition,
                                   const glm::mat4& viewMatrix,
                                   std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>>& renderObjectsToRender)
  {
    if (m_renderObjectsToMousePick.empty())
    {
      return;
    }

    int32_t mouseX, mouseY;
    if (!validateMousePickingMousePosition(mouseX, mouseY))
    {
      return;
    }

    m_logicalDevice->resetMousePickingFences(currentFrame);

    m_mousePickingCommandBuffer->setCurrentFrame(currentFrame);
    m_mousePickingCommandBuffer->resetCommandBuffer();
    recordMousePickingCommandBuffer(imageIndex, currentFrame, viewPosition, viewMatrix);
    m_logicalDevice->submitMousePickingGraphicsQueue(currentFrame, m_mousePickingCommandBuffer->getCommandBuffer());
    m_logicalDevice->waitForMousePickingFences(currentFrame);

    const auto objectID = getIDFromMousePickingFramebuffer(mouseX, mouseY);

    if (objectID == 0)
    {
      return;
    }

    handleMousePickingResult(objectID, renderObjectsToRender);
  }

  bool MousePicker::canMousePick() const
  {
    return m_canMousePick;
  }

  void MousePicker::renderObject(const std::shared_ptr<RenderObject>& renderObject,
                                 bool* mousePicked)
  {
    uint32_t objectID = static_cast<uint32_t>(m_renderObjectsToMousePick.size()) + 1;
    m_renderObjectsToMousePick.emplace_back( renderObject, objectID );
    m_mousePickingItems[objectID] = mousePicked;
    *mousePicked = false;
  }

  void MousePicker::setViewportPos(const ImVec2 viewportPos)
  {
    m_viewportPos = viewportPos;
  }

  void MousePicker::recordMousePickingCommandBuffer(const uint32_t imageIndex,
                                                    uint32_t currentFrame,
                                                    const glm::vec3 viewPosition,
                                                    const glm::mat4& viewMatrix) const
  {
    m_mousePickingCommandBuffer->record([this, imageIndex, currentFrame, viewPosition, viewMatrix]()
    {
      if (m_renderObjectsToMousePick.empty())
      {
        return;
      }

      const RenderInfo renderInfo {
        .commandBuffer = m_mousePickingCommandBuffer,
        .currentFrame = currentFrame,
        .viewPosition = viewPosition,
        .viewMatrix = viewMatrix,
        .extent = m_viewportExtent
      };

      m_mousePickingRenderPass->begin(m_mousePickingFramebuffer->getFramebuffer(imageIndex), m_viewportExtent, renderInfo.commandBuffer);

      const VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(renderInfo.extent.width),
        .height = static_cast<float>(renderInfo.extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
      };
      renderInfo.commandBuffer->setViewport(viewport);

      const VkRect2D scissor = {
        .offset = {0, 0},
        .extent = renderInfo.extent
      };
      renderInfo.commandBuffer->setScissor(scissor);

      m_mousePickingPipeline->render(&renderInfo, &m_renderObjectsToMousePick);

      m_mousePickingCommandBuffer->endRenderPass();
    });
  }

  bool MousePicker::validateMousePickingMousePosition(int32_t& mouseX,
                                                      int32_t& mouseY)
  {
    if (m_viewportExtent.width == 0 || m_viewportExtent.height == 0)
    {
      m_canMousePick = false;
    }
    else
    {
      double mouseXPos, mouseYPos;
      m_window->getCursorPos(mouseXPos, mouseYPos);
      mouseX = static_cast<int32_t>(mouseXPos);
      mouseY = static_cast<int32_t>(mouseYPos);

      mouseX -= static_cast<int32_t>(m_viewportPos.x);
      mouseY -= static_cast<int32_t>(m_viewportPos.y);

      m_canMousePick = !(mouseX < 0 || mouseX > m_viewportExtent.width - 1 ||
                         mouseY < 0 || mouseY > m_viewportExtent.height - 1);
    }

    return m_canMousePick;
  }

  uint32_t MousePicker::getIDFromMousePickingFramebuffer(const int32_t mouseX,
                                                         const int32_t mouseY) const
  {
    VkCommandBuffer commandBuffer = Buffers::beginSingleTimeCommands(m_logicalDevice, m_commandPool);

    const VkImageMemoryBarrier barrier {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = m_renderTarget->getColorImageResource(0).getImage(),
      .subresourceRange {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
      }
    };

    vkCmdPipelineBarrier(
      commandBuffer,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      0,
      0, nullptr,
      0, nullptr,
      1, &barrier
    );

    Images::copyImageToBuffer(m_renderTarget->getColorImageResource(0).getImage(), { mouseX, mouseY, 0 },
                              { 1, 1, 1 }, commandBuffer, m_stagingBuffer);

    const VkImageMemoryBarrier barrierBack {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = m_renderTarget->getColorImageResource(0).getImage(),
      .subresourceRange {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
      }
    };

    vkCmdPipelineBarrier(
      commandBuffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      0,
      0, nullptr,
      0, nullptr,
      1, &barrierBack
    );

    Buffers::endSingleTimeCommands(m_logicalDevice, m_commandPool, m_logicalDevice->getGraphicsQueue(), commandBuffer);

    const uint32_t objectID = getObjectIDFromBuffer(m_stagingBufferMemory);

    return objectID;
  }

  uint32_t MousePicker::getObjectIDFromBuffer(VkDeviceMemory stagingBufferMemory) const
  {
    uint32_t objectID = 0;

    m_logicalDevice->doMappedMemoryOperation(stagingBufferMemory, [&objectID](void* data) {
      const uint8_t* pixel = static_cast<uint8_t*>(data);

      objectID = pixel[0] << 16 | pixel[1] << 8 | pixel[2];
    });

    return objectID;
  }

  void MousePicker::handleMousePickingResult(const uint32_t objectID,
                                             std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>>& renderObjectsToRender)
  {
    *m_mousePickingItems.at(objectID) = true;

    for (auto& [object, id] : m_renderObjectsToMousePick)
    {
      if (id == objectID)
      {
        renderObjectsToRender[PipelineType::objectHighlight].push_back(object);
        break;
      }
    }
  }

} // namespace vke