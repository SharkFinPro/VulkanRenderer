#include "MousePicker.h"
#include "../../assets/objects/RenderObject.h"
#include "../../commandBuffer/SingleUseCommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../pipelines/pipelineManager/PipelineManager.h"
#include "../../window/Window.h"
#include "../../../utilities/Buffers.h"
#include "../../../utilities/Images.h"

namespace vke {

  MousePicker::MousePicker(std::shared_ptr<LogicalDevice> logicalDevice,
                           std::shared_ptr<Window> window,
                           const vk::CommandPool commandPool)
    : m_logicalDevice(std::move(logicalDevice)), m_window(std::move(window)), m_commandPool(commandPool)
  {
    constexpr vk::DeviceSize bufferSize = 4;

    Buffers::createBuffer(
      m_logicalDevice,
      bufferSize,
      vk::BufferUsageFlagBits::eTransferDst,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
      m_stagingBuffer,
      m_stagingBufferMemory
    );
  }

  bool MousePicker::canMousePick() const
  {
    return m_canMousePick;
  }

  void MousePicker::clearObjectsToMousePick()
  {
    m_renderObjectsToMousePick.clear();
  }

  void MousePicker::setViewportExtent(const vk::Extent2D viewportExtent)
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
    pipelineManager->bindGraphicsPipeline(renderInfo->commandBuffer, PipelineType::mousePicking);

    for (const auto& [object, id] : m_renderObjectsToMousePick)
    {
      pipelineManager->pushGraphicsPipelineConstants<uint32_t>(
        renderInfo->commandBuffer,
        PipelineType::mousePicking,
        vk::ShaderStageFlagBits::eFragment,
        0,
        id
      );

      pipelineManager->bindGraphicsPipelineDescriptorSet(
        renderInfo->commandBuffer,
        PipelineType::mousePicking,
        object->getDescriptorSet(renderInfo->currentFrame),
        0
      );

      object->updateUniformBuffer(renderInfo->currentFrame, renderInfo->viewMatrix, renderInfo->getProjectionMatrix());

      object->draw(renderInfo->commandBuffer);
    }
  }

  void MousePicker::handleRenderedMousePickingImage(vk::Image image)
  {
	  if (m_mousePickingItems.empty())
	  {
	    return;
	  }
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

  uint32_t MousePicker::getIDFromMousePickingImage(vk::Image image,
                                                   const int32_t mouseX,
                                                   const int32_t mouseY) const
  {
    const auto commandBuffer = SingleUseCommandBuffer(m_logicalDevice, m_commandPool, m_logicalDevice->getGraphicsQueue());

    commandBuffer.record([this, &commandBuffer, image, mouseX, mouseY] {
      transitionImageForReading(commandBuffer, image);

      Images::copyImageToBuffer(
        image,
        { mouseX, mouseY, 0 },
        { 1, 1, 1 },
        commandBuffer,
        m_stagingBuffer
      );

      transitionImageForWriting(commandBuffer, image);
    });

    return getObjectIDFromBuffer(m_stagingBufferMemory);
  }

  uint32_t MousePicker::getObjectIDFromBuffer(const vk::raii::DeviceMemory& stagingBufferMemory) const
  {
    uint32_t objectID = 0;

    Buffers::doMappedMemoryOperation(stagingBufferMemory, [&objectID](void* data) {
      const uint8_t* pixel = static_cast<uint8_t*>(data);

      objectID = static_cast<uint32_t>(pixel[0]) << 16 |
                 static_cast<uint32_t>(pixel[1]) << 8 |
                 static_cast<uint32_t>(pixel[2]);
    });

    return objectID;
  }

  void MousePicker::transitionImageForReading(const SingleUseCommandBuffer& commandBuffer,
                                              vk::Image image)
  {
    const vk::ImageMemoryBarrier imageMemoryBarrier {
      .srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
      .dstAccessMask = vk::AccessFlagBits::eTransferRead,
      .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .newLayout = vk::ImageLayout::eTransferSrcOptimal,
      .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
      .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
      .image = image,
      .subresourceRange {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
      }
    };

    commandBuffer.pipelineBarrier(
      vk::PipelineStageFlagBits::eColorAttachmentOutput,
      vk::PipelineStageFlagBits::eTransfer,
      {},
      {},
      {},
      { imageMemoryBarrier }
    );
  }

  void MousePicker::transitionImageForWriting(const SingleUseCommandBuffer& commandBuffer,
                                              vk::Image image)
  {
    const vk::ImageMemoryBarrier imageMemoryBarrier {
      .srcAccessMask = vk::AccessFlagBits::eTransferRead,
      .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
      .oldLayout = vk::ImageLayout::eTransferSrcOptimal,
      .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
      .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
      .image = image,
      .subresourceRange {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
      }
    };

    commandBuffer.pipelineBarrier(
      vk::PipelineStageFlagBits::eTransfer,
      vk::PipelineStageFlagBits::eColorAttachmentOutput,
      {},
      {},
      {},
      { imageMemoryBarrier }
    );
  }
} // namespace vke