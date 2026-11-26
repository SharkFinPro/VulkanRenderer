// The lighting set (set 2) of the lit object pipelines and the shadow lookups on it. The including shader enables
// GL_EXT_nonuniform_qualifier, GL_EXT_texture_shadow_lod (textureLod on the directional maps' array shadow sampler) and
// GL_GOOGLE_include_directive.
#include "Lighting.glsl"

layout(set = 2, binding = 0) uniform PointLightsMetadata {
  int numPointLights;
  int numSpotLights;
  int numDirectionalLights;
};

layout(set = 2, binding = 1) readonly buffer PointLights {
  PointLight pointLights[];
};

layout(set = 2, binding = 2) readonly buffer SpotLights {
  SpotLight spotLights[];
};

layout(set = 2, binding = 3) uniform Camera {
  vec3 position;
} camera;

layout(set = 2, binding = 4) uniform sampler2DShadow[] spotLightShadowMaps;

layout(set = 2, binding = 5) uniform samplerCubeShadow[] pointLightShadowMaps;

layout(set = 2, binding = 6) readonly buffer DirectionalLights {
  DirectionalLight directionalLights[];
};

layout(set = 2, binding = 7) uniform sampler2DArrayShadow[] directionalLightShadowMaps;

// The shadow lookups below move the fragment off its surface with shadowLookupPosition (the light's ShadowBias, in texels
// of the map at the fragment), so no depth bias is needed. normal is the unit shading normal. They return the lit
// fraction, filtered: the light's filter radius (shadowFilter.x, in texels) spreads shadowFilterTaps samples over a disc
// in the plane facing the light (ShadowFilter.h mirrors the offsets on the CPU); a radius of 0 takes one sample.

const int shadowFilterTaps = 16;

const vec2 shadowFilterDisc[shadowFilterTaps] = vec2[shadowFilterTaps](
  vec2(0.032389, 0.062461), vec2(-0.678361, -0.673981), vec2(0.571391, -0.800312), vec2(-0.482854, 0.854702),
  vec2(0.848794, 0.156892), vec2(-0.938362, 0.220159), vec2(0.530101, 0.783952), vec2(0.029889, -0.650125),
  vec2(-0.425273, -0.058974), vec2(-0.046156, 0.544012), vec2(0.855842, -0.370545), vec2(0.386474, 0.335194),
  vec2(0.054615, 0.958519), vec2(0.410468, -0.288637), vec2(-0.934812, -0.243051), vec2(-0.321681, -0.880796));

// A value in [0, 1) that varies from pixel to pixel
float interleavedGradientNoise(vec2 fragCoord)
{
  return fract(52.9829189 * fract(dot(fragCoord, vec2(0.06711056, 0.00583715))));
}

// (cos, sin) of this pixel's rotation of the disc
vec2 shadowFilterRotation()
{
  float angle = interleavedGradientNoise(gl_FragCoord.xy) * 6.28318530718;

  return vec2(cos(angle), sin(angle));
}

// An orthonormal basis of the plane perpendicular to the unit vector toLight
void shadowFilterBasis(vec3 toLight, out vec3 t, out vec3 b)
{
  vec3 absLight = abs(toLight);
  vec3 helper = absLight.x <= absLight.y && absLight.x <= absLight.z ? vec3(1.0, 0.0, 0.0)
              : absLight.y <= absLight.z ? vec3(0.0, 1.0, 0.0)
              : vec3(0.0, 0.0, 1.0);

  t = normalize(cross(toLight, helper));
  b = cross(toLight, t);
}

// The taps' offsets are world space; scale is the radius in texels times the world size of a texel. The disc lies in the
// plane facing the light, so each tap is then slid along the light onto the receiver's plane (the plane through the
// fragment with its normal): on a surface the light grazes, a tap left in the disc's plane would sit below the surface
// and read as shadowed. The slide is clamped like the slope bias.
//
// Everything but the disc point is the same for all of a lookup's taps, and both the offset and its slide are linear in
// that point, so the rotation, the scale, the slide's dot products and its division are folded into a frame once per
// lookup (shadowFilterFrame), and a tap is then two multiply-adds and a clamp. ShadowFilter.h mirrors both forms.
struct ShadowFilterFrame
{
  vec3 t;       // Where the disc's x axis lands: the basis rotated by the pixel's rotation, times scale
  vec3 b;       // Where the disc's y axis lands
  vec3 toLight; // The unit direction the taps slide along
  vec2 slide;   // The unclamped slide per unit of the disc point's x and y
  float limit;  // The slide's clamp per unit of the disc point's length (|offset| = scale * |d|, as t and b are orthonormal)
};

ShadowFilterFrame shadowFilterFrame(vec2 rotation, vec3 t, vec3 b, float scale, vec3 normal, vec3 toLight)
{
  ShadowFilterFrame frame;
  frame.t = (t * rotation.x + b * rotation.y) * scale;
  frame.b = (b * rotation.x - t * rotation.y) * scale;
  frame.toLight = toLight;
  frame.slide = -vec2(dot(frame.t, normal), dot(frame.b, normal)) / max(dot(normal, toLight), 1e-4);

  const float maxTangent = 10.0;
  frame.limit = maxTangent * scale;

  return frame;
}

// The world space offset of one tap
vec3 shadowFilterOffset(int tap, ShadowFilterFrame frame)
{
  vec2 d = shadowFilterDisc[tap];
  float limit = frame.limit * length(d);
  float slide = clamp(dot(d, frame.slide), -limit, limit);

  return frame.t * d.x + frame.b * d.y + frame.toLight * slide;
}

// One comparison sample of a point light's cube map at a world position
float pointShadowSample(int i, int shadowMap, vec3 lookup)
{
  float range = pointLights[i].shadowRange;
  float nearPlane = pointLights[i].shadowNearPlane;

  vec3 fragToLight = pointLights[i].position - lookup;
  fragToLight.xz *= -1.0;

  vec3 absLookup = abs(fragToLight);
  float lookupDepth = max(max(absLookup.x, max(absLookup.y, absLookup.z)), nearPlane);

  // A filter tap can reach past the shadow range, where nothing casts (and the depth below would go negative)
  if (lookupDepth >= range)
  {
    return 1.0;
  }

  // The reversed depth the face's projection gives at that view depth (PointLight::getLightViewProjectionMatrices). The
  // sampler compares greater-or-equal: lit where the fragment is at least as near as what the map holds.
  float ref = nearPlane * (range - lookupDepth) / (lookupDepth * (range - nearPlane));

  return texture(pointLightShadowMaps[nonuniformEXT(shadowMap)], vec4(fragToLight, ref));
}

// 1 where the fragment is lit by the point light, 0 where its shadow map hides it
float pointLightShadow(int i, vec3 fragPos, vec3 normal)
{
  int shadowMap = pointLights[i].shadowMapIndex;

  // A light without a map casts no shadows
  if (shadowMap < 0)
  {
    return 1.0;
  }

  vec3 toLight = pointLights[i].position - fragPos;

  float range = pointLights[i].shadowRange;
  float dist = length(toLight);

  // The shadow casters are culled to the range's sphere, so nothing beyond it can hide the fragment
  if (dist >= range)
  {
    return 1.0;
  }

  // The face a lookup reads is the one of the major axis, and that face's view depth is the distance along it. A texel of
  // a 90 degree face is 2 / mapSize of that depth wide.
  vec3 absToLight = abs(toLight);
  float texel = pointLights[i].shadowBias.w * max(absToLight.x, max(absToLight.y, absToLight.z));
  vec3 unitToLight = toLight / dist;
  vec3 lookup = shadowLookupPosition(fragPos, normal, unitToLight, texel, pointLights[i].shadowBias.xyz);

  float radius = pointLights[i].shadowFilter.x;

  if (radius <= 0.0)
  {
    return pointShadowSample(i, shadowMap, lookup);
  }

  vec3 t;
  vec3 b;
  shadowFilterBasis(unitToLight, t, b);
  vec2 rotation = shadowFilterRotation();
  float scale = radius * texel;
  ShadowFilterFrame frame = shadowFilterFrame(rotation, t, b, scale, normal, unitToLight);
  float sum = 0.0;

  for (int tap = 0; tap < shadowFilterTaps; tap++)
  {
    sum += pointShadowSample(i, shadowMap, lookup + shadowFilterOffset(tap, frame));
  }

  return sum / float(shadowFilterTaps);
}

// One comparison sample of a spot light's map at a world position
float spotShadowSample(int i, int shadowMap, vec3 lookup)
{
  vec4 fragPosLightSpace = spotLights[i].lightViewProjection * vec4(lookup, 1.0);
  vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
  projCoords.xy = projCoords.xy * 0.5 + 0.5;

  // Beyond the far plane (the shadow range) nothing is in the map, so the fragment is lit
  if (projCoords.z > 1.0)
  {
    return 1.0;
  }

  return texture(spotLightShadowMaps[nonuniformEXT(shadowMap)], projCoords);
}

float spotLightShadow(int i, vec3 fragPos, vec3 normal)
{
  int shadowMap = spotLights[i].shadowMapIndex;

  // A light without a map casts no shadows
  if (shadowMap < 0)
  {
    return 1.0;
  }

  // A texel of the map is shadowBias.w wide per unit of depth along the spot's axis
  vec3 toLight = spotLights[i].position - fragPos;
  float viewDepth = max(dot(-toLight, normalize(spotLights[i].direction)), 0.0);
  float texel = spotLights[i].shadowBias.w * viewDepth;
  vec3 unitToLight = normalize(toLight);
  vec3 lookup = shadowLookupPosition(fragPos, normal, unitToLight, texel, spotLights[i].shadowBias.xyz);

  float radius = spotLights[i].shadowFilter.x;

  if (radius <= 0.0)
  {
    return spotShadowSample(i, shadowMap, lookup);
  }

  vec3 t;
  vec3 b;
  shadowFilterBasis(unitToLight, t, b);
  vec2 rotation = shadowFilterRotation();
  float scale = radius * texel;
  ShadowFilterFrame frame = shadowFilterFrame(rotation, t, b, scale, normal, unitToLight);
  float sum = 0.0;

  for (int tap = 0; tap < shadowFilterTaps; tap++)
  {
    sum += spotShadowSample(i, shadowMap, lookup + shadowFilterOffset(tap, frame));
  }

  return sum / float(shadowFilterTaps);
}

// The fragment, moved by the bias, in cascade c of the directional light, in world space
vec3 directionalCascadeLookup(int i, int c, vec3 fragPos, vec3 normal)
{
  return shadowLookupPosition(fragPos, normal, -directionalLights[i].direction,
                              directionalLights[i].cascadeTexelSizes[c], directionalLights[i].shadowBias.xyz);
}

// A world position in the clip space of cascade c: xy mapped to [0, 1], z the depth
vec3 directionalCascadeCoords(int i, int c, vec3 lookup)
{
  vec4 clip = directionalLights[i].cascadeViewProjections[c] * vec4(lookup, 1.0);
  vec3 coords = clip.xyz / clip.w;
  coords.xy = coords.xy * 0.5 + 0.5;

  return coords;
}

bool insideUnitCube(vec3 coords)
{
  return all(greaterThanEqual(coords, vec3(0.0))) && all(lessThanEqual(coords, vec3(1.0)));
}

// One comparison sample of layer c of the directional light's map, at level 0: the map has one level, and this runs in
// non-uniform control flow, where implicit derivatives are undefined. An explicit lod, not explicit gradients: a gradient
// sample is issued at a fraction of the texture unit's rate (and carries four more operands), which made these 16 taps
// most of the frame, while a lod sample is full rate and filters the same single level identically.
float directionalShadowSample(int shadowMap, int c, vec3 coords)
{
  return textureLod(directionalLightShadowMaps[nonuniformEXT(shadowMap)], vec4(coords.xy, float(c), coords.z), 0.0);
}

// The filtered lit fraction in cascade c, given the lookup position and its coordinates there
float directionalCascadeShadow(int i, int shadowMap, int c, vec3 lookup, vec3 coords, vec3 t, vec3 b, vec2 rotation,
                               vec3 normal)
{
  float radius = directionalLights[i].shadowFilter.x;

  if (radius <= 0.0)
  {
    return directionalShadowSample(shadowMap, c, coords);
  }

  float scale = radius * directionalLights[i].cascadeTexelSizes[c];
  ShadowFilterFrame frame = shadowFilterFrame(rotation, t, b, scale, normal, -directionalLights[i].direction);
  float sum = 0.0;

  for (int tap = 0; tap < shadowFilterTaps; tap++)
  {
    vec3 tapOffset = shadowFilterOffset(tap, frame);
    vec3 tapCoords = directionalCascadeCoords(i, c, lookup + tapOffset);
    sum += directionalShadowSample(shadowMap, c, tapCoords);
  }

  return sum / float(shadowFilterTaps);
}

// 1 where the fragment is lit by the directional light, 0 where its shadow map hides it. The map has one layer per
// cascade (DirectionalLight): the first cascade that contains the fragment is sampled, blended with the next near its
// edge, and the shadow fades out over the last tenth of the shadow distance. Fragments beyond it, or in no cascade, are lit.
float directionalLightShadow(int i, vec3 fragPos, vec3 normal)
{
  int shadowMap = directionalLights[i].shadowMapIndex;
  int cascadeCount = directionalLights[i].cascadeCount;

  // A light without a map casts no shadows
  if (shadowMap < 0 || cascadeCount == 0)
  {
    return 1.0;
  }

  float shadowDistance = directionalLights[i].shadowDistance;
  float fade = smoothstep(0.9 * shadowDistance, shadowDistance, length(fragPos - camera.position));

  if (fade >= 1.0)
  {
    return 1.0;
  }

  vec3 t;
  vec3 b;
  shadowFilterBasis(-directionalLights[i].direction, t, b);
  vec2 rotation = shadowFilterRotation();

  for (int c = 0; c < cascadeCount; c++)
  {
    vec3 lookup = directionalCascadeLookup(i, c, fragPos, normal);
    vec3 coords = directionalCascadeCoords(i, c, lookup);

    if (!insideUnitCube(coords))
    {
      continue;
    }

    float shadow = directionalCascadeShadow(i, shadowMap, c, lookup, coords, t, b, rotation, normal);

    // Near the edge of the cascade, blend toward the next one so the change of resolution isn't a visible line
    const float blendWidth = 0.05;
    float edge = min(min(coords.x, 1.0 - coords.x), min(coords.y, 1.0 - coords.y));

    if (edge < blendWidth && c + 1 < cascadeCount)
    {
      vec3 nextLookup = directionalCascadeLookup(i, c + 1, fragPos, normal);
      vec3 nextCoords = directionalCascadeCoords(i, c + 1, nextLookup);

      if (insideUnitCube(nextCoords))
      {
        float nextShadow = directionalCascadeShadow(i, shadowMap, c + 1, nextLookup, nextCoords, t, b, rotation,
                                                     normal);

        shadow = mix(nextShadow, shadow, edge / blendWidth);
      }
    }

    return mix(shadow, 1.0, fade);
  }

  return 1.0;
}
