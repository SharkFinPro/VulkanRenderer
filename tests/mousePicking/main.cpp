#include "../common/gui.h"
#include <source/components/lighting/LightingManager.h>
#include <source/components/renderingManager/renderer3D/MousePicker.h>
#include <source/components/assets/objects/RenderObject.h>
#include <source/components/window/Window.h>
#include <source/components/assets/AssetManager.h>
#include <source/components/pipelines/implementations/common/PipelineTypes.h>
#include <source/VulkanEngine.h>
#include <imgui.h>
#include <algorithm>
#include <iostream>

struct MousePickingObject {
  std::shared_ptr<vke::RenderObject> object;
  bool hovering = false;
  bool selected = false;
};

void setupScene(const vke::VulkanEngine& renderer,
                std::vector<MousePickingObject>& mousePickingObjects,
                std::vector<std::shared_ptr<vke::RenderObject>>& objects,
                std::vector<std::shared_ptr<vke::Light>>& lights);

void renderScene(vke::VulkanEngine& renderer,
                 std::vector<MousePickingObject>& objects,
                 const std::vector<std::shared_ptr<vke::Light>>& lights);

int main()
{
  try
  {
    const vke::EngineConfig engineConfig {
      .window {
        .width = 800,
        .height = 600,
        .title = "Mouse Picking",
        .closeOnEscape = true
      },
      .camera {
        .position = { 0.0f, 0.0f, -5.0f }
      }
    };

    vke::VulkanEngine renderer(engineConfig);
    const auto gui = renderer.getImGuiInstance();

    ImGui::SetCurrentContext(vke::ImGuiInstance::getImGuiContext());

    std::vector<MousePickingObject> mousePickingObjects;
    std::vector<std::shared_ptr<vke::RenderObject>> objects;
    std::vector<std::shared_ptr<vke::Light>> lights;
    setupScene(renderer, mousePickingObjects, objects, lights);

    renderer.getRenderingManager()->setSceneOverlay([](ImDrawList* drawList, const vke::SceneViewRect& rect) {
      constexpr float inset = 4.0f;
      drawList->AddRect(
        ImVec2(rect.x + inset, rect.y + inset),
        ImVec2(rect.x + rect.width - inset, rect.y + rect.height - inset),
        IM_COL32(255, 255, 0, 255));

      const ImVec2 center(rect.x + rect.width * 0.5f, rect.y + rect.height * 0.5f);
      constexpr float crosshairSize = 6.0f;
      drawList->AddLine(ImVec2(center.x - crosshairSize, center.y), ImVec2(center.x + crosshairSize, center.y), IM_COL32(255, 255, 0, 255));
      drawList->AddLine(ImVec2(center.x, center.y - crosshairSize), ImVec2(center.x, center.y + crosshairSize), IM_COL32(255, 255, 0, 255));
    });

    while (renderer.isActive())
    {
      displayGui(renderer.getImGuiInstance(), lights, objects, renderer.getRenderingManager());

      if (renderer.getRenderingManager()->getRenderer3D()->getMousePicker()->canMousePick() && renderer.getWindow()->buttonIsPressed(GLFW_MOUSE_BUTTON_LEFT))
      {
        for (auto& [_, hovering, selected] : mousePickingObjects)
        {
          selected = hovering;
        }
      }

      renderScene(renderer, mousePickingObjects, lights);
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void setupScene(const vke::VulkanEngine& renderer,
                std::vector<MousePickingObject>& mousePickingObjects,
                std::vector<std::shared_ptr<vke::RenderObject>>& objects,
                std::vector<std::shared_ptr<vke::Light>>& lights)
{
  const auto texture = renderer.getAssetManager()->loadTexture("assets/textures/white.png");
  const auto specularMap = renderer.getAssetManager()->loadTexture("assets/textures/blank_specular.png");
  const auto model = renderer.getAssetManager()->loadModel("assets/models/square.glb");

  const auto object1 = renderer.getAssetManager()->loadRenderObject(texture, specularMap, model);
  object1->setPosition({ 0, -5, 0 });
  mousePickingObjects.push_back({ object1 });
  objects.push_back(object1);

  const auto object2 = renderer.getAssetManager()->loadRenderObject(texture, specularMap, model);
  object2->setPosition({ -5, -10, 0 });
  mousePickingObjects.push_back({ object2 });
  objects.push_back(object2);

  const auto object3 = renderer.getAssetManager()->loadRenderObject(texture, specularMap, model);
  object3->setPosition({ 10, 0, 15 });
  mousePickingObjects.push_back({ object3 });
  objects.push_back(object3);

  lights.push_back(renderer.getLightingManager()->createPointLight({0, -3.5f, 0}, {1.0f, 1.0f, 1.0f}, 0.1f, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createPointLight({5.0f, -3.5f, 5.0f}, {1.0f, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createPointLight({-5.0f, -3.5f, -5.0f}, {0.5f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createPointLight({5.0f, -3.5f, -5.0f}, {0, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createPointLight({-5.0f, -3.5f, 5.0f}, {1.0f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));
}

void renderScene(vke::VulkanEngine& renderer,
                 std::vector<MousePickingObject>& objects,
                 const std::vector<std::shared_ptr<vke::Light>>& lights)
{
  const auto r3d = renderer.getRenderingManager()->getRenderer3D();

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

  ImGui::Begin("Pick Result");
  if (const auto pick = r3d->getPickResult())
  {
    const auto found = std::ranges::find_if(objects, [&pick](const MousePickingObject& candidate) {
      return candidate.object == pick->renderObject;
    });

    if (found != objects.end())
    {
      ImGui::Text("Object: %d", static_cast<int>(found - objects.begin()));
    }
    ImGui::Text("Triangle: %u", pick->triangleIndex);
    ImGui::Text("World: %.3f %.3f %.3f", pick->worldPosition.x, pick->worldPosition.y, pick->worldPosition.z);
    ImGui::Text("Depth: %.6f", pick->depth);
  }
  else
  {
    ImGui::TextUnformatted("Object: none");
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
    if (selected)
    {
      r3d->renderOutline(object, { 1.0f, 0.5f, 0.0f, 1.0f });
    }
    else if (hovering)
    {
      r3d->renderOutline(object, { 1.0f, 1.0f, 0.0f, 1.0f });
    }

    r3d->renderObject(object, vke::PipelineType::object, &hovering);
  }

  for (const auto& light : lights)
  {
    renderer.getLightingManager()->renderLight(light);
  }

  // Render Frame
  renderer.render();
}