#version 450

layout(push_constant) uniform PushConstants {
    mat4 viewProjection;
    vec2 viewportSize;
};

layout(location = 0) in vec3 inStart;
layout(location = 1) in vec3 inEnd;
layout(location = 2) in vec4 inColor;
layout(location = 3) in float inWidth;

layout(location = 0) out vec4 outColor;

// Two triangles per line: x selects the end (0 start, 1 end), y the side of the line
const vec2 CORNERS[6] = vec2[](
    vec2(0, -1), vec2(0, 1), vec2(1, -1),
    vec2(1, -1), vec2(0, 1), vec2(1, 1)
);

void main() {
    outColor = inColor;

    vec4 a = viewProjection * vec4(inStart, 1.0);
    vec4 b = viewProjection * vec4(inEnd, 1.0);

    // Clip against the near plane (z >= 0) before the divide by w, so a point behind the
    // camera cannot flip the line across the screen
    if (a.z < 0.0 && b.z < 0.0) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
        return;
    }
    if (a.z < 0.0) {
        a = mix(a, b, a.z / (a.z - b.z));
        a.z = 0.0;
    } else if (b.z < 0.0) {
        b = mix(b, a, b.z / (b.z - a.z));
        b.z = 0.0;
    }

    vec2 ndcA = a.xy / a.w;
    vec2 ndcB = b.xy / b.w;

    vec2 pixelDelta = (ndcB - ndcA) * 0.5 * viewportSize;
    float pixelLength = length(pixelDelta);
    vec2 direction = pixelLength > 1e-5 ? pixelDelta / pixelLength : vec2(1.0, 0.0);
    vec2 normal = vec2(-direction.y, direction.x);

    vec2 corner = CORNERS[gl_VertexIndex % 6];
    vec4 clip = corner.x < 0.5 ? a : b;
    vec2 ndc = corner.x < 0.5 ? ndcA : ndcB;

    // Square caps: extend each end by half the width so joined lines leave no notch
    float endSign = corner.x < 0.5 ? -1.0 : 1.0;
    vec2 offsetPixels = (direction * endSign + normal * corner.y) * (inWidth * 0.5);

    ndc += offsetPixels * 2.0 / viewportSize;

    gl_Position = vec4(ndc * clip.w, clip.z, clip.w);
}
