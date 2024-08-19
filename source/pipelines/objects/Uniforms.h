#ifndef VULKANPROJECT_UNIFORMS_H
#define VULKANPROJECT_UNIFORMS_H

#include <glm/glm.hpp>
#include <imgui.h>
#include <string>

struct TransformUniform {
  alignas(16) glm::mat4 model;
  alignas(16) glm::mat4 view;
  alignas(16) glm::mat4 proj;
};

struct alignas(16) Light {
  float position[3];  // 12 bytes
  float padding1;      // 4 bytes to align the next member to a 16-byte boundary
  float color[3];     // 12 bytes
  float padding2;      // 4 bytes, to make the struct size a multiple of 16 bytes

  float ambient;       // 4 bytes
  float diffuse;       // 4 bytes
  float specular;      // 4 bytes
  float padding3;      // 4 bytes, to make the struct size a multiple of 16 bytes

  void displayGui(const size_t lightNum)
  {
    ImGui::Begin(("Light " + std::to_string(lightNum)).c_str());
    ImGui::Text("Control Light:");
    ImGui::ColorEdit3("Color", color);
    ImGui::SliderFloat("Ambient", &ambient, 0.0f, 1.0f);
    ImGui::SliderFloat("Diffuse", &diffuse, 0.0f, 1.0f);
    ImGui::SliderFloat3("Position", position, -50.0f, 50.0f);
    ImGui::End();
  }
};

struct LightMetadataUniform {
  alignas(16) int numLights;
};

struct LightsUniform {
  Light lights[5];  // Fixed-size array, aligned to 16 bytes
};

struct CameraUniform {
  alignas(16) glm::vec3 position;
};

#endif //VULKANPROJECT_UNIFORMS_H
