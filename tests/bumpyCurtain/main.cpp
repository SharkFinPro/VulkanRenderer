#include "../common/gui.h"
#include <source/components/lighting/LightingManager.h>
#include <source/components/objects/RenderObject.h>
#include <source/components/PipelineManager.h>
#include <source/pipelines/custom/config/PipelineTypes.h>
#include <source/VulkanEngine.h>
#include <imgui.h>
#include <iostream>

void renderScene(VulkanEngine& renderer, const std::shared_ptr<ImGuiInstance>& gui,
                 const std::shared_ptr<RenderObject>& object, const std::vector<std::shared_ptr<Light>>& lights,
                 bool& useBumpyCurtain);

int main()
{
  try
  {
    constexpr VulkanEngineOptions vulkanEngineOptions {
      .WINDOW_WIDTH = 800,
      .WINDOW_HEIGHT = 600,
      .WINDOW_TITLE = "Bumpy Curtain",
      .CAMERA_POSITION = { 0.0f, 0.0f, -15.0f },
      .DO_DOTS = false
    };

    VulkanEngine renderer(vulkanEngineOptions);
    const auto gui = renderer.getImGuiInstance();

    ImGui::SetCurrentContext(ImGuiInstance::getImGuiContext());

    const auto texture = renderer.loadTexture("assets/textures/white.png");
    const auto specularMap = renderer.loadTexture("assets/textures/blank_specular.png");
    const auto model = renderer.loadModel("assets/models/curtain.glb");

    const auto object = renderer.loadRenderObject(texture, specularMap, model);

    std::vector<std::shared_ptr<Light>> lights;

    lights.push_back(renderer.getLightingManager()->createLight({0, -3.5f, 0}, {1.0f, 1.0f, 1.0f}, 0.1f, 0.5f, 1.0f));

    lights.push_back(renderer.getLightingManager()->createLight({5.0f, -3.5f, 5.0f}, {1.0f, 1.0f, 0}, 0, 0.5f, 1.0f));

    lights.push_back(renderer.getLightingManager()->createLight({-5.0f, -3.5f, -5.0f}, {0.5f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));

    lights.push_back(renderer.getLightingManager()->createLight({5.0f, -3.5f, -5.0f}, {0, 1.0f, 0}, 0, 0.5f, 1.0f));

    lights.push_back(renderer.getLightingManager()->createLight({-5.0f, -3.5f, 5.0f}, {1.0f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));

    bool useBumpyCurtain = true;

    while (renderer.isActive())
    {
      renderScene(renderer, gui, object, lights, useBumpyCurtain);
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void renderScene(VulkanEngine& renderer, const std::shared_ptr<ImGuiInstance>& gui,
                 const std::shared_ptr<RenderObject>& object, const std::vector<std::shared_ptr<Light>>& lights,
                 bool& useBumpyCurtain)
{
  // Render GUI
  displayGui(gui, lights, { object });

  ImGui::Begin("Rendering");
  ImGui::Checkbox("Use Bumpy Curtain", &useBumpyCurtain);
  ImGui::End();


  // Render Objects
  renderer.getPipelineManager()->renderObject(object, useBumpyCurtain ? PipelineType::bumpyCurtain : PipelineType::object);

  for (const auto& light : lights)
  {
    renderer.getLightingManager()->renderLight(light);
  }

  // Render Frame
  renderer.render();
}