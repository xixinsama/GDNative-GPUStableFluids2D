#[compute]
#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Set 0: the field being advected (sampled as texture)
layout(set = 0, binding = 0) uniform sampler2D input_field;

// Set 1: velocity field (sampled as texture -- avoids Vulkan layout conflict
// when self-advecting where input_field == velocity_field)
layout(set = 1, binding = 0) uniform sampler2D velocity_field;

// Set 2: output (image, writeonly)
layout(rgba32f, set = 2, binding = 0) uniform restrict writeonly image2D output_field;

// Push constants
layout(push_constant, std430) uniform Params {
    vec2 resolution;
    float dt;
    float rdx;
} params;

// Below this velocity magnitude, skip advection entirely.
// Prevents numerical noise in stationary fluid from causing
// progressive smearing at the grid scale.
const float VEL_EPSILON = 1e-5;

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(params.resolution.x) || pixel.y >= int(params.resolution.y)) return;

    // Backtrace: uv - velocity * dt / grid_size
    vec2 uv = (vec2(pixel) + 0.5) / params.resolution;
    vec2 vel = texelFetch(velocity_field, pixel, 0).xy;

    // Skip advection for near-zero velocity — avoids unnecessary
    // bilinear interpolation (low-pass filter) on stationary regions.
    if (abs(vel.x) < VEL_EPSILON && abs(vel.y) < VEL_EPSILON) {
        imageStore(output_field, pixel, texelFetch(input_field, pixel, 0));
        return;
    }

    vec2 prev_uv = uv - vel * params.dt / params.resolution;
    prev_uv = clamp(prev_uv, 0.0, 1.0);

    vec4 result = texture(input_field, prev_uv);
    imageStore(output_field, pixel, result);
}
