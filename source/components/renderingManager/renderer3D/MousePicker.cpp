#include "MousePicker.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../pipelines/implementations/common/PipelineTypes.h"
#include "../../pipelines/pipelineManager/PipelineManager.h"
#include "../../window/Window.h"
#include "../../../utilities/Buffers.h"
#include "../../../utilities/Images.h"

namespace vke {

  MousePicker::MousePicker(std::shared_ptr<LogicalDevice> logicalDevice,
                           std::shared_ptr<Window> window,
                           const VkCommandPool& commandPool)
    : m_logicalDevice(std::move(logicalDevice)), m_window(std::move(window)), m_commandPool(commandPool)
  {
    constexpr VkDeviceSize bufferSize = 4;
    Buffers::createBuffer(m_logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          m_stagingBuffer, m_stagingBufferMemory);
  }

  MousePicker::~MousePicker()
  {
    Buffers::destroyBuffer(m_logicalDevice, m_stagingBuffer, m_stagingBufferMemory);
  }

  bool MousePicker::canMousePick() const
  {
    return m_canMousePick;
  }

  void MousePicker::clearObjectsToMousePick()
  {
    m_renderObjectsToMousePick.clear();
  }

  void MousePicker::setViewportExtent(const VkExtent2D viewportExtent)
  {
    m_viewportExtent = viewportExtent;
  }

  void MousePicker::setViewportPos(const ImVec2 viewportPos)
  {
    m_viewportPos = viewportPos;
  }

  void MousePicker::renderObject(const std::shared_ptr<RenderObject>& renderObject,
                                 bool* mousePicked)
  {
    uint32_t objectID = static_cast<uint32_t>(m_renderObjectsToMousePick.size()) + 1;
    m_renderObjectsToMousePick.emplace_back( renderObject, objectID );
    m_mousePickingItems[objectID] = mousePicked;
    *mousePicked = false;
  }

  void MousePicker::render(const RenderInfo* renderInfo,
                           const std::shared_ptr<PipelineManager>& pipelineManager) const
  {
    pipelineManager->renderMousePickingPipeline(renderInfo, &m_renderObjectsToMousePick);
  }

  void MousePicker::handleRenderedMousePickingImage(VkImage image)
  {
    int32_t mouseX, mouseY;
    if (!validateMousePickingMousePosition(mouseX, mouseY))
    {
      return;
    }

    const auto objectID = getIDFromMousePickingImage(image, mouseX, mouseY);

    if (objectID == 0)
    {
      return;
    }

    *m_mousePickingItems.at(objectID) = true;
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

  uint32_t MousePicker::getIDFromMousePickingImage(VkImage image,
                                                   const int32_t mouseX,
                                                   const int32_t mouseY) const
  {
    const auto commandBuffer = Buffers::beginSingleTimeCommands(m_logicalDevice, m_commandPool);

    transitionImageForReading(commandBuffer, image);

    Images::copyImageToBuffer(
      image,
      { mouseX, mouseY, 0 },
      { 1, 1, 1 },
      commandBuffer,
      m_stagingBuffer
    );

    transitionImageForWriting(commandBuffer, image);

    Buffers::endSingleTimeCommands(
      m_logicalDevice,
      m_commandPool,
      m_logicalDevice->getGraphicsQueue(),
      commandBuffer
    );

    return getObjectIDFromBuffer(m_stagingBufferMemory);
  }

  uint32_t MousePicker::getObjectIDFromBuffer(VkDeviceMemory stagingBufferMemory) const
  {
    uint32_t objectID = 0;

    m_logicalDevice->doMappedMemoryOperation(stagingBufferMemory, [&objectID](void* data) {
      const uint8_t* pixel = static_cast<uint8_t*>(data);

      objectID = static_cast<uint32_t>(pixel[0]) << 16 |
                 static_cast<uint32_t>(pixel[1]) << 8 |
                 static_cast<uint32_t>(pixel[2]);
    });

    return objectID;
  }

  void MousePicker::transitionImageForReading(VkCommandBuffer commandBuffer,
                                              VkImage image)
  {
    const VkImageMemoryBarrier barrier {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
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
  }

  void MousePicker::transitionImageForWriting(VkCommandBuffer commandBuffer,
                                              VkImage image)
  {
    const VkImageMemoryBarrier barrierBack {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
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
  }
} // namespace vke