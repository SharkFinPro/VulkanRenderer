#ifndef VKE_DRAWRUN_H
#define VKE_DRAWRUN_H

#include <cstdint>
#include <utility>
#include <vector>

namespace vke {

  class Material;
  class Mesh;
  class Model;

  // A draw to be grouped: the object table slot it draws and what neighboring draws must share to be one instanced draw.
  // A key a list does not group by (the material of a shadow list, the mask value of a main pass list) is null or zero.
  struct DrawItem {
    const Model* model;
    const Material* material;
    uint32_t maskValue;
    uint32_t slot;
    // Set instead of the model for a volume surface chunk
    const Mesh* mesh = nullptr;
    // Squared distance from the camera to the item's world bounds, where a list orders its runs front to back
    // (sortRunsFrontToBack); not a grouping key
    float distance = 0.0f;
  };

  // A run of draws with equal keys, drawn with one instanced draw. firstInstance is the run's offset into the frame's
  // draw slots, which hold the runs' slots one after the other.
  struct DrawRun {
    const Model* model;
    const Material* material;
    uint32_t maskValue;
    uint32_t firstInstance;
    uint32_t instanceCount;
    const Mesh* mesh = nullptr;
  };

  // Sorts the items by (maskValue, material, mesh, model, slot), writes their slots to drawSlots starting at offset, and
  // appends one run per distinct (maskValue, material, mesh, model) to runs. Returns the offset after the last slot written.
  // The slot is the last key, so the order within a run is the submission order without a stable sort, which would
  // allocate.
  [[nodiscard]] uint32_t buildDrawRuns(std::vector<DrawItem>& items,
                                       uint32_t* drawSlots,
                                       uint32_t offset,
                                       std::vector<DrawRun>& runs);

  // Reorders runs, just built by buildDrawRuns from items (still in the order it left them, its slots written from offset),
  // nearest first by the smallest distance among each run's items. An opaque, depth-tested list drawn this way lets the
  // early depth test reject what the nearer runs hide before it is shaded, where the grouping order is unrelated to depth.
  // The runs' slots are unchanged, so each run still draws the same instances. scratch is reused storage.
  void sortRunsFrontToBack(const std::vector<DrawItem>& items,
                           uint32_t offset,
                           std::vector<DrawRun>& runs,
                           std::vector<std::pair<float, DrawRun>>& scratch);

} // namespace vke

#endif //VKE_DRAWRUN_H
