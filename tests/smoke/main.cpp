#include <source/objects/RenderObject.h>
#include <source/VulkanEngine.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <iostream>
#include <string>

void displayObjectGui(const std::shared_ptr<RenderObject>& object, int id);
void displayLightGui(const std::shared_ptr<Light>& light, int id);
void createLights(VulkanEngine& renderer, std::vector<std::shared_ptr<Light>>& lights);
void createSmokeSystems(VulkanEngine& renderer);
void setDockOptions(const std::shared_ptr<ImGuiInstance>& gui);
void displayGui(const std::shared_ptr<ImGuiInstance>& gui, const std::vector<std::shared_ptr<Light>>& lights,
                const std::shared_ptr<RenderObject>& object);

int main()
{
  try
  {
    constexpr VulkanEngineOptions vulkanEngineOptions {
      .WINDOW_WIDTH = 800,
      .WINDOW_HEIGHT = 600,
      .WINDOW_TITLE = "Smoke",
      .CAMERA_POSITION = { 0.0f, 5.0f, -15.0f },
      .DO_DOTS = false
    };

    VulkanEngine renderer(vulkanEngineOptions);
    const auto gui = renderer.getImGuiInstance();

    ImGui::SetCurrentContext(VulkanEngine::getImGuiContext());

    const auto texture = renderer.loadTexture("assets/textures/white.png");
    const auto specularMap = renderer.loadTexture("assets/textures/blank_specular.png");
    const auto model = renderer.loadModel("assets/models/square.glb");

    const auto object = renderer.loadRenderObject(texture, specularMap, model);
    object->setPosition({ 0, 0, 0 });

    std::vector<std::shared_ptr<Light>> lights;
    createLights(renderer, lights);

    createSmokeSystems(renderer);

    while (renderer.isActive())
    {
      displayGui(gui, lights, object);

      renderer.renderObject(object, PipelineType::object);

      for (const auto& light : lights)
      {
        renderer.renderLight(light);
      }

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

void createLights(VulkanEngine& renderer, std::vector<std::shared_ptr<Light>>& lights)
{
  lights.push_back(renderer.createLight({0, 1.5f, 0}, {1.0f, 1.0f, 1.0f}, 0.1f, 0.5f, 1.0f));

  lights.push_back(renderer.createLight({5.0f, 1.5f, 5.0f}, {1.0f, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.createLight({-5.0f, 1.5f, -5.0f}, {0.5f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.createLight({5.0f, 1.5f, -5.0f}, {0, 1.0f, 0}, 0, 0.5f, 1.0f));

  lights.push_back(renderer.createLight({-5.0f, 1.5f, 5.0f}, {1.0f, 0.5f, 1.0f}, 0, 0.5f, 1.0f));
}

void createSmokeSystems(VulkanEngine& renderer)
{
  constexpr uint32_t numParticles = 2'500'000;

  renderer.createSmokeSystem({0, 0.95f, 0}, numParticles);
  renderer.createSmokeSystem({-5, 0.95f, -5}, numParticles * 2);
  renderer.createSmokeSystem({-5, 0.95f, 5}, numParticles / 2);
  renderer.createSmokeSystem({5, .95f, 5}, numParticles * 2);
  renderer.createSmokeSystem({5, 0.95f, -5}, numParticles / 2);
}

void setDockOptions(const std::shared_ptr<ImGuiInstance>& gui)
{
  gui->dockCenter("SceneView");
  gui->dockBottom("Objects");
  gui->dockBottom("Lights");
  gui->dockBottom("Smoke");

  gui->setBottomDockPercent(0.3);
}

void displayGui(const std::shared_ptr<ImGuiInstance>& gui, const std::vector<std::shared_ptr<Light>>& lights,
                const std::shared_ptr<RenderObject>& object)
{
  setDockOptions(gui);

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
}