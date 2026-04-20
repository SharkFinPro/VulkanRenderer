#include "AssetManager.h"
#include "fonts/Font.h"
#include "objects/Cloud.h"
#include "objects/Model.h"
#include "objects/RenderObject.h"
#include "objects/SmokeVolume.h"
#include "particleSystems/SmokeSystem.h"
#include "../assets/textures/Texture2D.h"
#include "../logicalDevice/LogicalDevice.h"
#include "../physicalDevice/PhysicalDevice.h"
#include <array>
#include <stdexcept>

namespace vke {

  AssetManager::AssetManager(std::shared_ptr<LogicalDevice> logicalDevice)
    : m_logicalDevice(std::move(logicalDevice))
  {
    createCommandPool();

    createDescriptorPool();

    createDescriptorSetLayouts();
  }

  std::shared_ptr<Texture2D> AssetManager::loadTexture(const char* path,
                                                       const bool repeat)
  {
    return std::make_shared<Texture2D>(
      m_logicalDevice,
      *m_commandPool,
      path,
      repeat ? vk::SamplerAddressMode::eRepeat : vk::SamplerAddressMode::eClampToEdge
    );
  }

  std::shared_ptr<Model> AssetManager::loadModel(const char* path,
                                                 glm::vec3 rotation)
  {
    return std::make_shared<Model>(
      m_logicalDevice,
      *m_commandPool,
      path,
      rotation
    );
  }

  std::shared_ptr<RenderObject> AssetManager::loadRenderObject(
    const std::shared_ptr<Texture2D>& texture,
    const std::shared_ptr<Texture2D>& specularMap,
    const std::shared_ptr<Model>& model)
  {
    return std::make_shared<RenderObject>(
      m_logicalDevice,
      getDescriptorPool(),
      *m_objectDescriptorSetLayout,
      texture,
      specularMap,
      model
    );
  }

  void AssetManager::registerFont(std::string fontName, std::string fontPath)
  {
    m_fontNames.insert({ std::move(fontName), std::move(fontPath) });
  }

  std::shared_ptr<Font> AssetManager::getFont(const std::string& fontName,
                                              const uint32_t fontSize)
  {
    const FontKey key { fontName, fontSize };

    auto font = m_fonts.find(key);

    if (font == m_fonts.end())
    {
      loadFont(fontName, fontSize);

      font = m_fonts.find(key);
    }

    return font->second;
  }

  std::shared_ptr<SmokeSystem> AssetManager::createSmokeSystem(glm::vec3 position,
                                                               uint32_t numParticles)
  {
    return std::make_shared<SmokeSystem>(
      m_logicalDevice,
      *m_commandPool,
      getDescriptorPool(),
      *m_smokeSystemDescriptorSetLayout,
      position,
      numParticles
    );
  }

  std::shared_ptr<SmokeVolume> AssetManager::createSmokeVolume(glm::vec3 position)
  {
    return std::make_shared<SmokeVolume>(
      m_logicalDevice,
      *m_commandPool,
      position
    );
  }

  std::shared_ptr<Cloud> AssetManager::createCloud()
  {
    return std::make_shared<Cloud>(m_logicalDevice, *m_commandPool);
  }

  vk::DescriptorSetLayout AssetManager::getObjectDescriptorSetLayout() const
  {
    return *m_objectDescriptorSetLayout;
  }

  vk::DescriptorSetLayout AssetManager::getFontDescriptorSetLayout() const
  {
    return *m_fontDescriptorSetLayout;
  }

  vk::DescriptorSetLayout AssetManager::getSmokeSystemDescriptorSetLayout() const
  {
    return *m_smokeSystemDescriptorSetLayout;
  }

  vk::DescriptorSetLayout AssetManager::getRayTracingDescriptorSetLayout() const
  {
    return *m_rayTracingDescriptorSetLayout;
  }

  void AssetManager::createDescriptorSetLayouts()
  {
    createObjectDescriptorSetLayout();

    createFontDescriptorSetLayout();

    createSmokeSystemDescriptorSetLayout();

    createRayTracingDescriptorSetLayout();
  }

  void AssetManager::createObjectDescriptorSetLayout()
  {
    constexpr vk::DescriptorSetLayoutBinding transformLayout {
      .binding = 0,
      .descriptorType = vk::DescriptorType::eUniformBuffer,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eVertex |
                    vk::ShaderStageFlagBits::eGeometry |
                    vk::ShaderStageFlagBits::eFragment
    };

    constexpr vk::DescriptorSetLayoutBinding textureLayout {
      .binding = 1,
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eFragment
    };

    constexpr vk::DescriptorSetLayoutBinding specularLayout {
      .binding = 4,
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eFragment
    };

    constexpr std::array objectBindings {
      transformLayout,
      textureLayout,
      specularLayout
    };

    const vk::DescriptorSetLayoutCreateInfo objectLayoutCreateInfo {
      .bindingCount = static_cast<uint32_t>(objectBindings.size()),
      .pBindings = objectBindings.data()
    };

    m_objectDescriptorSetLayout = m_logicalDevice->createDescriptorSetLayout(objectLayoutCreateInfo);
  }

  void AssetManager::createFontDescriptorSetLayout()
  {
    constexpr vk::DescriptorSetLayoutBinding glyphBinding {
      .binding = 0,
      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
      .descriptorCount = 1,
      .stageFlags = vk::ShaderStageFlagBits::eFragment
    };

    constexpr std::array bindings { glyphBinding };

    const vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
      .bindingCount = static_cast<uint32_t>(bindings.size()),
      .pBindings = bindings.data()
    };

    m_fontDescriptorSetLayout = m_logicalDevice->createDescriptorSetLayout(descriptorSetLayoutCreateInfo);
  }

  void AssetManager::createSmokeSystemDescriptorSetLayout()
  {
    constexpr std::array smokeBindings {
      vk::DescriptorSetLayoutBinding { // DT
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eCompute
      },
      vk::DescriptorSetLayoutBinding { // Last frame SB
        .binding = 1,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eCompute
      },
      vk::DescriptorSetLayoutBinding { // Current frame SB
        .binding = 2,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eCompute
      },
      vk::DescriptorSetLayoutBinding { // Transform
        .binding = 3,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex |
                      vk::ShaderStageFlagBits::eFragment
      },
      vk::DescriptorSetLayoutBinding { // Smoke
        .binding = 4,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eCompute
      }
    };

    const vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
      .bindingCount = static_cast<uint32_t>(smokeBindings.size()),
      .pBindings = smokeBindings.data()
    };

    m_smokeSystemDescriptorSetLayout = m_logicalDevice->createDescriptorSetLayout(descriptorSetLayoutCreateInfo);
  }

  void AssetManager::createRayTracingDescriptorSetLayout()
  {
    if (!m_logicalDevice->getPhysicalDevice()->supportsRayTracing())
    {
      return;
    }

    constexpr std::array rayTracingBindings {
      vk::DescriptorSetLayoutBinding {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eAccelerationStructureKHR,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR |
                      vk::ShaderStageFlagBits::eClosestHitKHR
      },
      vk::DescriptorSetLayoutBinding {
        .binding = 1,
        .descriptorType = vk::DescriptorType::eStorageImage,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR
      },
      vk::DescriptorSetLayoutBinding {
        .binding = 2,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eRaygenKHR
      },
      vk::DescriptorSetLayoutBinding {
        .binding = 3,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR
      },
      vk::DescriptorSetLayoutBinding {
        .binding = 4,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR
      },
      vk::DescriptorSetLayoutBinding {
        .binding = 5,
        .descriptorType = vk::DescriptorType::eStorageBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR
      },
      vk::DescriptorSetLayoutBinding {
        .binding = 6,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eIntersectionKHR
      },
      vk::DescriptorSetLayoutBinding {
        .binding = 7,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 256,
        .stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR
      }
    };

    // Binding 7 uses variable descriptor count and partial binding.
    constexpr std::array bindingFlags {
      vk::DescriptorBindingFlags {},
      vk::DescriptorBindingFlags {},
      vk::DescriptorBindingFlags {},
      vk::DescriptorBindingFlags {},
      vk::DescriptorBindingFlags {},
      vk::DescriptorBindingFlags {},
      vk::DescriptorBindingFlags {},
      vk::DescriptorBindingFlagBits::eVariableDescriptorCount | vk::DescriptorBindingFlagBits::ePartiallyBound
    };

    const vk::DescriptorSetLayoutBindingFlagsCreateInfo flagsInfo {
      .bindingCount = static_cast<uint32_t>(bindingFlags.size()),
      .pBindingFlags = bindingFlags.data()
    };

    const vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
      .pNext = &flagsInfo,
      .bindingCount = static_cast<uint32_t>(rayTracingBindings.size()),
      .pBindings = rayTracingBindings.data()
    };

    m_rayTracingDescriptorSetLayout = m_logicalDevice->createDescriptorSetLayout(descriptorSetLayoutCreateInfo);
  }

  void AssetManager::loadFont(const std::string& fontName,
                              const uint32_t fontSize)
  {
    const auto fontPath = m_fontNames.find(fontName);

    if (fontPath == m_fontNames.end())
    {
      throw std::runtime_error("Font not found: " + fontName);
    }

    auto font = std::make_shared<Font>(
      m_logicalDevice,
      fontPath->second,
      fontSize,
      *m_commandPool,
      getDescriptorPool(),
      *m_fontDescriptorSetLayout
    );

    m_fonts.emplace(FontKey{ fontName, fontSize }, std::move(font));
  }

  void AssetManager::createCommandPool()
  {
    const vk::CommandPoolCreateInfo poolInfo {
      .queueFamilyIndex = m_logicalDevice->getPhysicalDevice()->getQueueFamilies().graphicsFamily.value()
    };

    m_commandPool = m_logicalDevice->createCommandPool(poolInfo);
  }

  void AssetManager::createDescriptorPool()
  {
    const std::array<vk::DescriptorPoolSize, 3> poolSizes {{
      { vk::DescriptorType::eUniformBuffer, m_logicalDevice->getMaxFramesInFlight() * m_descriptorPoolSize },
      { vk::DescriptorType::eStorageBuffer, m_logicalDevice->getMaxFramesInFlight() * m_descriptorPoolSize },
      { vk::DescriptorType::eCombinedImageSampler, m_logicalDevice->getMaxFramesInFlight() * m_descriptorPoolSize }
    }};

    const vk::DescriptorPoolCreateInfo poolCreateInfo {
      .maxSets = m_logicalDevice->getMaxFramesInFlight() * m_descriptorPoolSize,
      .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
      .pPoolSizes = poolSizes.data()
    };

    m_descriptorPools.push_back(m_logicalDevice->createDescriptorPool(poolCreateInfo));
  }

  vk::DescriptorPool AssetManager::getDescriptorPool()
  {
    m_currentDescriptorPoolSize++;

    if (m_currentDescriptorPoolSize > m_descriptorPoolSize)
    {
      m_currentDescriptorPoolSize = 1;
      createDescriptorPool();
    }

    return *m_descriptorPools.back();
  }

} // namespace vke