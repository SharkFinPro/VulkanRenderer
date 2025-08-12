#include "Renderer.h"

Renderer::Renderer(const std::shared_ptr<LogicalDevice>& logicalDevice, VkCommandPool commandPool)
  : m_logicalDevice(logicalDevice), m_commandPool(commandPool)
{}
