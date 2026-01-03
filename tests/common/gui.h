#ifndef VKE_TESTS_GUI_H
#define VKE_TESTS_GUI_H

#include <source/components/assets/objects/RenderObject.h>
#include <source/components/imGui/ImGuiInstance.h>
#include <source/components/lighting/lights/Light.h>
#include <source/components/lighting/lights/SpotLight.h>
#include <source/components/pipelines/implementations/BendyPipeline.h>
#include <source/components/renderingManager/RenderingManager.h>
#include <source/components/renderingManager/renderer3D/Renderer3D.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <memory>
#include <string>

inline void displayBendyPlantGui(vke::BendyPlant& bendyPlant,
                                 const int id)
{
  ImGui::PushID(id);

  if (ImGui::CollapsingHeader(("Bending Plant " + std::to_string(id)).c_str()))
  {
    ImGui::SliderInt("# Fins", &bendyPlant.numFins, 0, 35);
    ImGui::SliderInt("Leaf Length", &bendyPlant.leafLength, 0, 10);
    ImGui::SliderFloat3("Position", &bendyPlant.position.x, -10, 10);
    ImGui::SliderFloat("Pitch", &bendyPlant.pitch, -90, 90);
    ImGui::SliderFloat("Bend Strength", &bendyPlant.bendStrength, -0.5, 0.5);
  }

  ImGui::PopID();
}

inline void displayBendyPlantGuis(std::vector<vke::BendyPlant>& bendyPlants)
{
  ImGui::Begin("Bendy Plants");
  for (int i = 0; i < bendyPlants.size(); i++)
  {
    displayBendyPlantGui(bendyPlants[i], i);
  }
  ImGui::End();
}

inline void displayObjectGui(const std::shared_ptr<vke::RenderObject>& object,
                             const int id)
{
  ImGui::PushID(id);

  if (ImGui::CollapsingHeader(("Object " + std::to_string(id)).c_str()))
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

  ImGui::PopID();
}

inline void displayLightGui(const std::shared_ptr<vke::Light>& light,
                            const int id)
{
  std::shared_ptr<vke::SpotLight> spotLight = nullptr;

  glm::vec3 position = light->getPosition();
  glm::vec3 color = light->getColor();
  float ambient = light->getAmbient();
  float diffuse = light->getDiffuse();
  float specular = light->getSpecular();
  bool isSpotLight = light->getLightType() == vke::LightType::spotLight;

  if (isSpotLight)
  {
    spotLight = std::dynamic_pointer_cast<vke::SpotLight>(light);
  }

  glm::vec3 direction = isSpotLight ? spotLight->getDirection() : glm::vec3(0.0f);
  float coneAngle = isSpotLight ? spotLight->getConeAngle() : 0.0f;

  ImGui::PushID(id);

  if (ImGui::CollapsingHeader(("Light " + std::to_string(id)).c_str()))
  {
    ImGui::Checkbox("Spot Light", &isSpotLight);
    ImGui::ColorEdit3("Color", value_ptr(color));
    ImGui::SliderFloat("Ambient", &ambient, 0.0f, 1.0f);
    ImGui::SliderFloat("Diffuse", &diffuse, 0.0f, 1.0f);
    ImGui::SliderFloat("Specular", &specular, 0.0f, 1.0f);
    ImGui::SliderFloat3("Position", value_ptr(position), -50.0f, 50.0f);
    ImGui::SliderFloat3("Direction", value_ptr(direction), -1.0f, 1.0f);
    ImGui::SliderFloat("Cone Angle", &coneAngle, 0.0f, 180.0f);
    ImGui::Separator();
  }

  ImGui::PopID();

  light->setPosition(position);
  light->setColor(color);
  light->setAmbient(ambient);
  light->setDiffuse(diffuse);
  light->setSpecular(specular);
  if (isSpotLight)
  {
    spotLight->setDirection(direction);
    spotLight->setConeAngle(coneAngle);
  }
}

inline void displayLightGuis(const std::vector<std::shared_ptr<vke::Light>>& lights)
{
  ImGui::Begin("Lights");
  for (int i = 0; i < lights.size(); i++)
  {
    displayLightGui(lights[i], i);
  }
  ImGui::End();
}

inline void displayObjectGuis(const std::vector<std::shared_ptr<vke::RenderObject>>& objects)
{
  ImGui::Begin("Objects");
  for (int i = 0; i < objects.size(); i++)
  {
    displayObjectGui(objects[i], i);
  }
  ImGui::End();
}

inline void setDockOptions(const std::shared_ptr<vke::ImGuiInstance>& gui)
{
  gui->dockCenter("Scene View");

  gui->dockBottom("Scene Options");

  gui->dockBottom("Bendy Plants");
  gui->dockBottom("Bumpy Curtain");
  gui->dockBottom("Chroma Depth");
  gui->dockBottom("Crosses");
  gui->dockBottom("Curtain");
  gui->dockBottom("Elliptical Dots");
  gui->dockBottom("Lights");
  gui->dockBottom("Magnify Whirl Mosaic");
  gui->dockBottom("Noisy Elliptical Dots");
  gui->dockBottom("Objects");
  gui->dockBottom("Rendering");
  gui->dockBottom("Snake");

  gui->setBottomDockPercent(0.3);
}

inline void displaySceneOptions(const std::shared_ptr<vke::RenderingManager>& renderingManager)
{
  ImGui::Begin("Scene Options");

  bool showGrid = renderingManager->getRenderer3D()->isGridEnabled();

  if (ImGui::Checkbox("Show Grid", &showGrid))
  {
    if (showGrid)
    {
      renderingManager->getRenderer3D()->enableGrid();
    }
    else
    {
      renderingManager->getRenderer3D()->disableGrid();
    }
  }

  ImGui::End();
}

inline void displayGui(const std::shared_ptr<vke::ImGuiInstance>& gui,
                       const std::vector<std::shared_ptr<vke::Light>>& lights,
                       const std::vector<std::shared_ptr<vke::RenderObject>>& objects,
                       const std::shared_ptr<vke::RenderingManager>& renderingManager)
{
  setDockOptions(gui);

  displayObjectGuis(objects);

  displayLightGuis(lights);

  displaySceneOptions(renderingManager);
}

#endif //VKE_TESTS_GUI_H
