#ifndef VKE_MATERIAL_H
#define VKE_MATERIAL_H

#include <vulkan/vulkan_raii.hpp>
#include <memory>

namespace vke {

  class DescriptorAllocator;
  class DescriptorSet;
  class LogicalDevice;
  class Texture;

  // A texture and specular map with the descriptor set (the material set, set 1 of the object pipelines) that binds
  // them. Render objects with the same textures share one, so drawing them in a row needs one set bind.
  class Material {
  public:
    Material(const std::shared_ptr<LogicalDevice>& logicalDevice,
             const std::shared_ptr<DescriptorAllocator>& descriptorAllocator,
             vk::DescriptorSetLayout descriptorSetLayout,
             std::shared_ptr<Texture> texture,
             std::shared_ptr<Texture> specularMap);

    ~Material();

    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;

    [[nodiscard]] const std::shared_ptr<Texture>& getTexture() const;

    [[nodiscard]] const std::shared_ptr<Texture>& getSpecularMap() const;

    [[nodiscard]] vk::DescriptorSet getDescriptorSet(uint32_t currentFrame) const;

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    std::shared_ptr<DescriptorSet> m_descriptorSet;

    std::shared_ptr<Texture> m_texture;
    std::shared_ptr<Texture> m_specularMap;
  };

} // namespace vke

#endif //VKE_MATERIAL_H
