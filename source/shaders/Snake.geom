#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 9) out;

layout(set = 1, binding = 0) uniform Transform {
    mat4 model;
    mat4 view;
    mat4 proj;
} transform;

layout(set = 0, binding = 4) uniform Snake {
    float wiggle;
} snake;

layout(location = 0) in vec3 fragPos[];
layout(location = 1) in vec2 fragTexCoord[];
layout(location = 2) in vec3 fragNormal[];

layout(location = 0) out vec3 gsFragPos;
layout(location = 1) out vec2 gsTexCoord;
layout(location = 2) out vec3 gsNormal;

void EmitVertexModified(int i, float expansionFactor)
{
    vec3 pos = fragPos[i] + fragNormal[i] * sin(fragPos[i].x * 3.0 + snake.wiggle * 5.0) * expansionFactor;

    gsFragPos = pos;
    gsTexCoord = fragTexCoord[i];
    gsNormal = fragNormal[i];

    gl_Position = transform.proj * transform.view * vec4(pos, 1.0);
    EmitVertex();
}

void main()
{
    for (int i = 0; i < 3; i++)
    {
        float tension = abs(sin(fragPos[i].x * 0.5) * snake.wiggle);

        //    float expansionFactor = 0.1 * snake.wiggle; // Controls how much the geometry expands
        float expansionFactor = 0.2 * tension; // Controls how much the geometry expands

        EmitVertexModified(i, expansionFactor);
    }
    EndPrimitive();
}
