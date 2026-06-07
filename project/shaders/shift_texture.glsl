#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Set 0: texture to shift (sampled)
layout(set = 0, binding = 0) uniform sampler2D input_tex;

// Set 1: output (writeonly image)
layout(rgba32f, set = 1, binding = 0) uniform restrict writeonly image2D output_tex;

layout(push_constant, std430) uniform Params {
    vec2 resolution;
    vec2 offset; // UV-space, negative for scrolling opposite camera direction
} params;

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(params.resolution.x) || pixel.y >= int(params.resolution.y)) return;

    vec2 uv = (vec2(pixel) + 0.5) / params.resolution;
    // Scroll: sample from uv - offset (toroidal wrap via fract)
    vec2 sample_uv = fract(uv - params.offset);
    vec4 result = texture(input_tex, sample_uv);
    imageStore(output_tex, pixel, result);
}
