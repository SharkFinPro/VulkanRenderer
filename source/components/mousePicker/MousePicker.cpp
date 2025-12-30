#include "MousePicker.h"
#include "../commandBuffer/CommandBuffer.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../physicalDevice/PhysicalDevice.h"
#include "../pipelines/implementations/common/PipelineTypes.h"
#include "../renderingManager/ImageResource.h"
#include "../renderingManager/RenderTarget.h"
#include "../window/Window.h"
#include "../../utilities/Buffers.h"
#include "../../utilities/Images.h"

namespace vke {

  MousePicker::MousePicker(std::shared_ptr<LogicalDevice> logicalDevice,
                           std::shared_ptr<Window> window,
                           const VkCommandPool& commandPool,
                           VkDescriptorSetLayout objectDescriptorSetLayout)
    : m_logicalDevice(std::move(logicalDevice)), m_window(std::move(window)), m_commandPool(commandPool)
  {
    // m_mousePickingPipeline = std::make_unique<MousePickingPipeline>(m_logicalDevice, m_mousePickingRenderPass,
    //                                                                 objectDescriptorSetLayout);

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
  }

  void MousePicker::doMousePicking(std::unordered_map<PipelineType, std::vector<std::shared_ptr<RenderObject>>>& renderObjectsToRender)
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
    return 0;
    // const auto commandBuffer = Buffers::beginSingleTimeCommands(m_logicalDevice, m_commandPool);
    //
    // transitionImageForReading(commandBuffer);
    //
    // Images::copyImageToBuffer(
    //   m_renderTarget->getColorImageResource(0).getImage(),
    //   { mouseX, mouseY, 0 },
    //   { 1, 1, 1 },
    //   commandBuffer,
    //   m_stagingBuffer
    // );
    //
    // transitionImageForWriting(commandBuffer);
    //
    // Buffers::endSingleTimeCommands(
    //   m_logicalDevice,
    //   m_commandPool,
    //   m_logicalDevice->getGraphicsQueue(),
    //   commandBuffer
    // );
    //
    // return getObjectIDFromBuffer(m_stagingBufferMemory);
  }

  void MousePicker::transitionImageForReading(VkCommandBuffer commandBuffer) const
  {
    // const VkImageMemoryBarrier barrier {
    //   .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    //   .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    //   .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
    //   .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    //   .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    //   .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    //   .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    //   .image = m_renderTarget->getColorImageResource(0).getImage(),
    //   .subresourceRange {
    //     .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    //     .baseMipLevel = 0,
    //     .levelCount = 1,
    //     .baseArrayLayer = 0,
    //     .layerCount = 1
    //   }
    // };
    //
    // vkCmdPipelineBarrier(
    //   commandBuffer,
    //   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    //   VK_PIPELINE_STAGE_TRANSFER_BIT,
    //   0,
    //   0, nullptr,
    //   0, nullptr,
    //   1, &barrier
    // );
  }

  void MousePicker::transitionImageForWriting(VkCommandBuffer commandBuffer) const
  {
    // const VkImageMemoryBarrier barrierBack {
    //   .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    //   .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
    //   .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    //   .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    //   .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    //   .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    //   .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    //   .image = m_renderTarget->getColorImageResource(0).getImage(),
    //   .subresourceRange {
    //     .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    //     .baseMipLevel = 0,
    //     .levelCount = 1,
    //     .baseArrayLayer = 0,
    //     .layerCount = 1
    //   }
    // };
    //
    // vkCmdPipelineBarrier(
    //   commandBuffer,
    //   VK_PIPELINE_STAGE_TRANSFER_BIT,
    //   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    //   0,
    //   0, nullptr,
    //   0, nullptr,
    //   1, &barrierBack
    // );
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