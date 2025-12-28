#ifndef VULKANPROJECT_SURFACE_H
#define VULKANPROJECT_SURFACE_H

#include <vulkan/vulkan.h>
#include <memory>

namespace vke {

  class Instance;
  class Window;

  class Surface {
  public:
    Surface(std::shared_ptr<Instance> instance,
            const std::shared_ptr<Window>& window);

    ~Surface();

    [[nodiscard]] VkSurfaceKHR getSurface() const;

  private:
    std::shared_ptr<Instance> m_instance;

    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
  };
} // vke

#endif //VULKANPROJECT_SURFACE_H