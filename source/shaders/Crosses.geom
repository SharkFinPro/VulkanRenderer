#version 450

layout(triangles) in;
layout(line_strip, max_vertices=78) out;

layout(set = 1, binding = 0) uniform Transform {
  mat4 model;
  mat4 view;
  mat4 proj;
} transform;

layout(set = 0, binding = 4) uniform Crosses {
  int level;
  float quantize;
  float size;
  float shininess;
} crosses;

layout(location = 0) in vec3 gsPos[];
layout(location = 1) in vec3 gsNormal[];

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;

void ProduceCrosses(float s, float t);

vec3 Quantize(vec3 v);

vec3 V0, V1, V2;
vec3 V01, V02;
vec3 N0, N1, N2;
vec3 N01, N02;
vec3 LIGHTPOSITION = vec3(1);

void main()
{
  V0 = gsPos[0].xyz;
  V1 = gsPos[1].xyz;
  V2 = gsPos[2].xyz;
  V01 = V1 - V0;
  V02 = V2 - V0;

  N0 = gsNormal[0].xyz;
  N1 = gsNormal[1].xyz;
  N2 = gsNormal[2].xyz;
  N01 = N1 - N0;
  N02 = N2 - N0;

  int numLayers = 1 << crosses.level;

  float dt = 1. / float( numLayers );
  float t = 1.;

  for(int i = 0; i <= numLayers; i++)
  {
    float smax = 1. - t;

    int nums = i + 1;
    float ds = smax / float( nums - 1 );

    float s = 0.;

    for( int is = 0; is < nums; is++ )
    {
      ProduceCrosses( s, t );
      s += ds;
    }

    t -= dt;
  }
}

void ProduceCrosses(float s, float t)
{
  // Interpolate vertex position using barycentric coordinates
  vec3 v = (1.0 - s - t) * V0 + s * V1 + t * V2;
  v = Quantize(v);

  // Interpolate normal vectors using the same barycentric interpolation
  vec3 n = normalize((1.0 - s - t) * N0 + s * N1 + t * N2);
  vec3 nv = normalize(mat3(transpose(inverse(transform.model))) * n);

  // Cross size
  vec3 sizeVec = vec3(crosses.size);

  // X-line cross
  vec3 leftX = v - vec3(sizeVec.x, 0.0, 0.0);
  vec3 rightX = v + vec3(sizeVec.x, 0.0, 0.0);
  gl_Position = transform.proj * transform.view * transform.model * vec4(leftX, 1.0);
  fragPos = leftX;
  fragNormal = nv;
  EmitVertex();

  gl_Position = transform.proj * transform.view * transform.model * vec4(rightX, 1.0);
  fragPos = rightX;
  fragNormal = nv;
  EmitVertex();
  EndPrimitive();

  // Y-line cross
  vec3 downY = v - vec3(0.0, sizeVec.y, 0.0);
  vec3 upY = v + vec3(0.0, sizeVec.y, 0.0);
  gl_Position = transform.proj * transform.view * transform.model * vec4(downY, 1.0);
  fragPos = downY;
  fragNormal = nv;
  EmitVertex();

  gl_Position = transform.proj * transform.view * transform.model * vec4(upY, 1.0);
  fragPos = upY;
  fragNormal = nv;
  EmitVertex();
  EndPrimitive();

  // Z-line cross
  vec3 backZ = v - vec3(0.0, 0.0, sizeVec.z);
  vec3 forwardZ = v + vec3(0.0, 0.0, sizeVec.z);
  gl_Position = transform.proj * transform.view * transform.model * vec4(backZ, 1.0);
  fragPos = backZ;
  fragNormal = nv;
  EmitVertex();

  gl_Position = transform.proj * transform.view * transform.model * vec4(forwardZ, 1.0);
  fragPos = forwardZ;
  fragNormal = nv;
  EmitVertex();
  EndPrimitive();
}

vec3 Quantize(vec3 v)
{
  v.x *= crosses.quantize;
  int xi = int(v.x);
  v.x = float(xi) / crosses.quantize;

  v.y *= crosses.quantize;
  int yi = int(v.y);
  v.y = float(yi) / crosses.quantize;

  v.z *= crosses.quantize;
  int zi = int(v.z);
  v.z = float(zi) / crosses.quantize;

  return v;
}