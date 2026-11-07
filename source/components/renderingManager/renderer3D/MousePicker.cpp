#include "MousePicker.h"
#include "../../assets/objects/Material.h"
#include "../../assets/objects/RenderObject.h"
#include "../../commandBuffer/CommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../pipelines/pipelineManager/PipelineManager.h"
#include "../../../utilities/Buffers.h"
#include <glm/matrix.hpp>
#include <cmath>
#include <cstring>

namespace vke {

  MousePicker::MousePicker(std::shared_ptr<LogicalDevice> logicalDevice)
    : m_logicalDevice(std::move(logicalDevice))
  {
    createReadbackSlots();
  }

  void MousePicker::createReadbackSlots()
  {
    constexpr vk::DeviceSize bufferSize = sizeof(uint32_t) * 4;

    const uint32_t slotCount = m_logicalDevice->getMaxFramesInFlight();
    m_readbackSlots.resize(slotCount);

    for (auto& slot : m_readbackSlots)
    {
      Buffers::createBuffer(
        m_logicalDevice,
        bufferSize,
        vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        slot.buffer,
        slot.memory
      );

      // Mapped for the slot's lifetime; freeing the memory unmaps it.
      slot.mapped = slot.memory.mapMemory(0, bufferSize, vk::MemoryMapFlags{});
    }
  }

  bool MousePicker::canMousePick() const
  {
    return m_canMousePick;
  }

  std::optional<PickResult> MousePicker::getPickResult() const
  {
    if (!m_pick)
    {
      return std::nullopt;
    }

    auto renderObject = m_pick->renderObject.lock();
    if (!renderObject)
    {
      return std::nullopt;
    }

    return PickResult {
      .renderObject = renderObject,
      .triangleIndex = m_pick->triangleIndex,
      .worldPosition = m_pick->worldPosition,
      .depth = m_pick->depth
    };
  }

  void MousePicker::beginFrame()
  {
    m_canMousePick = false;
    m_pick.reset();
  }

  void MousePicker::clearObjectsToMousePick()
  {
    m_renderObjectsToMousePick.clear();

    // Ids restart at 1 each frame, and the pointers belong to the application, which may free them once it stops
    // submitting an object.
    m_mousePickingFlags.clear();
  }

  void MousePicker::setViewportExtent(const vk::Extent2D viewportExtent)
  {
    m_viewportExtent = viewportExtent;
  }

  void MousePicker::setViewportPos(const ImVec2 viewportPos)
  {
    m_viewportPos = viewportPos;
  }

  void MousePicker::setViewportDisplaySize(const ImVec2 viewportDisplaySize)
  {
    m_viewportDisplaySize = viewportDisplaySize;
  }

  void MousePicker::setSceneHovered(const bool sceneHovered)
  {
    m_sceneHovered = sceneHovered;
  }

  uint32_t MousePicker::renderObject(const std::shared_ptr<RenderObject>& renderObject,
                                     bool* mousePicked,
                                     const bool alphaTested,
                                     const uint32_t slot)
  {
    const auto objectID = static_cast<uint32_t>(m_renderObjectsToMousePick.size()) + 1;
    m_renderObjectsToMousePick.push_back({ renderObject, slot, alphaTested });
    m_mousePickingFlags.push_back(mousePicked);

    if (mousePicked)
    {
      *mousePicked = false;
    }

    return objectID;
  }

  void MousePicker::render(const RenderInfo* renderInfo,
                           const std::shared_ptr<PipelineManager>& pipelineManager,
                           const vk::DescriptorSet sceneDescriptorSet) const
  {
    m_viewMatrix = renderInfo->viewMatrix;
    m_projectionMatrix = renderInfo->getProjectionMatrix();

    renderEntries(renderInfo, pipelineManager, sceneDescriptorSet, false);
    renderEntries(renderInfo, pipelineManager, sceneDescriptorSet, true);
  }

  void MousePicker::renderEntries(const RenderInfo* renderInfo,
                                  const std::shared_ptr<PipelineManager>& pipelineManager,
                                  const vk::DescriptorSet sceneDescriptorSet,
                                  const bool alphaTested) const
  {
    const auto pipelineType = alphaTested ? PipelineType::mousePickingAlphaTested : PipelineType::mousePicking;
    bool bound = false;

    // Only alpha-tested picking samples a texture, so only it needs a material set
    const Material* boundMaterial = nullptr;

    for (const auto& [object, slot, entryAlphaTested] : m_renderObjectsToMousePick)
    {
      if (entryAlphaTested != alphaTested)
      {
        continue;
      }

      if (!bound)
      {
        pipelineManager->bindGraphicsPipeline(renderInfo->commandBuffer, pipelineType);

        pipelineManager->bindGraphicsPipelineDescriptorSet(renderInfo->commandBuffer, pipelineType, sceneDescriptorSet, 0);

        bound = true;
      }

      if (alphaTested && object->getMaterial().get() != boundMaterial)
      {
        boundMaterial = object->getMaterial().get();

        pipelineManager->bindGraphicsPipelineDescriptorSet(
          renderInfo->commandBuffer,
          pipelineType,
          boundMaterial->getDescriptorSet(renderInfo->currentFrame),
          1
        );
      }

      object->draw(renderInfo->commandBuffer, slot);
    }
  }


  void MousePicker::resolveReadback(const uint32_t currentFrame)
  {
    auto& slot = m_readbackSlots[currentFrame];

    if (!slot.pending)
    {
      return;
    }

    slot.pending = false;

    std::array<uint32_t, 4> pixel {};
    std::memcpy(pixel.data(), slot.mapped, sizeof(pixel));

    storePick(slot, pixel);

    // Drops the objects; the vector keeps its capacity for the next swap.
    slot.entries.clear();
  }

  void MousePicker::storePick(const ReadbackSlot& slot,
                              const std::array<uint32_t, 4>& pixel)
  {
    const auto [objectID, triangleIndex, depthBits, _] = pixel;

    if (objectID == 0 || objectID > slot.entries.size())
    {
      return;
    }

    float depth;
    std::memcpy(&depth, &depthBits, sizeof(depth));

    const auto& object = slot.entries[objectID - 1].object;

    m_pick = Pick {
      .renderObject = object,
      .triangleIndex = triangleIndex,
      .worldPosition = getWorldPosition(slot, depth),
      .depth = depth
    };

    flagCurrentSubmissions(object.get());
  }

  void MousePicker::flagCurrentSubmissions(const RenderObject* picked)
  {
    // The ids belong to an earlier frame, so match this frame's submissions by object.
    for (size_t i = 0; i < m_renderObjectsToMousePick.size(); i++)
    {
      if (m_renderObjectsToMousePick[i].object.get() == picked && m_mousePickingFlags[i])
      {
        *m_mousePickingFlags[i] = true;
      }
    }
  }

  void MousePicker::recordReadback(const std::shared_ptr<CommandBuffer>& commandBuffer,
                                   const vk::Image image,
                                   const uint32_t currentFrame)
  {
    // Checked even with nothing to pick, so canMousePick() still says whether the cursor is over the scene.
    int32_t mouseX, mouseY;
    if (!validateMousePickingMousePosition(mouseX, mouseY) || m_renderObjectsToMousePick.empty())
    {
      return;
    }

    auto& slot = m_readbackSlots[currentFrame];

    slot.pending = true;
    slot.mouseX = mouseX;
    slot.mouseY = mouseY;
    slot.extent = m_viewportExtent;
    slot.viewMatrix = m_viewMatrix;
    slot.projectionMatrix = m_projectionMatrix;

    // The slot's previous entries were resolved and cleared, so this hands back an empty vector with capacity.
    slot.entries.swap(m_renderObjectsToMousePick);

    // An identifier buffer rather than a ray query: it works without ray tracing and matches exactly what was
    // rasterized.
    recordPixelCopy(*commandBuffer, image, slot);
  }

  bool MousePicker::validateMousePickingMousePosition(int32_t& mouseX,
                                                      int32_t& mouseY)
  {
    const ImVec2 mousePos = ImGui::GetIO().MousePos;

    // ImGui reports an invalid position when no mouse is available/positioned (e.g. this frame's window lost the
    // mouse). The cursor can also lie within the scene image while another window covers it there.
    if (!m_sceneHovered || m_viewportExtent.width == 0 || m_viewportExtent.height == 0 ||
        m_viewportDisplaySize.x <= 0.0f || m_viewportDisplaySize.y <= 0.0f || !ImGui::IsMousePosValid(&mousePos))
    {
      m_canMousePick = false;
    }
    else
    {
      // Both mousePos and m_viewportPos are absolute screen coordinates, so this holds whichever OS window (main or
      // a detached viewport) the cursor and the scene image are actually in. The offset is then scaled from the size
      // the image is displayed at to the picking image's pixels, which differ on high-density displays.
      const float scaleX = static_cast<float>(m_viewportExtent.width) / m_viewportDisplaySize.x;
      const float scaleY = static_cast<float>(m_viewportExtent.height) / m_viewportDisplaySize.y;

      const float pixelX = std::floor((mousePos.x - m_viewportPos.x) * scaleX);
      const float pixelY = std::floor((mousePos.y - m_viewportPos.y) * scaleY);

      // Checked as floats so a cursor far outside the image can't overflow the conversion to a pixel index.
      m_canMousePick = pixelX >= 0.0f && pixelX < static_cast<float>(m_viewportExtent.width) &&
                       pixelY >= 0.0f && pixelY < static_cast<float>(m_viewportExtent.height);

      if (m_canMousePick)
      {
        mouseX = static_cast<int32_t>(pixelX);
        mouseY = static_cast<int32_t>(pixelY);
      }
    }

    return m_canMousePick;
  }

  glm::vec3 MousePicker::getWorldPosition(const ReadbackSlot& slot,
                                          const float depth)
  {
    // The projection already flips Y for Vulkan, so framebuffer rows map straight to NDC y.
    const glm::vec4 ndc {
      2.0f * (static_cast<float>(slot.mouseX) + 0.5f) / static_cast<float>(slot.extent.width) - 1.0f,
      2.0f * (static_cast<float>(slot.mouseY) + 0.5f) / static_cast<float>(slot.extent.height) - 1.0f,
      depth,
      1.0f
    };

    const glm::vec4 world = glm::inverse(slot.projectionMatrix * slot.viewMatrix) * ndc;

    return glm::vec3(world) / world.w;
  }

  void MousePicker::recordPixelCopy(const CommandBuffer& commandBuffer,
                                    const vk::Image image,
                                    const ReadbackSlot& slot)
  {
    transitionImageForReading(commandBuffer, image);

    const vk::BufferImageCopy region {
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .mipLevel = 0,
        .baseArrayLayer = 0,
        .layerCount = 1
      },
      .imageOffset = { slot.mouseX, slot.mouseY, 0 },
      .imageExtent = { 1, 1, 1 }
    };

    commandBuffer.copyImageToBuffer(image, vk::ImageLayout::eTransferSrcOptimal, *slot.buffer, region);

    transitionImageForWriting(commandBuffer, image);

    makeReadbackVisibleToHost(commandBuffer, *slot.buffer);
  }

  void MousePicker::makeReadbackVisibleToHost(const CommandBuffer& commandBuffer,
                                              const vk::Buffer buffer)
  {
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

    commandBuffer.pipelineBarrier(dependencyInfo);
  }

  void MousePicker::transitionImageForReading(const CommandBuffer& commandBuffer,
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

    commandBuffer.pipelineBarrier(dependencyInfo);
  }

  void MousePicker::transitionImageForWriting(const CommandBuffer& commandBuffer,
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

    commandBuffer.pipelineBarrier(dependencyInfo);
  }
} // namespace vke