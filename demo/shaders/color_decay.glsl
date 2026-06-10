#[compute]
#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Set 0: color/density field (readonly)
layout(rgba32f, set = 0, binding = 0) uniform restrict readonly image2D color_in;

// Set 1: color/density field (writeonly)
layout(rgba32f, set = 1, binding = 0) uniform restrict writeonly image2D color_out;

layout(push_constant, std430) uniform Params {
    vec2 resolution;
    float longevity;   // multiplicative: color *= longevity (0.995 = slow fade)
    float decay;       // subtractive: color -= decay (0.0005 = slow subtract)
} params;

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(params.resolution.x) || pixel.y >= int(params.resolution.y)) return;

    vec4 color = imageLoad(color_in, pixel);

    // Apply multiplicative fade
    color.rgb *= params.longevity;

    // Apply subtractive decay (only to RGB, preserve alpha as density)
    color.rgb -= params.decay;

    // Clamp to valid range
    color.rgb = max(color.rgb, vec3(0.0));
    color.a   = clamp(color.a, 0.0, 1.0);

    imageStore(color_out, pixel, color);
}
