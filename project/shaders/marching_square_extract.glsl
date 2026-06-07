#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Density texture to extract contours from
layout(rgba32f, set = 0, binding = 0) uniform restrict readonly image2D density_tex;

// Output buffer: each cell = 4-bit marching square code (0-15)
// 0 = empty, 1-14 = contour cell, 15 = interior
layout(set = 1, binding = 0, std430) restrict writeonly buffer OutputBuffer {
    int codes[];
} output_data;

layout(push_constant, std430) uniform Params {
    vec2 resolution;
    float threshold;
    int scale; // downscale factor for coarser contour extraction
} params;

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(params.resolution.x) || pixel.y >= int(params.resolution.y)) return;

    // Marching square: sample 4 corners at (pixel + offset)
    ivec2 p00 = pixel * params.scale;
    ivec2 p10 = p00 + ivec2(params.scale, 0);
    ivec2 p11 = p00 + ivec2(params.scale, params.scale);
    ivec2 p01 = p00 + ivec2(0, params.scale);

    int max_x = int(params.resolution.x) * params.scale - 1;
    int max_y = int(params.resolution.y) * params.scale - 1;

    // Clamp to bounds
    p00 = clamp(p00, ivec2(0), ivec2(max_x, max_y));
    p10 = clamp(p10, ivec2(0), ivec2(max_x, max_y));
    p11 = clamp(p11, ivec2(0), ivec2(max_x, max_y));
    p01 = clamp(p01, ivec2(0), ivec2(max_x, max_y));

    float v00 = imageLoad(density_tex, p00).r;
    float v10 = imageLoad(density_tex, p10).r;
    float v11 = imageLoad(density_tex, p11).r;
    float v01 = imageLoad(density_tex, p01).r;

    int code = 0;
    if (v00 > params.threshold) code |= 1;  // bottom-left
    if (v10 > params.threshold) code |= 2;  // bottom-right
    if (v11 > params.threshold) code |= 4;  // top-right
    if (v01 > params.threshold) code |= 8;  // top-left

    int idx = pixel.y * int(params.resolution.x) + pixel.x;
    output_data.codes[idx] = code;
}
