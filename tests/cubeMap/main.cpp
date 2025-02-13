#include <iostream>
#include <source/VulkanEngine.h>
#include <source/objects/RenderObject.h>
#include <imgui.h>
#include <string>
#include <glm/gtc/type_ptr.hpp>

void displayObjectGui(const std::shared_ptr<RenderObject>& object, int id);
void displayLightGui(const std::shared_ptr<Light>& light, int id);

int main()
{
  try
  {
    constexpr VulkanEngineOptions vulkanEngineOptions {
      .WINDOW_WIDTH = 800,
      .WINDOW_HEIGHT = 600,
      .WINDOW_TITLE = "Cube Map",
      .CAMERA_POSITION = { 0.0f, 0.0f, -5.0f },
      .DO_DOTS = false
    };

    VulkanEngine renderer(vulkanEngineOptions);

    ImGui::SetCurrentContext(VulkanEngine::getImGuiContext());

    const auto texture = renderer.loadTexture("assets/textures/white.png");
    const auto specularMap = renderer.loadTexture("assets/textures/blank_specular.png");
    const auto model = renderer.loadModel("assets/models/square.glb");

    const auto object = renderer.loadRenderObject(texture, specularMap, model);
    object->setPosition({ 0, -5, 0 });

    bool useNoisyEllipticalDots = true;

    while (renderer.isActive())
    {
      // Render GUI
      ImGui::Begin("Objects");
      displayObjectGui(object, 0);
      ImGui::End();

      ImGui::Begin("Rendering");
      ImGui::Checkbox("Use Cube Map", &useNoisyEllipticalDots);
      ImGui::End();

      // Render Objects
      renderer.renderObject(object, useNoisyEllipticalDots ? PipelineType::cubeMap : PipelineType::object);

      // Render Frame
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
