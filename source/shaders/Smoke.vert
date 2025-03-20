#version 450

layout(set = 0, binding = 3) uniform Transform {
  mat4 model;
  mat4 view;
  mat4 proj;
} transform;

layout(location = 0) in vec4 inPositionTtl;
layout(location = 1) in vec4 inVelocityColor;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec4 fragColor;

const float TTL = 8.0f;

void main()
{
  if (inPositionTtl.w < 0)
  {
    gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
    return;
  }

  vec4 modelTransform = transform.model * vec4(inPositionTtl.xyz, 1.0);

  vec4 viewPos = transform.view * modelTransform;

  float basePointSize = 7.0;
  float distanceScale = 1.0 / -viewPos.z;

  gl_PointSize = clamp(basePointSize * distanceScale, 1.0, 20.0);
  gl_Position = transform.proj * viewPos;


  fragColor = vec4(inVelocityColor.w, inVelocityColor.w, inVelocityColor.w, sqrt((TTL - inPositionTtl.w) / TTL));

  fragPos = vec3(modelTransform);
}