#include "Light.h"
#include <imgui.h>
#include <string>
#include "glm/gtc/type_ptr.hpp"

Light::Light(const glm::vec3 position, const glm::vec3 color, const float ambient, const float diffuse,
             const float specular)
  : position(position), color(color), ambient(ambient), diffuse(diffuse), specular(specular)
{}

void Light::displayGui(const int id)
{
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
}

LightUniform Light::getUniform() const
{
  return {
    .position = position,
    .color = color,
    .ambient = ambient,
    .diffuse = diffuse,
    .specular = specular
  };
}
