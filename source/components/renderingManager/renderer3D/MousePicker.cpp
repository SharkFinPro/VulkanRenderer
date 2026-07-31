#include "MousePicker.h"
#include "../../assets/objects/RenderObject.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../pipelines/pipelineManager/PipelineManager.h"
#include "../../window/Window.h"
#include "../../../utilities/Buffers.h"
#include "../../../utilities/Images.h"

namespace vke {

  MousePicker::MousePicker(std::shared_ptr<LogicalDevice> logicalDevice,
                           std::shared_ptr<Window> window)
    : m_logicalDevice(std::move(logicalDevice)), m_window(std::move(window))
  {
    createReadbackSlots();
  }

  bool MousePicker::canMousePick() const
  {
    return m_canMousePick;
  }

  void MousePicker::clearObjectsToMousePick()
  {
    m_renderObjectsToMousePick.clear();

    // The registered bools belong to the caller and only stay valid for the frame that
    // registered them, so the mapping cannot outlive the frame either.
    m_mousePickingItems.clear();
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

  void MousePicker::resolveReadback(const uint32_t currentFrame)
  {
    auto& slot = m_readbackSlots.at(currentFrame);

    const bool hasResult = slot.pending;
    slot.pending = false;

    // Refreshes m_canMousePick for this frame; a cursor that has left the viewport drops the
    // stale result rather than letting it linger for another maxFramesInFlight frames.
    int32_t mouseX, mouseY;
    if (!validateMousePickingMousePosition(mouseX, mouseY) || !hasResult)
    {
      return;
    }

    const auto objectID = getObjectIDFromBuffer(slot.mapped);

    if (objectID == 0)
    {
      return;
    }

    // The ID was assigned maxFramesInFlight frames ago, so it may not correspond to anything
    // registered this frame.
    if (const auto it = m_mousePickingItems.find(objectID); it != m_mousePickingItems.end())
    {
      *it->second = true;
    }
  }

  void MousePicker::recordReadback(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                   const uint32_t currentFrame,
                                   const vk::Image image)
  {
    auto& slot = m_readbackSlots.at(currentFrame);
    slot.pending = false;

    int32_t mouseX, mouseY;
    if (!validateMousePickingMousePosition(mouseX, mouseY))
    {
      // No copy recorded, so the image stays in the layout the next picking pass expects.
      return;
    }

    transitionImageForReading(commandBuffer, image);

    Images::copyImageToBuffer(
      image,
      { mouseX, mouseY, 0 },
      { 1, 1, 1 },
      *commandBuffer,
      slot.buffer
    );

    transitionImageForWriting(commandBuffer, image);

    barrierForHostRead(commandBuffer, slot.buffer);

    slot.pending = true;
  }

  void MousePicker::createReadbackSlots()
  {
    constexpr vk::DeviceSize bufferSize = 4;

    const auto maxFramesInFlight = m_logicalDevice->getMaxFramesInFlight();

    m_readbackSlots.reserve(maxFramesInFlight);

    for (uint32_t i = 0; i < maxFramesInFlight; ++i)
    {
      auto& slot = m_readbackSlots.emplace_back();

      Buffers::createBuffer(
        m_logicalDevice,
        bufferSize,
        vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        slot.buffer,
        slot.memory
      );

      // Mapped for the lifetime of the picker: a slot is only read after its frame's timeline
      // wait, so there is nothing for a per-frame map/unmap to guard against.
      slot.mapped = slot.memory.mapMemory(0, vk::WholeSize);
    }
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

  uint32_t MousePicker::getObjectIDFromBuffer(const void* mappedMemory)
  {
    const auto* pixel = static_cast<const uint8_t*>(mappedMemory);

    return static_cast<uint32_t>(pixel[0]) << 16 |
           static_cast<uint32_t>(pixel[1]) << 8 |
           static_cast<uint32_t>(pixel[2]);
  }

  void MousePicker::transitionImageForReading(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                              const vk::Image image)
  {
    const vk::ImageMemoryBarrier2 imageMemoryBarrier {
      .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
      .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
      .dstAccessMask = vk::AccessFlagBits2::eTransferRead,
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

    const vk::DependencyInfo dependencyInfo {
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &imageMemoryBarrier
    };

    commandBuffer->pipelineBarrier(dependencyInfo);
  }

  void MousePicker::transitionImageForWriting(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                              const vk::Image image)
  {
    const vk::ImageMemoryBarrier2 imageMemoryBarrier {
      .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
      .srcAccessMask = vk::AccessFlagBits2::eTransferRead,
      .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
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

    const vk::DependencyInfo dependencyInfo {
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &imageMemoryBarrier
    };

    commandBuffer->pipelineBarrier(dependencyInfo);
  }

  void MousePicker::barrierForHostRead(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                       const vk::Buffer buffer)
  {
    // Makes the copy visible to the host read that resolveReadback() performs once this frame's
    // timeline value has been reached.
    const vk::BufferMemoryBarrier2 bufferMemoryBarrier {
      .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
      .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
      .dstStageMask = vk::PipelineStageFlagBits2::eHost,
      .dstAccessMask = vk::AccessFlagBits2::eHostRead,
      .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
      .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
      .buffer = buffer,
      .offset = 0,
      .size = vk::WholeSize
    };

    const vk::DependencyInfo dependencyInfo {
      .bufferMemoryBarrierCount = 1,
      .pBufferMemoryBarriers = &bufferMemoryBarrier
    };

    commandBuffer->pipelineBarrier(dependencyInfo);
  }
} // namespace vke
