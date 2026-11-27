#include "DrawRun.h"
#include <algorithm>
#include <functional>

namespace vke {

  namespace {

    [[nodiscard]] bool sharesRun(const DrawItem& a,
                                 const DrawItem& b)
    {
      return a.model == b.model && a.material == b.material && a.mesh == b.mesh &&
             a.maskValue == b.maskValue;
    }

  } // namespace

  bool drawsBefore(const DrawItem& a,
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

    if (a.mesh != b.mesh)
    {
      return pointerLess(a.mesh, b.mesh);
    }

    if (a.model != b.model)
    {
      return pointerLess(a.model, b.model);
    }

    return a.slot < b.slot;
  }

  uint32_t buildDrawRuns(std::vector<DrawItem>& items,
                         uint32_t* drawSlots,
                         const uint32_t offset,
                         std::vector<DrawRun>& runs)
  {
    // Lists cut from an order sorted once per frame (Renderer3D) arrive sorted, and checking is linear where sorting
    // again is not
    if (!std::ranges::is_sorted(items, drawsBefore))
    {
      std::ranges::sort(items, drawsBefore);
    }

    uint32_t next = offset;

    for (const DrawItem& item : items)
    {
      if (next == offset || !sharesRun(items[next - offset - 1], item))
      {
        runs.push_back({ item.model, item.material, item.maskValue, next, 0, item.mesh, item.palette });
      }

      ++runs.back().instanceCount;

      drawSlots[next++] = item.slot;
    }

    return next;
  }

  void sortRunsFrontToBack(const std::vector<DrawItem>& items,
                           const uint32_t offset,
                           std::vector<DrawRun>& runs,
                           std::vector<std::pair<float, DrawRun>>& scratch)
  {
    scratch.clear();

    for (const DrawRun& run : runs)
    {
      // The run's items are the ones whose slots it draws, in the order buildDrawRuns wrote them
      const auto first = items.begin() + (run.firstInstance - offset);
      const auto nearest = std::ranges::min_element(first, first + run.instanceCount, {}, &DrawItem::distance);

      scratch.emplace_back(nearest->distance, run);
    }

    // Runs at equal distances keep the grouping order (firstInstance is unique and follows it), so the order doesn't
    // depend on the sort's internals; a plain sort with that tie-break, since a stable sort would allocate
    std::ranges::sort(scratch, [](const std::pair<float, DrawRun>& a, const std::pair<float, DrawRun>& b)
    {
      return a.first != b.first ? a.first < b.first : a.second.firstInstance < b.second.firstInstance;
    });

    for (size_t i = 0; i < runs.size(); ++i)
    {
      runs[i] = scratch[i].second;
    }
  }

} // namespace vke
