// Headless checks of the present mode fallback orders. No engine, no GPU: ctest runs this.
#include "source/components/window/PresentMode.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

  using vke::PresentMode;
  using Vk = vk::PresentModeKHR;

  int g_checks = 0;
  int g_failures = 0;

  void check(const bool condition,
             const char* name)
  {
    ++g_checks;

    if (!condition)
    {
      ++g_failures;
      std::cout << "FAIL: " << name << '\n';
    }
  }

  constexpr std::array<PresentMode, 4> ALL_MODES { PresentMode::fifo, PresentMode::fifoRelaxed, PresentMode::mailbox,
                                                   PresentMode::immediate };

  constexpr std::array<Vk, 4> ALL_AVAILABLE { Vk::eFifo, Vk::eFifoRelaxed, Vk::eMailbox, Vk::eImmediate };

  void checkChoice()
  {
    for (const PresentMode mode : ALL_MODES)
    {
      check(vke::choosePresentMode(mode, ALL_AVAILABLE) == vke::toVulkanPresentMode(mode), "a preference with everything available returns itself");

      constexpr std::array<Vk, 1> fifoOnly { Vk::eFifo };
      check(vke::choosePresentMode(mode, fifoOnly) == Vk::eFifo, "a preference with only FIFO returns FIFO");

      check(vke::choosePresentMode(mode, {}) == Vk::eFifo, "an empty list returns FIFO");

      constexpr std::array<Vk, 2> shared { Vk::eSharedDemandRefresh, Vk::eSharedContinuousRefresh };
      check(vke::choosePresentMode(mode, shared) == Vk::eFifo, "only shared refresh modes returns FIFO");
    }

    constexpr std::array<Vk, 2> noImmediate { Vk::eFifo, Vk::eMailbox };
    check(vke::choosePresentMode(PresentMode::immediate, noImmediate) == Vk::eMailbox, "immediate falls to mailbox");

    constexpr std::array<Vk, 2> noMailbox { Vk::eFifo, Vk::eImmediate };
    check(vke::choosePresentMode(PresentMode::mailbox, noMailbox) == Vk::eFifo, "mailbox does not fall to immediate");
    check(vke::choosePresentMode(PresentMode::immediate, noMailbox) == Vk::eImmediate, "immediate is taken when listed");

    constexpr std::array<Vk, 2> noRelaxed { Vk::eMailbox, Vk::eImmediate };
    check(vke::choosePresentMode(PresentMode::fifoRelaxed, noRelaxed) == Vk::eFifo, "fifoRelaxed falls to FIFO");

    constexpr std::array<Vk, 4> reversed { Vk::eImmediate, Vk::eMailbox, Vk::eFifoRelaxed, Vk::eFifo };
    check(vke::choosePresentMode(PresentMode::immediate, reversed) == Vk::eImmediate, "list order doesn't matter for immediate");
    check(vke::choosePresentMode(PresentMode::mailbox, reversed) == Vk::eMailbox, "list order doesn't matter for mailbox");
    check(vke::choosePresentMode(PresentMode::fifo, reversed) == Vk::eFifo, "list order doesn't matter for fifo");

    constexpr std::array<Vk, 2> noFifoListed { Vk::eImmediate, Vk::eMailbox };
    check(vke::choosePresentMode(PresentMode::fifo, noFifoListed) == Vk::eFifo, "fifo is returned although unlisted");
  }

  void checkSupported()
  {
    check(vke::supportedPresentModes(ALL_AVAILABLE) == std::vector<PresentMode>(ALL_MODES.begin(), ALL_MODES.end()), "everything is listed in enum order");

    constexpr std::array<Vk, 3> shuffled { Vk::eImmediate, Vk::eFifoRelaxed, Vk::eFifo };
    check(vke::supportedPresentModes(shuffled) == std::vector<PresentMode>({ PresentMode::fifo, PresentMode::fifoRelaxed, PresentMode::immediate }), "enum order regardless of list order");

    check(vke::supportedPresentModes({}) == std::vector<PresentMode>({ PresentMode::fifo }), "fifo is included when nothing is listed");

    constexpr std::array<Vk, 5> noisy { Vk::eMailbox, Vk::eMailbox, Vk::eSharedDemandRefresh, Vk::eFifo, Vk::eFifo };
    check(vke::supportedPresentModes(noisy) == std::vector<PresentMode>({ PresentMode::fifo, PresentMode::mailbox }), "duplicates and unknown modes are dropped");
  }

  void checkConversion()
  {
    for (const PresentMode mode : ALL_MODES)
    {
      check(vke::toPresentMode(vke::toVulkanPresentMode(mode)) == mode, "modes round-trip");
    }

    check(vke::toPresentMode(Vk::eSharedDemandRefresh) == PresentMode::fifo, "shared demand refresh maps to fifo");
    check(vke::toPresentMode(Vk::eSharedContinuousRefresh) == PresentMode::fifo, "shared continuous refresh maps to fifo");
  }

} // namespace

int main()
{
  checkChoice();
  checkSupported();
  checkConversion();

  std::cout << g_checks - g_failures << " of " << g_checks << " checks passed\n";

  return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
