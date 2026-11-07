#ifndef VKE_OBJECTTABLE_H
#define VKE_OBJECTTABLE_H

#include <glm/mat4x4.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace vke {

  class LogicalDevice;

  // One entry of the object table, as the shaders read it (ObjectData in shaders/common/SceneData.glsl, std430).
  struct ObjectData {
    glm::mat4 model;
    // mat4(transpose(inverse(mat3(model)))), so the shaders never invert a matrix per vertex or fragment
    glm::mat4 normalMatrix;
    // The 1-based picking id of the submission, or 0 when it is not pickable
    uint32_t pickId;
    uint32_t pad0;
    uint32_t pad1;
    uint32_t pad2;
  };

  static_assert(sizeof(ObjectData) == 144, "ObjectData must match the std430 layout in SceneData.glsl");

  // The per-frame table of object data the object shaders index with the draw's first instance: a storage buffer per
  // frame in flight, persistently mapped and written by the CPU, so there is no staging copy. A buffer only grows, to
  // the next power of two, and the one it replaces is retired.
  class ObjectTable {
  public:
    // The fewest entries a buffer is made for, so a scene that grows one object at a time doesn't reallocate each frame.
    static constexpr size_t s_minimumCapacity = 256;

    explicit ObjectTable(std::shared_ptr<LogicalDevice> logicalDevice);

    // Makes the frame's buffer hold at least count entries. Returns whether the buffer was replaced, in which case the
    // frame's descriptor set must be rewritten with getBufferInfo().
    [[nodiscard]] bool reserve(uint32_t frame,
                               size_t count);

    // The frame's mapped entries, valid up to the reserved capacity and until the next reserve that returns true.
    [[nodiscard]] ObjectData* getEntries(uint32_t frame) const;

    [[nodiscard]] const vk::DescriptorBufferInfo& getBufferInfo(uint32_t frame) const;

    [[nodiscard]] size_t getCapacity(uint32_t frame) const;

  private:
    // Memory is declared first so that, on destruction, the buffer goes before the memory bound to it.
    struct FrameTable {
      vk::raii::DeviceMemory memory = nullptr;
      vk::raii::Buffer buffer = nullptr;
      ObjectData* entries = nullptr;
      size_t capacity = 0;
      vk::DescriptorBufferInfo bufferInfo {};
    };

    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::vector<FrameTable> m_frames;

    void allocate(FrameTable& frame,
                  size_t capacity) const;
  };

} // namespace vke

#endif //VKE_OBJECTTABLE_H
