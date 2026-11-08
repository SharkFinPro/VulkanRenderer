// Headless checks of the placement math behind Window::setFullscreen. No engine, no GPU: ctest runs this.
#include "source/components/window/WindowPlacement.h"
#include <array>
#include <climits>
#include <cstdlib>
#include <iostream>

namespace {

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

  bool equal(const vke::WindowRect& a,
             const vke::WindowRect& b)
  {
    return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
  }

  // Two 1920x1080 monitors side by side, the second to the left of the primary (negative coordinates, as Windows
  // reports a monitor left of the primary), and a 2560x1440 one above the primary.
  constexpr std::array<vke::WindowRect, 3> MONITORS {{
    { 0, 0, 1920, 1080 },
    { -1920, 0, 1920, 1080 },
    { 0, -1440, 2560, 1440 }
  }};

  // The same monitors less a 40 pixel taskbar along their bottom edges.
  constexpr std::array<vke::WindowRect, 3> WORK_AREAS {{
    { 0, 0, 1920, 1040 },
    { -1920, 0, 1920, 1040 },
    { 0, -1440, 2560, 1400 }
  }};

  void checkMonitorChoice()
  {
    check(vke::findMostOverlappedArea({ 100, 100, 800, 600 }, MONITORS) == 0, "a window inside the primary picks it");
    check(vke::findMostOverlappedArea({ -1000, 200, 800, 600 }, MONITORS) == 1, "negative coordinates pick the left monitor");
    check(vke::findMostOverlappedArea({ 100, -1000, 800, 600 }, MONITORS) == 2, "a window above picks the upper monitor");

    // 300 pixels of width on the left monitor, 500 on the primary.
    check(vke::findMostOverlappedArea({ -300, 100, 800, 600 }, MONITORS) == 0, "a straddling window picks the larger share");
    check(vke::findMostOverlappedArea({ -500, 100, 800, 600 }, MONITORS) == 1, "the larger share wins on either side");

    // Exactly half on each: the first listed wins, so the choice doesn't flip between calls.
    check(vke::findMostOverlappedArea({ -400, 100, 800, 600 }, MONITORS) == 0, "equal shares pick the first listed");

    // Touching edges share no pixels.
    check(vke::findMostOverlappedArea({ 1920, 0, 800, 600 }, MONITORS) == std::nullopt, "an edge-adjacent window overlaps nothing");
    check(vke::findMostOverlappedArea({ 5000, 5000, 800, 600 }, MONITORS) == std::nullopt, "an off-screen window overlaps nothing");
    check(vke::findMostOverlappedArea({ 100, 100, 800, 600 }, {}) == std::nullopt, "no monitors overlaps nothing");
    check(vke::findMostOverlappedArea({ 100, 100, 0, 0 }, MONITORS) == std::nullopt, "an empty window overlaps nothing");

    // Extents near the int range must not overflow into a negative or wrapped overlap.
    constexpr std::array<vke::WindowRect, 1> huge {{ { INT_MIN / 2, INT_MIN / 2, INT_MAX, INT_MAX } }};
    check(vke::findMostOverlappedArea({ 0, 0, INT_MAX / 2, INT_MAX / 2 }, huge) == 0, "large extents don't overflow");
  }

  void checkCentering()
  {
    check(equal(vke::centerInArea(800, 600, WORK_AREAS[0]), { 560, 220, 800, 600 }), "centers in the primary work area");
    check(equal(vke::centerInArea(800, 600, WORK_AREAS[1]), { -1360, 220, 800, 600 }), "centers in an offset work area");

    // Too wide and too tall: keeps its size, pinned to the top-left so its title bar stays reachable.
    check(equal(vke::centerInArea(2000, 1200, WORK_AREAS[0]), { 0, 0, 2000, 1200 }), "an oversized window keeps its size at the corner");
    check(equal(vke::centerInArea(2000, 600, WORK_AREAS[2]), { 280, -1040, 2000, 600 }), "each axis centers on its own");
  }

  void checkRestore()
  {
    // The round trip: a placement still on a monitor comes back exactly, wherever it is.
    constexpr vke::WindowRect onPrimary { 137, 91, 800, 600 };
    check(equal(vke::restoreWindowedRect(onPrimary, WORK_AREAS, WORK_AREAS[0]), onPrimary), "restores a placement on the primary");

    constexpr vke::WindowRect onLeft { -1700, 300, 1024, 768 };
    check(equal(vke::restoreWindowedRect(onLeft, WORK_AREAS, WORK_AREAS[0]), onLeft), "restores a placement on another monitor");

    // Partly off screen is how the user left it, so it is kept too.
    constexpr vke::WindowRect straddling { 1700, 900, 800, 600 };
    check(equal(vke::restoreWindowedRect(straddling, WORK_AREAS, WORK_AREAS[0]), straddling), "keeps a partly visible placement");

    // The left monitor was unplugged while fullscreen: the saved size comes back, centered on the fallback.
    constexpr std::array<vke::WindowRect, 1> primaryOnly {{ WORK_AREAS[0] }};
    check(equal(vke::restoreWindowedRect(onLeft, primaryOnly, WORK_AREAS[0]), { 448, 136, 1024, 768 }), "recenters when its monitor is gone");

    check(equal(vke::restoreWindowedRect(onPrimary, {}, WORK_AREAS[2]), { 880, -1040, 800, 600 }), "recenters when no monitor is listed");
  }

} // namespace

int main()
{
  checkMonitorChoice();
  checkCentering();
  checkRestore();

  std::cout << g_checks - g_failures << " of " << g_checks << " checks passed\n";

  return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
