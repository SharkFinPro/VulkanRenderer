#version 450

layout(push_constant) uniform PushConstants {
    mat4 lightViewProj;
} pc;

layout(set = 0, binding = 0) uniform Transform {
    mat4 model;
} transform;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

void main() {
    gl_Position = pc.lightViewProj * transform.model * vec4(inPosition, 1.0);
}