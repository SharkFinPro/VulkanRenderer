#ifndef VKE_VOLUMEPALETTE_H
#define VKE_VOLUMEPALETTE_H

#include "VolumeMaterial.h"
#include <glm/vec4.hpp>
#include <vulkan/vulkan_raii.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace vke {

  class DescriptorAllocator;
  class DescriptorSet;
  class LogicalDevice;
  class Texture2DArray;

  // The palette's uniform block as the fragment shader reads it (std140); the scale is in x
  struct VolumePaletteData {
    std::array<glm::vec4, s_maxVolumeMaterials> textureScales;
    uint32_t materialCount;
    uint32_t padding[3];
  };

  static_assert(sizeof(VolumePaletteData) == 272);
  static_assert(offsetof(VolumePaletteData, materialCount) == 256);

  // What building a palette needs from the engine
  struct VolumePaletteContext {
    std::shared_ptr<LogicalDevice> logicalDevice;
    vk::CommandPool commandPool;
    std::shared_ptr<DescriptorAllocator> descriptorAllocator;
    vk::DescriptorSetLayout layout;
  };

  // A volume surface's materials as the shader reads them: an albedo array and a specular array (layer i is material
  // i), a uniform buffer of per-material texture scales, and the descriptor set (set 1 of the volume surface pipeline)
  // that binds them. Immutable once built; a surface swaps in a new one to change its materials.
  class VolumePalette {
  public:
    // Throws what validateVolumePalette throws. A null texture becomes a white layer. An empty palette is not valid
    // here; see createDefault.
    VolumePalette(const VolumePaletteContext& context,
                  std::span<const VolumeMaterial> materials);

    ~VolumePalette();

    VolumePalette(const VolumePalette&) = delete;
    VolumePalette& operator=(const VolumePalette&) = delete;

    // The engine's default: s_defaultVolumeColors as solid layers, white specular, scale 1
    [[nodiscard]] static std::shared_ptr<VolumePalette> createDefault(const VolumePaletteContext& context);

    [[nodiscard]] vk::DescriptorSet getDescriptorSet(uint32_t currentFrame) const;

    // The device address of the uniform block, for the ray tracing shaders
    [[nodiscard]] vk::DeviceAddress getDataAddress() const;

    [[nodiscard]] vk::DescriptorImageInfo getAlbedoImageInfo() const;

    [[nodiscard]] vk::DescriptorImageInfo getSpecularImageInfo() const;

  private:
    struct Layers;

    VolumePalette(const VolumePaletteContext& context,
                  Layers&& layers);

    void createUniformBuffer(const std::shared_ptr<LogicalDevice>& logicalDevice,
                             std::span<const float> scales);

    void writeDescriptorSet() const;

    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::shared_ptr<Texture2DArray> m_albedo;
    std::shared_ptr<Texture2DArray> m_specular;

    vk::raii::Buffer m_buffer = nullptr;
    vk::raii::DeviceMemory m_bufferMemory = nullptr;
    vk::DeviceAddress m_dataAddress = 0;

    std::shared_ptr<DescriptorSet> m_descriptorSet;
  };

} // namespace vke

#endif //VKE_VOLUMEPALETTE_H
