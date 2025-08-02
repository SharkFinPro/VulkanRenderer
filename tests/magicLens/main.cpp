#include "../common/gui.h"
#include <source/components/objects/RenderObject.h>
#include <source/VulkanEngine.h>
#include <imgui.h>
#include <iostream>

void createLights(const VulkanEngine& renderer, std::vector<std::shared_ptr<Light>>& lights);
void renderScene(VulkanEngine& renderer, const std::shared_ptr<ImGuiInstance>& gui,
                 const std::shared_ptr<RenderObject>& object, const std::vector<std::shared_ptr<Light>>& lights,
                 bool& useMagicLens);

int main()
{
  try
  {
    constexpr VulkanEngineOptions vulkanEngineOptions {
      .WINDOW_WIDTH = 800,
      .WINDOW_HEIGHT = 600,
      .WINDOW_TITLE = "Magic Lens",
      .CAMERA_POSITION = { 0.0f, 0.0f, -15.0f },
      .DO_DOTS = false
    };

    VulkanEngine renderer(vulkanEngineOptions);
    const auto gui = renderer.getImGuiInstance();

    ImGui::SetCurrentContext(VulkanEngine::getImGuiContext());

    const auto texture = renderer.loadTexture("assets/textures/container.png");
    const auto specularMap = renderer.loadTexture("assets/textures/blank_specular.png");
    const auto model = renderer.loadModel("assets/models/curtain.glb");

    const auto object = renderer.loadRenderObject(texture, specularMap, model);

    std::vector<std::shared_ptr<Light>> lights;
    createLights(renderer, lights);

    bool useMagicLens = true;

    while (renderer.isActive())
    {
      renderScene(renderer, gui, object, lights, useMagicLens);
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void createLights(const VulkanEngine& renderer, std::vector<std::shared_ptr<Light>>& lights)
{
  lights.push_back(renderer.createLight({0, -3.5f, 0}, {1.0f, 1.0f, 1.0f}, 0.1f, 0.5f, 1.0f));

  lights.push_back(renderer.createLight({5.0f, -3.5f, 5.0f}, {1.0f, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.createLight({-5.0f, -3.5f, -5.0f}, {0.5f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.createLight({5.0f, -3.5f, -5.0f}, {0, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.createLight({-5.0f, -3.5f, 5.0f}, {1.0f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));
}

void renderScene(VulkanEngine& renderer, const std::shared_ptr<ImGuiInstance>& gui,
                 const std::shared_ptr<RenderObject>& object, const std::vector<std::shared_ptr<Light>>& lights,
                 bool& useMagicLens)
{
  displayGui(gui, lights, { object });

  ImGui::Begin("Rendering");
  ImGui::Checkbox("Use Magic Lens", &useMagicLens);
  ImGui::End();


  // Render Objects
  renderer.renderObject(object, useMagicLens ? PipelineType::magnifyWhirlMosaic : PipelineType::object);

  for (const auto& light : lights)
  {
    renderer.renderLight(light);
  }

  // Render Frame
  renderer.render();
}