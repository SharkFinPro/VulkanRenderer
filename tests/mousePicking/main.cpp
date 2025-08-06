#include "../common/gui.h"
#include <source/components/lighting/LightingManager.h>
#include <source/components/MousePicker.h>
#include <source/components/objects/RenderObject.h>
#include <source/VulkanEngine.h>
#include <imgui.h>
#include <iostream>

struct MousePickingObject {
  std::shared_ptr<RenderObject> object;
  bool hovering = false;
  bool selected = false;
};

void setupScene(VulkanEngine& renderer, std::vector<MousePickingObject>& objects,
                std::vector<std::shared_ptr<Light>>& lights);

void renderScene(VulkanEngine& renderer, const std::shared_ptr<ImGuiInstance>& gui, std::vector<MousePickingObject>& objects,
                const std::vector<std::shared_ptr<Light>>& lights);

int main()
{
  try
  {
    constexpr VulkanEngineOptions vulkanEngineOptions {
      .WINDOW_WIDTH = 800,
      .WINDOW_HEIGHT = 600,
      .WINDOW_TITLE = "Cube",
      .CAMERA_POSITION = { 0.0f, 0.0f, -5.0f },
      .DO_DOTS = false
    };

    VulkanEngine renderer(vulkanEngineOptions);
    const auto gui = renderer.getImGuiInstance();

    ImGui::SetCurrentContext(VulkanEngine::getImGuiContext());

    std::vector<MousePickingObject> objects;
    std::vector<std::shared_ptr<Light>> lights;
    setupScene(renderer, objects, lights);

    while (renderer.isActive())
    {
      if (renderer.getMousePicker()->canMousePick() && renderer.buttonIsPressed(GLFW_MOUSE_BUTTON_LEFT))
      {
        for (auto& [_, hovering, selected] : objects)
        {
          selected = hovering;
        }
      }

      renderScene(renderer, gui, objects, lights);
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void setupScene(VulkanEngine& renderer, std::vector<MousePickingObject>& objects,
                std::vector<std::shared_ptr<Light>>& lights)
{
  const auto texture = renderer.loadTexture("assets/textures/white.png");
  const auto specularMap = renderer.loadTexture("assets/textures/blank_specular.png");
  const auto model = renderer.loadModel("assets/models/square.glb");

  const auto object1 = renderer.loadRenderObject(texture, specularMap, model);
  object1->setPosition({ 0, -5, 0 });
  objects.push_back({ object1 });

  const auto object2 = renderer.loadRenderObject(texture, specularMap, model);
  object2->setPosition({ -5, -10, 0 });
  objects.push_back({ object2 });

  const auto object3 = renderer.loadRenderObject(texture, specularMap, model);
  object3->setPosition({ 10, 0, 15 });
  objects.push_back({ object3 });


  lights.push_back(renderer.getLightingManager()->createLight({0, -3.5f, 0}, {1.0f, 1.0f, 1.0f}, 0.1f, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createLight({5.0f, -3.5f, 5.0f}, {1.0f, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createLight({-5.0f, -3.5f, -5.0f}, {0.5f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createLight({5.0f, -3.5f, -5.0f}, {0, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createLight({-5.0f, -3.5f, 5.0f}, {1.0f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));
}

void renderScene(VulkanEngine& renderer, const std::shared_ptr<ImGuiInstance>& gui, std::vector<MousePickingObject>& objects,
                const std::vector<std::shared_ptr<Light>>& lights)
{
  gui->dockCenter("SceneView");
  gui->dockBottom("Selected Object");
  gui->dockBottom("Lights");
  gui->dockBottom("Elliptical Dots");
  gui->dockBottom("Noisy Elliptical Dots");

  gui->setBottomDockPercent(0.3);

  // Render GUI
  ImGui::Begin("Selected Object");
  for (auto& [object, _, selected] : objects)
  {
    if (selected)
    {
      displayObjectGui(object, 0);
    }
  }
  ImGui::End();

  ImGui::Begin("Lights");
  for (int i = 0; i < lights.size(); i++)
  {
    displayLightGui(lights[i], i);
  }
  ImGui::End();

  // Render Objects
  for (auto& [object, hovering, selected] : objects)
  {
    renderer.renderObject(object, PipelineType::object, &hovering);

    if (selected)
    {
      renderer.renderObject(object, PipelineType::objectHighlight);
    }
  }

  for (const auto& light : lights)
  {
    renderer.getLightingManager()->renderLight(light);
  }

  // Render Frame
  renderer.render();
}