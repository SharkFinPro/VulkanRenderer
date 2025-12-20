#include "AssetManager.h"
#include "fonts/Font.h"
#include "objects/Model.h"
#include "objects/RenderObject.h"
#include "../assets/textures/Texture2D.h"
#include "../logicalDevice/LogicalDevice.h"
#include <array>
#include <stdexcept>

namespace vke {

AssetManager::AssetManager(const std::shared_ptr<LogicalDevice>& logicalDevice,
                           VkCommandPool commandPool,
                           VkDescriptorPool descriptorPool)
  : m_logicalDevice(logicalDevice), m_commandPool(commandPool), m_descriptorPool(descriptorPool)
{
  createDescriptorSetLayouts();
}

AssetManager::~AssetManager()
{
  m_logicalDevice->destroyDescriptorSetLayout(m_objectDescriptorSetLayout);

  m_logicalDevice->destroyDescriptorSetLayout(m_fontDescriptorSetLayout);
}

std::shared_ptr<Texture2D> AssetManager::loadTexture(const char* path, bool repeat)
{
  auto texture = std::make_shared<Texture2D>(m_logicalDevice, m_commandPool, path,
                                             repeat ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
  m_textures.push_back(texture);

  return texture;
}

std::shared_ptr<Model> AssetManager::loadModel(const char* path, glm::vec3 rotation)
{
  auto model = std::make_shared<Model>(
    m_logicalDevice,
    m_commandPool,
    path,
    rotation
  );

  m_models.push_back(model);

  return model;
}

std::shared_ptr<RenderObject> AssetManager::loadRenderObject(const std::shared_ptr<Texture2D>& texture,
                                                             const std::shared_ptr<Texture2D>& specularMap,
                                                             const std::shared_ptr<Model>& model)
{
  auto renderObject = std::make_shared<RenderObject>(
    m_logicalDevice,
    m_objectDescriptorSetLayout,
    texture,
    specularMap,
    model
  );

  m_renderObjects.push_back(renderObject);

  return renderObject;

}

void AssetManager::registerFont(std::string fontName, std::string fontPath)
{
  m_fontNames.insert({fontName, fontPath});
}

std::shared_ptr<Font> AssetManager::getFont(const std::string& fontName, const uint32_t fontSize)
{
  auto fontByName = m_fonts.find(fontName);

  if (fontByName == m_fonts.end())
  {
    loadFont(fontName, fontSize);

    fontByName = m_fonts.find(fontName);
  }

  auto fontBySize = fontByName->second.find(fontSize);
  if (fontBySize == fontByName->second.end())
  {
    loadFont(fontName, fontSize);

    fontBySize = fontByName->second.find(fontSize);
  }

  return fontBySize->second;
}

VkDescriptorSetLayout AssetManager::getObjectDescriptorSetLayout() const
{
  return m_objectDescriptorSetLayout;
}

VkDescriptorSetLayout AssetManager::getFontDescriptorSetLayout() const
{
  return m_fontDescriptorSetLayout;
}

void AssetManager::createDescriptorSetLayouts()
{
  createObjectDescriptorSetLayout();

  createFontDescriptorSetLayout();
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

  std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings {
    glyphDescriptorSetLayoutBinding
  };

  const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size()),
    .pBindings = descriptorSetLayoutBindings.data()
  };

  m_fontDescriptorSetLayout = m_logicalDevice->createDescriptorSetLayout(descriptorSetLayoutCreateInfo);
}

void AssetManager::loadFont(const std::string& fontName, uint32_t fontSize)
{
  const auto fontPath = m_fontNames.find(fontName);

  if (fontPath == m_fontNames.end())
  {
    throw std::runtime_error("Font not found!");
  }

  auto font = std::make_shared<Font>(m_logicalDevice, fontPath->second, fontSize, m_commandPool, m_descriptorPool, m_fontDescriptorSetLayout);

  m_fonts[fontName].insert({fontSize, font});
}
} // namespace vke