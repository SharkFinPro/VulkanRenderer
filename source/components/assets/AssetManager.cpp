#include "AssetManager.h"
#include "fonts/Font.h"
#include "objects/Model.h"
#include "objects/RenderObject.h"
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

  AssetManager::~AssetManager()
  {
    m_logicalDevice->destroyDescriptorSetLayout(m_objectDescriptorSetLayout);

    m_logicalDevice->destroyDescriptorSetLayout(m_fontDescriptorSetLayout);

    m_logicalDevice->destroyDescriptorSetLayout(m_smokeSystemDescriptorSetLayout);

    for (auto& descriptorPool : m_descriptorPools)
    {
      m_logicalDevice->destroyDescriptorPool(descriptorPool);
    }

    m_logicalDevice->destroyCommandPool(m_commandPool);
  }

  std::shared_ptr<Texture2D> AssetManager::loadTexture(const char* path,
                                                       const bool repeat)
  {
    auto texture = std::make_shared<Texture2D>(
      m_logicalDevice,
      m_commandPool,
      path,
      repeat ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
    );

    return texture;
  }

  std::shared_ptr<Model> AssetManager::loadModel(const char* path,
                                                 glm::vec3 rotation)
  {
    auto model = std::make_shared<Model>(
      m_logicalDevice,
      m_commandPool,
      path,
      rotation
    );

    return model;
  }

  std::shared_ptr<RenderObject> AssetManager::loadRenderObject(const std::shared_ptr<Texture2D>& texture,
                                                               const std::shared_ptr<Texture2D>& specularMap,
                                                               const std::shared_ptr<Model>& model)
  {
    auto renderObject = std::make_shared<RenderObject>(
      m_logicalDevice,
      getDescriptorPool(),
      m_objectDescriptorSetLayout,
      texture,
      specularMap,
      model
    );

    return renderObject;
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

  std::shared_ptr<SmokeSystem> AssetManager::createSmokeSystem(glm::vec3 position, uint32_t numParticles)
  {
    return std::make_shared<SmokeSystem>(
      m_logicalDevice,
      m_commandPool,
      getDescriptorPool(),
      m_smokeSystemDescriptorSetLayout,
      position,
      numParticles
    );
  }

  VkDescriptorSetLayout AssetManager::getObjectDescriptorSetLayout() const
  {
    return m_objectDescriptorSetLayout;
  }

  VkDescriptorSetLayout AssetManager::getFontDescriptorSetLayout() const
  {
    return m_fontDescriptorSetLayout;
  }

  VkDescriptorSetLayout AssetManager::getSmokeSystemDescriptorSetLayout() const
  {
    return m_smokeSystemDescriptorSetLayout;
  }

  void AssetManager::createDescriptorSetLayouts()
  {
    createObjectDescriptorSetLayout();

    createFontDescriptorSetLayout();

    createSmokeSystemDescriptorSetLayout();
  }

  void AssetManager::createObjectDescriptorSetLayout()
  {
    constexpr VkDescriptorSetLayoutBinding transformLayout {
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
    };

    constexpr VkDescriptorSetLayoutBinding textureLayout {
      .binding = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    constexpr VkDescriptorSetLayoutBinding specularLayout {
      .binding = 4,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    constexpr std::array objectBindings {
      transformLayout,
      textureLayout,
      specularLayout
    };

    const VkDescriptorSetLayoutCreateInfo objectLayoutCreateInfo {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = static_cast<uint32_t>(objectBindings.size()),
      .pBindings = objectBindings.data()
    };

    m_objectDescriptorSetLayout = m_logicalDevice->createDescriptorSetLayout(objectLayoutCreateInfo);
  }

  void AssetManager::createFontDescriptorSetLayout()
  {
    constexpr VkDescriptorSetLayoutBinding glyphDescriptorSetLayoutBinding {
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    constexpr std::array descriptorSetLayoutBindings {
      glyphDescriptorSetLayoutBinding
    };

    const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size()),
      .pBindings = descriptorSetLayoutBindings.data()
    };

    m_fontDescriptorSetLayout = m_logicalDevice->createDescriptorSetLayout(descriptorSetLayoutCreateInfo);
  }

  void AssetManager::createSmokeSystemDescriptorSetLayout()
  {
    const std::vector<VkDescriptorSetLayoutBinding> smokeSystemDescriptorSetLayoutBinding {{
      { // DT
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      },
      { // Last Frame SB
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
      },
      { // Current Frame SB
        .binding = 2,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
      },
      { // Transform
        .binding = 3,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
      },
      { // Smoke
        .binding = 4,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
      }
    }};

    const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = static_cast<uint32_t>(smokeSystemDescriptorSetLayoutBinding.size()),
      .pBindings = smokeSystemDescriptorSetLayoutBinding.data()
    };

    m_smokeSystemDescriptorSetLayout = m_logicalDevice->createDescriptorSetLayout(descriptorSetLayoutCreateInfo);
  }

  void AssetManager::loadFont(const std::string& fontName,
                              uint32_t fontSize)
  {
    const auto fontPath = m_fontNames.find(fontName);

    if (fontPath == m_fontNames.end())
    {
      throw std::runtime_error("Font not found: " + fontName);
    }

    auto font = std::make_shared<Font>(
      m_logicalDevice, fontPath->second, fontSize, m_commandPool, getDescriptorPool(), m_fontDescriptorSetLayout);

    m_fonts.emplace(FontKey{ fontName, fontSize }, std::move(font));
  }

  void AssetManager::createCommandPool()
  {
    const VkCommandPoolCreateInfo poolInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = m_logicalDevice->getPhysicalDevice()->getQueueFamilies().graphicsFamily.value()
    };

    m_commandPool = m_logicalDevice->createCommandPool(poolInfo);
  }

  void AssetManager::createDescriptorPool()
  {
    const std::array<VkDescriptorPoolSize, 3> poolSizes {{
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_logicalDevice->getMaxFramesInFlight() * m_descriptorPoolSize},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_logicalDevice->getMaxFramesInFlight() * m_descriptorPoolSize},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_logicalDevice->getMaxFramesInFlight() * m_descriptorPoolSize}
    }};

    const VkDescriptorPoolCreateInfo poolCreateInfo {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = m_logicalDevice->getMaxFramesInFlight() * m_descriptorPoolSize,
      .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
      .pPoolSizes = poolSizes.data()
    };

    m_descriptorPools.push_back(m_logicalDevice->createDescriptorPool(poolCreateInfo));
  }

  VkDescriptorPool AssetManager::getDescriptorPool()
  {
    m_currentDescriptorPoolSize++;

    if (m_currentDescriptorPoolSize > m_descriptorPoolSize)
    {
      m_currentDescriptorPoolSize = 1;
      createDescriptorPool();
    }

    return m_descriptorPools.back();
  }
} // namespace vke