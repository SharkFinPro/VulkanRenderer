#ifndef GUI_H
#define GUI_H

#include <source/components/lighting/Light.h>
#include <source/objects/RenderObject.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <memory>
#include <string>

inline void displayObjectGui(const std::shared_ptr<RenderObject>& object, const int id)
{
  glm::vec3 position = object->getPosition();

  ImGui::PushID(id);

  if (ImGui::CollapsingHeader(("Object " + std::to_string(id)).c_str()))
  {
    ImGui::SliderFloat3("Position", glm::value_ptr(position), -50.0f, 50.0f);
  }

  ImGui::PopID();

  object->setPosition(position);
}

inline void displayLightGui(const std::shared_ptr<Light>& light, const int id)
{
  glm::vec3 position = light->getPosition();
  glm::vec3 color = light->getColor();
  float ambient = light->getAmbient();
  float diffuse = light->getDiffuse();
  float specular = light->getSpecular();

  ImGui::PushID(id);

  if (ImGui::CollapsingHeader(("Light " + std::to_string(id)).c_str()))
  {
    ImGui::ColorEdit3("Color", glm::value_ptr(color));
    ImGui::SliderFloat("Ambient", &ambient, 0.0f, 1.0f);
    ImGui::SliderFloat("Diffuse", &diffuse, 0.0f, 1.0f);
    ImGui::SliderFloat("Specular", &specular, 0.0f, 1.0f);
    ImGui::SliderFloat3("Position", glm::value_ptr(position), -50.0f, 50.0f);
    ImGui::Separator();
  }

  ImGui::PopID();

  light->setPosition(position);
  light->setColor(color);
  light->setAmbient(ambient);
  light->setDiffuse(diffuse);
  light->setSpecular(specular);
}

inline void displayLightGuis(const std::vector<std::shared_ptr<Light>>& lights)
{
  ImGui::Begin("Lights");
  for (int i = 0; i < lights.size(); i++)
  {
    displayLightGui(lights[i], i);
  }
  ImGui::End();
}

#endif //GUI_H
