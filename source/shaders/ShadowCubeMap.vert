#version 450
#extension GL_EXT_multiview : require

layout(set = 0, binding = 0) uniform Transform {
  mat4 model;
} transform;

layout(set = 1, binding = 0) uniform Shadow {
  mat4 lightViewProj[6];
} shadow;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPos;

void main() {
  fragPos = (transform.model * vec4(inPosition, 1.0)).xyz;

  gl_Position = shadow.lightViewProj[gl_ViewIndex] * transform.model * vec4(inPosition, 1.0);
}