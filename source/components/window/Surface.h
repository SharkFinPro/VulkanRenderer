#ifndef VULKANPROJECT_SURFACE_H
#define VULKANPROJECT_SURFACE_H

#include <vulkan/vulkan_raii.hpp>
#include <memory>

namespace vke {

  class Instance;
  class Window;

  class Surface {
  public:
    Surface(const std::shared_ptr<Instance>& instance,
            const std::shared_ptr<Window>& window);

    [[nodiscard]] vk::SurfaceKHR getSurface() const;

  private:
    vk::raii::SurfaceKHR m_surface = nullptr;
  };
} // vke

#endif //VULKANPROJECT_SURFACE_H