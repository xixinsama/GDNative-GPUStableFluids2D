#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba32f, set = 0, binding = 0) uniform restrict readonly image2D src_tex;
layout(rgba32f, set = 1, binding = 0) uniform restrict writeonly image2D dst_tex;

layout(push_constant, std430) uniform Params {
    vec2 resolution;
} params;

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(params.resolution.x) || pixel.y >= int(params.resolution.y)) return;

    vec4 val = imageLoad(src_tex, pixel);
    imageStore(dst_tex, pixel, val);
}
