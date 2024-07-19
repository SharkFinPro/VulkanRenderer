#include <iostream>
#include <source/VulkanEngine.h>
#include <source/objects/RenderObject.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

int main()
{
  try
  {
    auto vulkanEngineOptions = VulkanEngineOptions{};
    vulkanEngineOptions.WINDOW_WIDTH = 800;
    vulkanEngineOptions.WINDOW_HEIGHT = 600;
    vulkanEngineOptions.WINDOW_TITLE = "Cube";
    vulkanEngineOptions.VERTEX_SHADER_FILE = "assets/shaders/vert.spv";
    vulkanEngineOptions.FRAGMENT_SHADER_FILE = "assets/shaders/frag.spv";
    vulkanEngineOptions.cameraPosition = { 0.0f, 0.0f, -5.0f };

    VulkanEngine renderer(&vulkanEngineOptions);

    auto texture = renderer.loadTexture("assets/textures/white.png");
    auto specularMap = renderer.loadTexture("assets/textures/blank_specular.png");
    auto model = renderer.loadModel("assets/models/square.glb");

    auto object = renderer.loadRenderObject(texture, specularMap, model);
    glm::vec3 position = {0, -5, 0};
    object->setPosition(position);

    while (renderer.isActive())
    {
      ImGui_ImplVulkan_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      ImGui::Begin("Window");
      ImGui::Text("Control Object:");
      ImGui::SliderFloat("x", &position.x, -50.0f, 50.0f);
      ImGui::SliderFloat("y", &position.y, -50.0f, 50.0f);
      ImGui::SliderFloat("z", &position.z, -50.0f, 50.0f);
      ImGui::End();

      object->setPosition(position);

      renderer.render();
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}