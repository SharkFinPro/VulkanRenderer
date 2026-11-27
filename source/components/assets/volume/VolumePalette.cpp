#include "VolumePalette.h"
#include "../textures/Texture2D.h"
#include "../textures/Texture2DArray.h"
#include "../../logicalDevice/LogicalDevice.h"
#include "../../pipelines/descriptorSets/DescriptorSet.h"
#include "../../../utilities/Buffers.h"
#include <cstring>
#include <stdexcept>
#include <utility>
#include <vector>

namespace vke {

  struct VolumePalette::Layers {
    std::vector<Texture2DArrayLayer> albedo;
    std::vector<Texture2DArrayLayer> specular;
    std::vector<float> scales;
  };

  namespace {

    constexpr glm::vec4 s_white { 1.0f };

  } // namespace

  VolumePalette::VolumePalette(const VolumePaletteContext& context,
                               const std::span<const VolumeMaterial> materials)
    : VolumePalette(context, [materials] {
        validateVolumePalette(materials);

        if (materials.empty())
        {
          throw std::invalid_argument("A volume surface palette needs at least one material.");
        }

        Layers layers;

        for (const VolumeMaterial& material : materials)
        {
          layers.albedo.push_back({ .source = material.albedo, .fill = s_white });
          layers.specular.push_back({ .source = material.specular, .fill = s_white });
          layers.scales.push_back(material.textureScale);
        }

        return layers;
      }())
  {
  }

  VolumePalette::VolumePalette(const VolumePaletteContext& context,
                               Layers&& layers)
    : m_logicalDevice(context.logicalDevice)
  {
    m_albedo = std::make_shared<Texture2DArray>(context.logicalDevice, context.commandPool, layers.albedo,
                                                vk::SamplerAddressMode::eRepeat);
    m_specular = std::make_shared<Texture2DArray>(context.logicalDevice, context.commandPool, layers.specular,
                                                  vk::SamplerAddressMode::eRepeat);

    VolumePaletteData data {};
    data.materialCount = static_cast<uint32_t>(layers.scales.size());

    for (size_t i = 0; i < layers.scales.size(); i++)
    {
      data.textureScales[i] = glm::vec4(layers.scales[i], 0.0f, 0.0f, 0.0f);
    }

    Buffers::createBuffer(context.logicalDevice, sizeof(VolumePaletteData), vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
                          vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                          m_buffer, m_bufferMemory);

    Buffers::doMappedMemoryOperation(m_bufferMemory, [&data](void* mapped) {
      std::memcpy(mapped, &data, sizeof(VolumePaletteData));
    });

    m_dataAddress = context.logicalDevice->getBufferDeviceAddress(*m_buffer);

    m_descriptorSet = std::make_shared<DescriptorSet>(context.logicalDevice, context.descriptorAllocator, context.layout);

    const vk::DescriptorBufferInfo bufferInfo {
      .buffer = *m_buffer,
      .offset = 0,
      .range = sizeof(VolumePaletteData)
    };

    m_descriptorSet->updateDescriptorSets([this, &bufferInfo](const vk::DescriptorSet descriptorSet,
                                                              [[maybe_unused]] const size_t frame)
    {
      return std::array {
        m_albedo->getDescriptorSet(0, descriptorSet),
        m_specular->getDescriptorSet(1, descriptorSet),
        vk::WriteDescriptorSet {
          .dstSet = descriptorSet,
          .dstBinding = 2,
          .dstArrayElement = 0,
          .descriptorCount = 1,
          .descriptorType = vk::DescriptorType::eUniformBuffer,
          .pBufferInfo = &bufferInfo
        }
      };
    });
  }

  VolumePalette::~VolumePalette()
  {
    // Frames in flight may still sample the arrays and read the buffer
    m_logicalDevice->retire(std::move(m_albedo));
    m_logicalDevice->retire(std::move(m_specular));
    m_logicalDevice->retireHandles(std::move(m_buffer));
    m_logicalDevice->retireHandles(std::move(m_bufferMemory));
  }

  std::shared_ptr<VolumePalette> VolumePalette::createDefault(const VolumePaletteContext& context)
  {
    Layers layers;

    for (const glm::vec3& color : s_defaultVolumeColors)
    {
      layers.albedo.push_back({ .source = nullptr, .fill = glm::vec4(color, 1.0f) });
      layers.specular.push_back({ .source = nullptr, .fill = s_white });
      layers.scales.push_back(1.0f);
    }

    return std::shared_ptr<VolumePalette>(new VolumePalette(context, std::move(layers)));
  }

  vk::DeviceAddress VolumePalette::getDataAddress() const
  {
    return m_dataAddress;
  }

  vk::DescriptorImageInfo VolumePalette::getAlbedoImageInfo() const
  {
    return m_albedo->getImageInfo();
  }

  vk::DescriptorImageInfo VolumePalette::getSpecularImageInfo() const
  {
    return m_specular->getImageInfo();
  }

  vk::DescriptorSet VolumePalette::getDescriptorSet(const uint32_t currentFrame) const
  {
    return m_descriptorSet->getDescriptorSet(currentFrame);
  }

} // namespace vke
