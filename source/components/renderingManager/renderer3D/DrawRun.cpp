#include "DrawRun.h"
#include <algorithm>
#include <functional>

namespace vke {

  namespace {

    [[nodiscard]] bool sharesRun(const DrawItem& a,
                                 const DrawItem& b)
    {
      return a.model == b.model && a.material == b.material && a.maskValue == b.maskValue;
    }

    [[nodiscard]] bool drawsBefore(const DrawItem& a,
                                   const DrawItem& b)
    {
      // std::less, since < on pointers to unrelated objects is unspecified
      constexpr std::less<const void*> pointerLess;

      if (a.maskValue != b.maskValue)
      {
        return a.maskValue < b.maskValue;
      }

      if (a.material != b.material)
      {
        return pointerLess(a.material, b.material);
      }

      if (a.model != b.model)
      {
        return pointerLess(a.model, b.model);
      }

      return a.slot < b.slot;
    }

  } // namespace

  uint32_t buildDrawRuns(std::vector<DrawItem>& items,
                         uint32_t* drawSlots,
                         const uint32_t offset,
                         std::vector<DrawRun>& runs)
  {
    std::ranges::sort(items, drawsBefore);

    uint32_t next = offset;

    for (const DrawItem& item : items)
    {
      if (next == offset || !sharesRun(items[next - offset - 1], item))
      {
        runs.push_back({ item.model, item.material, item.maskValue, next, 0 });
      }

      ++runs.back().instanceCount;

      drawSlots[next++] = item.slot;
    }

    return next;
  }

} // namespace vke
