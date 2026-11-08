#include "WindowPlacement.h"
#include <algorithm>
#include <cstdint>

namespace vke {

  namespace {

    int64_t overlapArea(const WindowRect& a,
                        const WindowRect& b)
    {
      // 64-bit, so coordinates near the int range don't overflow the extents
      const int64_t left = std::max<int64_t>(a.x, b.x);
      const int64_t top = std::max<int64_t>(a.y, b.y);
      const int64_t right = std::min(static_cast<int64_t>(a.x) + a.width, static_cast<int64_t>(b.x) + b.width);
      const int64_t bottom = std::min(static_cast<int64_t>(a.y) + a.height, static_cast<int64_t>(b.y) + b.height);

      if (right <= left || bottom <= top)
      {
        return 0;
      }

      return (right - left) * (bottom - top);
    }

  } // namespace

  std::optional<size_t> findMostOverlappedArea(const WindowRect& window,
                                               const std::span<const WindowRect> areas)
  {
    std::optional<size_t> best;
    int64_t bestOverlap = 0;

    for (size_t i = 0; i < areas.size(); ++i)
    {
      if (const int64_t overlap = overlapArea(window, areas[i]); overlap > bestOverlap)
      {
        best = i;
        bestOverlap = overlap;
      }
    }

    return best;
  }

  WindowRect centerInArea(const int width,
                          const int height,
                          const WindowRect& area)
  {
    return {
      .x = area.x + std::max(0, (area.width - width) / 2),
      .y = area.y + std::max(0, (area.height - height) / 2),
      .width = width,
      .height = height
    };
  }

  WindowRect restoreWindowedRect(const WindowRect& saved,
                                 const std::span<const WindowRect> workAreas,
                                 const WindowRect& fallback)
  {
    if (findMostOverlappedArea(saved, workAreas).has_value())
    {
      return saved;
    }

    return centerInArea(saved.width, saved.height, fallback);
  }

} // namespace vke
