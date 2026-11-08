#include "PresentMode.h"
#include <algorithm>
#include <array>

namespace vke {

  namespace {

    bool contains(const std::span<const vk::PresentModeKHR> available,
                  const vk::PresentModeKHR mode)
    {
      return std::ranges::find(available, mode) != available.end();
    }

  } // namespace

  vk::PresentModeKHR choosePresentMode(const PresentMode preference,
                                       const std::span<const vk::PresentModeKHR> available)
  {
    std::array<vk::PresentModeKHR, 3> order{};
    size_t count = 0;

    switch (preference)
    {
      case PresentMode::fifo:
        break;
      case PresentMode::fifoRelaxed:
        order[count++] = vk::PresentModeKHR::eFifoRelaxed;
        break;
      case PresentMode::mailbox:
        order[count++] = vk::PresentModeKHR::eMailbox;
        break;
      case PresentMode::immediate:
        order[count++] = vk::PresentModeKHR::eImmediate;
        order[count++] = vk::PresentModeKHR::eMailbox;
        break;
    }

    for (size_t i = 0; i < count; ++i)
    {
      if (contains(available, order[i]))
      {
        return order[i];
      }
    }

    return vk::PresentModeKHR::eFifo;
  }

  PresentMode toPresentMode(const vk::PresentModeKHR mode)
  {
    switch (mode)
    {
      case vk::PresentModeKHR::eFifoRelaxed:
        return PresentMode::fifoRelaxed;
      case vk::PresentModeKHR::eMailbox:
        return PresentMode::mailbox;
      case vk::PresentModeKHR::eImmediate:
        return PresentMode::immediate;
      default:
        return PresentMode::fifo;
    }
  }

  vk::PresentModeKHR toVulkanPresentMode(const PresentMode mode)
  {
    switch (mode)
    {
      case PresentMode::fifoRelaxed:
        return vk::PresentModeKHR::eFifoRelaxed;
      case PresentMode::mailbox:
        return vk::PresentModeKHR::eMailbox;
      case PresentMode::immediate:
        return vk::PresentModeKHR::eImmediate;
      case PresentMode::fifo:
        break;
    }

    return vk::PresentModeKHR::eFifo;
  }

  std::vector<PresentMode> supportedPresentModes(const std::span<const vk::PresentModeKHR> available)
  {
    std::vector<PresentMode> modes;
    modes.push_back(PresentMode::fifo);

    for (const PresentMode mode : { PresentMode::fifoRelaxed, PresentMode::mailbox, PresentMode::immediate })
    {
      if (contains(available, toVulkanPresentMode(mode)))
      {
        modes.push_back(mode);
      }
    }

    return modes;
  }

  const char* toString(const PresentMode mode)
  {
    switch (mode)
    {
      case PresentMode::fifo:
        return "FIFO (vsync)";
      case PresentMode::fifoRelaxed:
        return "FIFO relaxed";
      case PresentMode::mailbox:
        return "Mailbox";
      case PresentMode::immediate:
        return "Immediate (no vsync)";
    }

    return "Unknown";
  }

} // namespace vke
