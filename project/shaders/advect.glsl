#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Set 0: the field being advected (sampled as texture)
layout(set = 0, binding = 0) uniform sampler2D input_field;

// Set 1: velocity field (image, readonly)
layout(rgba32f, set = 1, binding = 0) uniform restrict readonly image2D velocity_field;

// Set 2: output (image, writeonly)
layout(rgba32f, set = 2, binding = 0) uniform restrict writeonly image2D output_field;

// Push constants
layout(push_constant, std430) uniform Params {
    vec2 resolution;
    float dt;
    float rdx;
} params;

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(params.resolution.x) || pixel.y >= int(params.resolution.y)) return;

    // Backtrace: uv - velocity * dt / resolution
    vec2 uv = (vec2(pixel) + 0.5) / params.resolution;
    vec2 vel = imageLoad(velocity_field, pixel).xy;
    vec2 prev_uv = uv - vel * params.dt / params.resolution;
    prev_uv = clamp(prev_uv, 0.0, 1.0);

    vec4 result = texture(input_field, prev_uv);
    imageStore(output_field, pixel, result);
}
