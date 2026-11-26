#version 450
#extension GL_GOOGLE_include_directive : require
#include "../common/SceneData.glsl"

layout(push_constant) uniform PushConstants {
    mat4 lightViewProj;
} pc;

layout(location = 0) in vec3 inPosition;

void main() {
    // Transform the vector twice rather than multiply the matrices: left to right, lightViewProj * model is a 4x4 matrix
    // product per vertex (64 multiply-adds) before the vector multiply, against 32 this way
    gl_Position = pc.lightViewProj * (objects[objectSlot()].model * vec4(inPosition, 1.0));
}
