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

  // The per-frame data the object shaders read through the scene set: the table of object data (binding 0) and the draw
  // slots (binding 2). A draw's first instance indexes the draw slots, whose entry is the object's index in the table,
  // so one instanced draw can cover objects that are not adjacent in the table. Each is a storage buffer per frame in
  // flight, persistently mapped and written by the CPU, so there is no staging copy. A buffer only grows, to the next
  // power of two, and the one it replaces is retired.
  class ObjectTable {
  public:
    // The fewest entries a buffer is made for, so a scene that grows one object at a time doesn't reallocate each frame.
    static constexpr size_t s_minimumCapacity = 256;

    explicit ObjectTable(std::shared_ptr<LogicalDevice> logicalDevice);

    // Makes the frame's buffers hold at least objectCount table entries and slotCount draw slots. Returns whether
    // either buffer was replaced, in which case the frame's descriptor set must be rewritten with getBufferInfo() and
    // getDrawSlotsBufferInfo(). Call it before any command that uses the frame's set is recorded.
    [[nodiscard]] bool reserve(uint32_t frame,
                               size_t objectCount,
                               size_t slotCount);

    // The frame's mapped entries, valid up to the reserved capacity and until the next reserve that returns true.
    [[nodiscard]] ObjectData* getEntries(uint32_t frame) const;

    // The frame's mapped draw slots, with the same validity.
    [[nodiscard]] uint32_t* getDrawSlots(uint32_t frame) const;

    [[nodiscard]] const vk::DescriptorBufferInfo& getBufferInfo(uint32_t frame) const;

    [[nodiscard]] const vk::DescriptorBufferInfo& getDrawSlotsBufferInfo(uint32_t frame) const;

    [[nodiscard]] size_t getCapacity(uint32_t frame) const;

    [[nodiscard]] size_t getDrawSlotCapacity(uint32_t frame) const;

  private:
    // Memory is declared first so that, on destruction, the buffer goes before the memory bound to it.
    struct MappedBuffer {
      vk::raii::DeviceMemory memory = nullptr;
      vk::raii::Buffer buffer = nullptr;
      void* mapped = nullptr;
      size_t capacity = 0;
      vk::DescriptorBufferInfo bufferInfo {};
    };

    struct Frame {
      MappedBuffer objects;
      MappedBuffer drawSlots;
    };

    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::vector<Frame> m_frames;

    // Replaces the buffer when it holds fewer than count elements. Returns whether it did.
    [[nodiscard]] bool grow(MappedBuffer& target,
                            size_t count,
                            size_t elementSize) const;

    void allocate(MappedBuffer& target,
                  size_t capacity,
                  size_t elementSize) const;
  };

} // namespace vke

#endif //VKE_OBJECTTABLE_H
