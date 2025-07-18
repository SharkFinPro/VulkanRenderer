#include <source/objects/RenderObject.h>
#include <source/VulkanEngine.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <iostream>
#include <string>

void displayObjectGui(const std::shared_ptr<RenderObject>& object, int id);
void displayLightGui(const std::shared_ptr<Light>& light, int id);
void renderScene(VulkanEngine& renderer, const std::shared_ptr<ImGuiInstance>& gui,
                 const std::shared_ptr<RenderObject>& object, const std::vector<std::shared_ptr<Light>>& lights,
                 bool& useCurtain);

int main()
{
  try
  {
    constexpr VulkanEngineOptions vulkanEngineOptions {
      .WINDOW_WIDTH = 800,
      .WINDOW_HEIGHT = 600,
      .WINDOW_TITLE = "Curtain",
      .CAMERA_POSITION = { 0.0f, 0.0f, -15.0f },
      .DO_DOTS = false
    };

    VulkanEngine renderer(vulkanEngineOptions);
    const auto gui = renderer.getImGuiInstance();

    ImGui::SetCurrentContext(VulkanEngine::getImGuiContext());

    const auto texture = renderer.loadTexture("assets/textures/white.png");
    const auto specularMap = renderer.loadTexture("assets/textures/blank_specular.png");
    const auto model = renderer.loadModel("assets/models/curtain.glb");

    const auto object = renderer.loadRenderObject(texture, specularMap, model);

    std::vector<std::shared_ptr<Light>> lights;

    lights.push_back(renderer.createLight({0, -3.5f, 0}, {1.0f, 1.0f, 1.0f}, 0.1f, 0.5f, 1.0f));

    lights.push_back(renderer.createLight({5.0f, -3.5f, 5.0f}, {1.0f, 1.0f, 0}, 0, 0.5f, 1.0f));

    lights.push_back(renderer.createLight({-5.0f, -3.5f, -5.0f}, {0.5f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));

    lights.push_back(renderer.createLight({5.0f, -3.5f, -5.0f}, {0, 1.0f, 0}, 0, 0.5f, 1.0f));

    lights.push_back(renderer.createLight({-5.0f, -3.5f, 5.0f}, {1.0f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));

    bool useCurtain = true;

    while (renderer.isActive())
    {
      renderScene(renderer, gui, object, lights, useCurtain);
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void displayObjectGui(const std::shared_ptr<RenderObject>& object, const int id)
{
  glm::vec3 position = object->getPosition();

  ImGui::PushID(id);

  if (ImGui::CollapsingHeader(("Object " + std::to_string(id)).c_str()))
  {
    ImGui::SliderFloat3("Position", value_ptr(position), -50.0f, 50.0f);
  }

  ImGui::PopID();

  object->setPosition(position);
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

void renderScene(VulkanEngine& renderer, const std::shared_ptr<ImGuiInstance>& gui,
                 const std::shared_ptr<RenderObject>& object, const std::vector<std::shared_ptr<Light>>& lights,
                 bool& useCurtain)
{
  gui->dockCenter("SceneView");
  gui->dockBottom("Objects");
  gui->dockBottom("Lights");
  gui->dockBottom("Rendering");
  gui->dockBottom("Curtain");

  gui->setBottomDockPercent(0.3);

  // Render GUI
  ImGui::Begin("Objects");
  displayObjectGui(object, 0);
  ImGui::End();

  ImGui::Begin("Lights");
  for (int i = 0; i < lights.size(); i++)
  {
    displayLightGui(lights[i], i);
  }
  ImGui::End();

  ImGui::Begin("Rendering");
  ImGui::Checkbox("Use Curtain", &useCurtain);
  ImGui::End();


  // Render Objects
  renderer.renderObject(object, useCurtain ? PipelineType::curtain : PipelineType::object);

  for (const auto& light : lights)
  {
    renderer.renderLight(light);
  }

  // Render Frame
  renderer.render();
}