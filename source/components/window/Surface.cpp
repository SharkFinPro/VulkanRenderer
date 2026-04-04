#include "Surface.h"
#include "Window.h"
#include "../instance/Instance.h"

namespace vke {
  Surface::Surface(const std::shared_ptr<Instance>& instance,
                   const std::shared_ptr<Window>& window)
  : m_surface(instance->createSurface(window->getWindow()))
  {}

  vk::SurfaceKHR Surface::getSurface() const
  {
    return m_surface;
  }
} // vke