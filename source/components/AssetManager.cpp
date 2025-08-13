#include "AssetManager.h"
#include "core/logicalDevice/LogicalDevice.h"
#include "objects/Model.h"
#include "objects/RenderObject.h"
#include "textures/Texture2D.h"
#include <array>

AssetManager::AssetManager(const std::shared_ptr<LogicalDevice>& logicalDevice, VkCommandPool commandPool)
  : m_logicalDevice(logicalDevice), m_commandPool(commandPool)
{
  createObjectDescriptorSetLayout();
}

AssetManager::~AssetManager()
{
  m_logicalDevice->destroyDescriptorSetLayout(m_objectDescriptorSetLayout);
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

VkDescriptorSetLayout AssetManager::getObjectDescriptorSetLayout() const
{
  return m_objectDescriptorSetLayout;
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
