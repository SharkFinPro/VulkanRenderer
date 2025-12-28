#include "Surface.h"
#include "Window.h"
#include "../instance/Instance.h"

namespace vke {
  Surface::Surface(std::shared_ptr<Instance> instance,
                   const std::shared_ptr<Window>& window)
    : m_instance(std::move(instance))
  {
    m_surface = m_instance->createSurface(window->getWindow());
  }

  Surface::~Surface()
  {
    m_instance->destroySurface(m_surface);
  }

  VkSurfaceKHR Surface::getSurface() const
  {
    return m_surface;
  }
} // vke