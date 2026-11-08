#ifndef VKE_PRESENTMODE_H
#define VKE_PRESENTMODE_H

#include <vulkan/vulkan.hpp>
#include <span>
#include <vector>

namespace vke {

  enum class PresentMode {
    // Vsync on, always supported; waits for the vertical blank.
    fifo,
    // Vsync, but a late frame is shown at once and may tear.
    fifoRelaxed,
    // No tearing; the newest frame replaces a queued one and presenting does not block.
    mailbox,
    // Vsync off; may tear, does not block.
    immediate
  };

  // Resolves a preference against the modes the surface offers. Fallback orders:
  //   fifo        -> FIFO
  //   fifoRelaxed -> FIFO_RELAXED, FIFO
  //   mailbox     -> MAILBOX, FIFO
  //   immediate   -> IMMEDIATE, MAILBOX, FIFO
  // FIFO is the last resort even when not listed, because the specification guarantees it.
  [[nodiscard]] vk::PresentModeKHR choosePresentMode(PresentMode preference,
                                                     std::span<const vk::PresentModeKHR> available);

  // Any mode the engine never selects (the shared refresh modes) maps to fifo.
  [[nodiscard]] PresentMode toPresentMode(vk::PresentModeKHR mode);

  [[nodiscard]] vk::PresentModeKHR toVulkanPresentMode(PresentMode mode);

  // The modes natively available, in enum order, always including fifo.
  [[nodiscard]] std::vector<PresentMode> supportedPresentModes(std::span<const vk::PresentModeKHR> available);

  [[nodiscard]] const char* toString(PresentMode mode);

} // namespace vke

#endif //VKE_PRESENTMODE_H
