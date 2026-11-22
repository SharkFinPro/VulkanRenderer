// A flat albedo by material until surfaces get textured materials
vec3 volumePaletteAlbedo(uint material)
{
  const vec3 palette[8] = vec3[](
    vec3(0.60, 0.60, 0.60),
    vec3(0.55, 0.75, 0.40),
    vec3(0.70, 0.55, 0.35),
    vec3(0.55, 0.60, 0.80),
    vec3(0.85, 0.80, 0.55),
    vec3(0.80, 0.50, 0.50),
    vec3(0.50, 0.75, 0.75),
    vec3(0.75, 0.60, 0.80)
  );

  return palette[material % 8];
}
