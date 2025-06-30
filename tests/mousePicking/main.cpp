#include <iostream>
#include <source/VulkanEngine.h>
#include <source/objects/RenderObject.h>
#include <imgui.h>
#include <string>

struct MousePickingObject {
  std::shared_ptr<RenderObject> object;
  bool hovering = false;
  bool selected = false;
};

void displayObjectGui(const std::shared_ptr<RenderObject>& object);
void displayLightGui(const std::shared_ptr<Light>& light, int id);
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
      if (renderer.canMousePick() && renderer.buttonIsPressed(GLFW_MOUSE_BUTTON_LEFT))
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

void displayObjectGui(const std::shared_ptr<RenderObject>& object)
{
  glm::vec3 position = object->getPosition();
  ImGui::SliderFloat3("Position", value_ptr(position), -50.0f, 50.0f);
  object->setPosition(position);

  glm::vec3 scale = object->getScale();
  ImGui::SliderFloat3("Scale", value_ptr(scale), 0.01f, 50.0f);
  object->setScale(scale);

  glm::vec3 rotation = object->getOrientationEuler();
  ImGui::SliderFloat3("Rotation", value_ptr(rotation), -90.0f, 90.0f);
  object->setOrientationEuler(rotation);
}

void displayLightGui(const std::shared_ptr<Light>& light, const int id)
{
  glm::vec3 position = light->getPosition();
  glm::vec3 color = light->getColor();
  float ambient = light->getAmbient();
  float diffuse = light->getDiffuse();
  float specular = light->getSpecular();

  ImGui::PushID(id);

  if (ImGui::CollapsingHeader(("Light " + std::to_string(id)).c_str()))
  {
    ImGui::ColorEdit3("Color", value_ptr(color));
    ImGui::SliderFloat("Ambient", &ambient, 0.0f, 1.0f);
    ImGui::SliderFloat("Diffuse", &diffuse, 0.0f, 1.0f);
    ImGui::SliderFloat("Specular", &specular, 0.0f, 1.0f);
    ImGui::SliderFloat3("Position", value_ptr(position), -50.0f, 50.0f);
    ImGui::Separator();
  }

  ImGui::PopID();

  light->setPosition(position);
  light->setColor(color);
  light->setAmbient(ambient);
  light->setDiffuse(diffuse);
  light->setSpecular(specular);
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


  lights.push_back(renderer.createLight({0, -3.5f, 0}, {1.0f, 1.0f, 1.0f}, 0.1f, 0.5f, 1.0f));

  lights.push_back(renderer.createLight({5.0f, -3.5f, 5.0f}, {1.0f, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.createLight({-5.0f, -3.5f, -5.0f}, {0.5f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.createLight({5.0f, -3.5f, -5.0f}, {0, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.createLight({-5.0f, -3.5f, 5.0f}, {1.0f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));
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
      displayObjectGui(object);
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
    renderer.renderLight(light);
  }

  // Render Frame
  renderer.render();
}