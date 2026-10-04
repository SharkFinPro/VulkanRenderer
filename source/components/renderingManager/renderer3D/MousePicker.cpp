#include "MousePicker.h"
#include "../../assets/objects/Model.h"
#include "../../assets/objects/RenderObject.h"
#include "../../commandBuffer/SingleUseCommandBuffer.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../pipelines/pipelineManager/PipelineManager.h"
#include "../../../utilities/Buffers.h"
#include "../../../utilities/Images.h"
#include <glm/matrix.hpp>
#include <cmath>
#include <cstring>

namespace vke {

  MousePicker::MousePicker(std::shared_ptr<LogicalDevice> logicalDevice,
                           const vk::CommandPool commandPool)
    : m_logicalDevice(std::move(logicalDevice)), m_commandPool(commandPool)
  {
    constexpr vk::DeviceSize bufferSize = sizeof(uint32_t) * 4;

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
      .meshIndex = renderObject->getModel()->getMeshIndex(m_pick->triangleIndex),
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

  void MousePicker::renderObject(const std::shared_ptr<RenderObject>& renderObject,
                                 bool* mousePicked)
  {
    uint32_t objectID = static_cast<uint32_t>(m_renderObjectsToMousePick.size()) + 1;
    m_renderObjectsToMousePick.emplace_back( renderObject, objectID );
    m_mousePickingFlags.push_back(mousePicked);

    if (mousePicked)
    {
      *mousePicked = false;
    }
  }

  void MousePicker::render(const RenderInfo* renderInfo,
                           const std::shared_ptr<PipelineManager>& pipelineManager) const
  {
    m_viewMatrix = renderInfo->viewMatrix;
    m_projectionMatrix = renderInfo->getProjectionMatrix();

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

  void MousePicker::handleRenderedMousePickingImage(const vk::Image image)
  {
    // Checked even with nothing to pick, so canMousePick() still says whether the cursor is over the scene.
    int32_t mouseX, mouseY;
    if (!validateMousePickingMousePosition(mouseX, mouseY) || m_mousePickingFlags.empty())
    {
      return;
    }

    // An identifier buffer rather than a ray query: it works without ray tracing, matches exactly what was
    // rasterized, and needs only this one-pixel readback.
    const auto [objectID, triangleIndex, depthBits, _] = getPixelFromMousePickingImage(image, mouseX, mouseY);

    if (objectID == 0 || objectID > m_renderObjectsToMousePick.size())
    {
      return;
    }

    if (bool* mousePicked = m_mousePickingFlags[objectID - 1])
    {
      *mousePicked = true;
    }

    float depth;
    std::memcpy(&depth, &depthBits, sizeof(depth));

    m_pick = Pick {
      .renderObject = m_renderObjectsToMousePick[objectID - 1].first,
      .triangleIndex = triangleIndex,
      .worldPosition = getWorldPosition(mouseX, mouseY, depth),
      .depth = depth
    };
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

  glm::vec3 MousePicker::getWorldPosition(const int32_t mouseX,
                                          const int32_t mouseY,
                                          const float depth) const
  {
    // The projection already flips Y for Vulkan, so framebuffer rows map straight to NDC y.
    const glm::vec4 ndc {
      2.0f * (static_cast<float>(mouseX) + 0.5f) / static_cast<float>(m_viewportExtent.width) - 1.0f,
      2.0f * (static_cast<float>(mouseY) + 0.5f) / static_cast<float>(m_viewportExtent.height) - 1.0f,
      depth,
      1.0f
    };

    const glm::vec4 world = glm::inverse(m_projectionMatrix * m_viewMatrix) * ndc;

    return glm::vec3(world) / world.w;
  }

  std::array<uint32_t, 4> MousePicker::getPixelFromMousePickingImage(vk::Image image,
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

    return getPixelFromBuffer(m_stagingBufferMemory);
  }

  std::array<uint32_t, 4> MousePicker::getPixelFromBuffer(const vk::raii::DeviceMemory& stagingBufferMemory)
  {
    std::array<uint32_t, 4> pixel {};

    Buffers::doMappedMemoryOperation(stagingBufferMemory, [&pixel](void* data) {
      std::memcpy(pixel.data(), data, sizeof(pixel));
    });

    return pixel;
  }

  void MousePicker::transitionImageForReading(const SingleUseCommandBuffer& commandBuffer,
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

  void MousePicker::transitionImageForWriting(const SingleUseCommandBuffer& commandBuffer,
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