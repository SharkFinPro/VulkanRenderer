#version 450

layout(push_constant) uniform GridPC {
  mat4 viewProj;
  vec3 viewPosition;
} pc;

layout(location = 0) in vec3 nearPoint;
layout(location = 1) in vec3 farPoint;

layout(location = 0) out vec4 outColor;

vec4 gridColor(vec3 position, int minor, int major)
{
  float lineWidth = 1.0;
  float axisWidth = 2.0;

  // Axis Highlights
  vec2 axisDistance = abs(position.xz) / fwidth(position.xz);

  float axisAlphaX = 1.0 - smoothstep(0.0, axisWidth, axisDistance.y);
  if (axisAlphaX > 0.0)
  {
    return vec4(1, 0, 0, 0.9 * axisAlphaX);
  }

  float axisAlphaZ = 1.0 - smoothstep(0.0, axisWidth, axisDistance.x);
  if (axisAlphaZ > 0.0)
  {
    return vec4(0, 0, 1, 0.9 * axisAlphaZ);
  }

  // Major Grid
  vec2 majorPos = position.xz / (minor * major);
  vec2 majorGrid = abs(fract(majorPos - 0.5) - 0.5) / fwidth(majorPos);
  float majorLine = min(majorGrid.x, majorGrid.y);
  float majorAlpha = 1.0 - smoothstep(0.0, lineWidth, majorLine);

  if (majorAlpha > 0.0)
  {
    return vec4(1, 1, 1, 0.3 * majorAlpha);
  }

  // Minor Grid
  vec2 minorPos = position.xz / minor;
  vec2 minorGrid = abs(fract(minorPos - 0.5) - 0.5) / fwidth(minorPos);
  float minorLine = min(minorGrid.x, minorGrid.y);
  float minorAlpha = 1.0 - smoothstep(0.0, lineWidth, minorLine);

  if (minorAlpha > 0.0)
  {
    return vec4(1, 1, 1, 0.1 * minorAlpha);
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

  vec4 clipSpacePosition = pc.viewProj * vec4(fragPosition.xyz, 1.0);
  gl_FragDepth = clipSpacePosition.z / clipSpacePosition.w;

  float dist = distance(fragPosition, pc.viewPosition);
  float fadeout = exp(-dist * 0.0025);

  outColor = gridColor(fragPosition, 1, 10) * fadeout;
}