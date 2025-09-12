#version 450

layout(set = 0, binding = 0) uniform Grid {
  mat4 view;
  mat4 proj;
  vec3 viewPosition;
} grid;

layout(location = 0) in vec3 nearPoint;
layout(location = 1) in vec3 farPoint;

layout(location = 0) out vec4 outColor;

vec4 gridColor(vec3 position, int minor, int major)
{
  float lineWidth = 0.5;
  float axisWidth = 1.0;

  vec2 axisDistance = abs(position.xz) / fwidth(position.xz);

  if (axisDistance.y < axisWidth)
  {
    return vec4(1, 0, 0, 0.9);
  }

  if (axisDistance.x < axisWidth)
  {
    return vec4(0, 0, 1, 0.9);
  }

  vec2 majorGrid = abs(fract(position.xz / (minor * major) - 0.5) - 0.5) / fwidth(position.xz / (minor * major));
  float majorLine = min(majorGrid.x, majorGrid.y);

  if (majorLine < lineWidth)
  {
    return vec4(1, 1, 1, 0.3); // Bright white for major grid lines
  }

  vec2 minorGrid = abs(fract(position.xz / minor - 0.5) - 0.5) / fwidth(position.xz / minor);
  float minorLine = min(minorGrid.x, minorGrid.y);

  if (minorLine < lineWidth)
  {
    return vec4(1, 1, 1, 0.1); // Dimmer gray for minor grid lines
  }

  return vec4(0, 0, 0, 0);
}

void main() {
  float t = -nearPoint.y / (farPoint.y - nearPoint.y);

  if (t <= 0)
  {
    discard;
  }

  vec3 fragPosition = nearPoint + t * (farPoint - nearPoint);

  vec4 clipSpacePosition = grid.proj * grid.view * vec4(fragPosition.xyz, 1.0);
  gl_FragDepth = clipSpacePosition.z / clipSpacePosition.w;

  float dist = distance(fragPosition, grid.viewPosition);
  float fadeout = exp(-dist * 0.0025);

  outColor = gridColor(fragPosition, 1, 10) * fadeout;
}