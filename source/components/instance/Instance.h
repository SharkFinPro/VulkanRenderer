#ifndef VKE_INSTANCE_H
#define VKE_INSTANCE_H

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>
#include <array>
#include <vector>

namespace vke {

  constexpr std::array<const char*, 1> validationLayers {
    "VK_LAYER_KHRONOS_validation"
  };

  class Instance {
  public:
    Instance();

    [[nodiscard]] VkSurfaceKHR createSurface(GLFWwindow* window) const;

    void destroySurface(VkSurfaceKHR& surface) const;

    [[nodiscard]] std::vector<vk::raii::PhysicalDevice> getPhysicalDevices() const;

    static bool validationLayersEnabled();

    friend class ImGuiInstance;

  private:
    vk::raii::Context m_context;

    vk::raii::Instance m_instance = nullptr;

    vk::raii::DebugUtilsMessengerEXT m_debugMessenger = nullptr;

    void createDebugUtilsMessenger();

    [[nodiscard]] bool checkValidationLayerSupport() const;

    static std::vector<const char*> getRequiredExtensions();
  };

} // namespace vke

#endif //VKE_INSTANCE_H
