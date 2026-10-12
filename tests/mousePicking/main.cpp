#include "../common/gui.h"
#include <source/components/lighting/LightingManager.h>
#include <source/components/renderingManager/renderer3D/MousePicker.h>
#include <source/components/assets/objects/Model.h>
#include <source/components/assets/objects/RenderObject.h>
#include <source/components/window/Window.h>
#include <source/components/assets/AssetManager.h>
#include <source/components/pipelines/implementations/common/PipelineTypes.h>
#include <source/VulkanEngine.h>
#include <imgui.h>
#include <array>
#include <iostream>
#include <span>
#include <vector>

struct MousePickingObject {
  std::shared_ptr<vke::RenderObject> object;
  bool hovering = false;
  bool selected = false;
  vke::PipelineType pipelineType = vke::PipelineType::object;
};

void setupScene(const vke::VulkanEngine& renderer,
                std::vector<MousePickingObject>& mousePickingObjects,
                std::vector<std::shared_ptr<vke::RenderObject>>& objects,
                std::vector<std::shared_ptr<vke::Light>>& lights);

std::shared_ptr<vke::Model> createOctahedron(const vke::VulkanEngine& renderer);

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
        for (auto& [object, hovering, selected, pipelineType] : mousePickingObjects)
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

  // A flat quad with a transparent-cornered ring icon, cut out by the textured plane pipeline.
  const auto iconTexture = renderer.getAssetManager()->loadTexture("assets/textures/icon_ring.png");
  const auto iconModel = renderer.getAssetManager()->loadModel("assets/models/curtain.glb");

  const auto icon = renderer.getAssetManager()->loadRenderObject(iconTexture, specularMap, iconModel);
  icon->setPosition({ 0, 0, 0 });
  icon->setScale(0.25f);
  mousePickingObjects.push_back({ icon, false, false, vke::PipelineType::texturedPlane });
  objects.push_back(icon);

  // A model built in code instead of loaded from a file.
  const auto octahedronObject = renderer.getAssetManager()->loadRenderObject(texture, specularMap, createOctahedron(renderer));
  octahedronObject->setPosition({ 3, 0, 3 });
  mousePickingObjects.push_back({ octahedronObject });
  objects.push_back(octahedronObject);

  lights.push_back(renderer.getLightingManager()->createPointLight({0, -3.5f, 0}, {1.0f, 1.0f, 1.0f}, 0.1f, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createPointLight({5.0f, -3.5f, 5.0f}, {1.0f, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createPointLight({-5.0f, -3.5f, -5.0f}, {0.5f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createPointLight({5.0f, -3.5f, -5.0f}, {0, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.getLightingManager()->createPointLight({-5.0f, -3.5f, 5.0f}, {1.0f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));
}

// Flat shaded: each of the 8 faces has its own 3 vertices and its face normal.
std::shared_ptr<vke::Model> createOctahedron(const vke::VulkanEngine& renderer)
{
  constexpr std::array<glm::vec3, 6> corners {{
    { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 }
  }};

  std::vector<vke::Vertex> vertices;
  std::vector<uint32_t> indices;
  vertices.reserve(24);
  indices.reserve(24);

  for (const int x : { 0, 1 })
  {
    for (const int y : { 2, 3 })
    {
      for (const int z : { 4, 5 })
      {
        const glm::vec3 normal = glm::normalize(corners[x] + corners[y] + corners[z]);

        // Counter-clockwise seen from outside, flipped when the three corners come out the other way.
        const bool flip = glm::dot(glm::cross(corners[y] - corners[x], corners[z] - corners[x]), normal) < 0.0f;
        const std::array<int, 3> face = flip ? std::array{ x, z, y } : std::array{ x, y, z };

        constexpr std::array<glm::vec2, 3> texCoords {{ { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.5f, 1.0f } }};

        for (int i = 0; i < 3; i++)
        {
          indices.push_back(static_cast<uint32_t>(vertices.size()));
          vertices.push_back({ .pos = corners[face[i]], .normal = normal, .texCoord = texCoords[i] });
        }
      }
    }
  }

  return renderer.getAssetManager()->createModel(vertices, indices);
}

void renderScene(vke::VulkanEngine& renderer,
                 std::vector<MousePickingObject>& objects,
                 const std::vector<std::shared_ptr<vke::Light>>& lights)
{
  const auto r3d = renderer.getRenderingManager()->getRenderer3D();

  // Render GUI
  ImGui::Begin("Selected Object");
  for (auto& [object, hovering, selected, pipelineType] : objects)
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
  for (auto& [object, hovering, selected, pipelineType] : objects)
  {
    if (selected)
    {
      r3d->renderOutline(object, { 1.0f, 0.5f, 0.0f, 1.0f });
    }
    else if (hovering)
    {
      r3d->renderOutline(object, { 1.0f, 1.0f, 0.0f, 1.0f });
    }

    r3d->renderObject(object, pipelineType, &hovering);
  }

  for (const auto& light : lights)
  {
    renderer.getLightingManager()->renderLight(light);
  }

  // Render Frame
  renderer.render();
}