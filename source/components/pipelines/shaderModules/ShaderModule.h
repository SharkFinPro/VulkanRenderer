#ifndef VKE_SHADERMODULE_H
#define VKE_SHADERMODULE_H

#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <vector>

namespace vke {

  class LogicalDevice;

  class ShaderModule {
  public:
    ShaderModule(std::shared_ptr<LogicalDevice> logicalDevice,
                 const char* filename,
                 vk::ShaderStageFlagBits stage);

    ~ShaderModule() = default;

    ShaderModule(ShaderModule&& other) noexcept
      : m_logicalDevice(std::move(other.m_logicalDevice)),
        m_stage(other.m_stage),
        m_module(std::move(other.m_module))
    {}

    [[nodiscard]] vk::PipelineShaderStageCreateInfo getShaderStageCreateInfo() const;

  private:
    std::shared_ptr<LogicalDevice> m_logicalDevice;

    vk::ShaderStageFlagBits m_stage{};
    vk::raii::ShaderModule m_module = nullptr;

    static std::vector<char> readFile(const char* filename);

    void createShaderModule(const char* file);
  };

} // namespace vke

#endif //VKE_SHADERMODULE_H