#ifndef VKE_DRAWRUN_H
#define VKE_DRAWRUN_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace vke {

  class Material;
  class Model;

  // A draw to be grouped: the object table slot it draws and what neighboring draws must share to be one instanced draw.
  // A key a list does not group by (the material of a shadow list, the mask value of a main pass list) is null or zero.
  struct DrawItem {
    const Model* model;
    const Material* material;
    uint32_t maskValue;
    uint32_t slot;
  };

  // A run of draws with equal keys, drawn with one instanced draw. firstInstance is the run's offset into the frame's
  // draw slots, which hold the runs' slots one after the other.
  struct DrawRun {
    const Model* model;
    const Material* material;
    uint32_t maskValue;
    uint32_t firstInstance;
    uint32_t instanceCount;
  };

  // Sorts the items by (maskValue, material, model, slot), writes their slots to drawSlots starting at offset, and
  // appends one run per distinct (maskValue, material, model) to runs. Returns the offset after the last slot written.
  // The slot is the last key, so the order within a run is the submission order without a stable sort, which would
  // allocate.
  [[nodiscard]] uint32_t buildDrawRuns(std::vector<DrawItem>& items,
                                       uint32_t* drawSlots,
                                       uint32_t offset,
                                       std::vector<DrawRun>& runs);

} // namespace vke

#endif //VKE_DRAWRUN_H
