#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 payload;

void main()
{
  // Return a flat color on hit to confirm geometry is being traced
  payload = vec3(1.0, 0.5, 0.0);
}
