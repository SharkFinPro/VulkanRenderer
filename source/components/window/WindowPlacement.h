#ifndef VKE_WINDOWPLACEMENT_H
#define VKE_WINDOWPLACEMENT_H

#include <cstddef>
#include <optional>
#include <span>

namespace vke {

  // A rectangle in the window system's screen coordinates: a window's client area, a monitor or a monitor's work area.
  struct WindowRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
  };

  // The index of the area in areas that window overlaps most, the first of equal overlaps, or nullopt when it overlaps
  // none of them. Picks the monitor a window should go fullscreen on.
  [[nodiscard]] std::optional<size_t> findMostOverlappedArea(const WindowRect& window,
                                                             std::span<const WindowRect> areas);

  // A width by height rectangle centered in area. A rectangle larger than area keeps its size and is placed at area's
  // top-left corner, so the window's title bar stays on screen.
  [[nodiscard]] WindowRect centerInArea(int width,
                                        int height,
                                        const WindowRect& area);

  // Where a window leaving fullscreen goes: saved, its placement before entering, while it still overlaps one of
  // workAreas; otherwise (its monitor was disconnected or rearranged meanwhile) saved's size centered in fallback.
  [[nodiscard]] WindowRect restoreWindowedRect(const WindowRect& saved,
                                               std::span<const WindowRect> workAreas,
                                               const WindowRect& fallback);

} // namespace vke

#endif //VKE_WINDOWPLACEMENT_H
