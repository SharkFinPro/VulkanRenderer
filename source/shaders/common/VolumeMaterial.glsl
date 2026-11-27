// Triplanar blending of volume surface materials, shared by the raster and ray traced shading.

// VolumeMaterial.h mirrors these
const float triplanarSharpness = 4.0;
const float triplanarMinWeight = 0.02;

// Palette entries per surface (s_maxVolumeMaterials)
const int maxVolumeMaterials = 16;

vec3 triplanarWeights(vec3 normal)
{
  vec3 weights = pow(abs(normal), vec3(triplanarSharpness));
  weights /= weights.x + weights.y + weights.z;
  weights = mix(weights, vec3(0.0), lessThan(weights, vec3(triplanarMinWeight)));

  return weights / (weights.x + weights.y + weights.z);
}

// Texture coordinates for the projection along an axis. Image v runs down, so the side projections take -y to keep
// pictures upright on walls. Linear, so it maps the position's derivatives the same way.
vec2 projectAxis(int axis, vec3 v)
{
  return axis == 0 ? vec2(v.z, -v.y) : axis == 1 ? v.xz : vec2(v.x, -v.y);
}
