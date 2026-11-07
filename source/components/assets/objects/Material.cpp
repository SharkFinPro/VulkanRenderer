#include "Material.h"
#include "../textures/Texture.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../pipelines/descriptorSets/DescriptorSet.h"
#include <array>

namespace vke {

  Material::Material(const std::shared_ptr<LogicalDevice>& logicalDevice,
                     const std::shared_ptr<DescriptorAllocator>& descriptorAllocator,
                     const vk::DescriptorSetLayout descriptorSetLayout,
                     std::shared_ptr<Texture> texture,
                     std::shared_ptr<Texture> specularMap)
    : m_logicalDevice(logicalDevice),
      m_descriptorSet(std::make_shared<DescriptorSet>(logicalDevice, descriptorAllocator, descriptorSetLayout)),
      m_texture(std::move(texture)),
      m_specularMap(std::move(specularMap))
  {
    m_descriptorSet->updateDescriptorSets([this](const vk::DescriptorSet descriptorSet, [[maybe_unused]] const size_t frame)
    {
      return std::array {
        m_texture->getDescriptorSet(0, descriptorSet),
        m_specularMap->getDescriptorSet(1, descriptorSet)
      };
    });
  }

  Material::~Material()
  {
    // Frames in flight may still sample the textures.
    m_logicalDevice->retire(std::move(m_texture));
    m_logicalDevice->retire(std::move(m_specularMap));
  }

  const std::shared_ptr<Texture>& Material::getTexture() const
  {
    return m_texture;
  }

  const std::shared_ptr<Texture>& Material::getSpecularMap() const
  {
    return m_specularMap;
  }

  vk::DescriptorSet Material::getDescriptorSet(const uint32_t currentFrame) const
  {
    return m_descriptorSet->getDescriptorSet(currentFrame);
  }

} // namespace vke
