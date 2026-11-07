#include "DrawRun.h"
#include <algorithm>
#include <tuple>

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
      return std::tie(a.maskValue, a.material, a.model, a.slot) < std::tie(b.maskValue, b.material, b.model, b.slot);
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
