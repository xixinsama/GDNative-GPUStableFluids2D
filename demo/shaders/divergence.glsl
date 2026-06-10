#[compute]
#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Set 0: velocity field (readonly)
layout(rgba32f, set = 0, binding = 0) uniform restrict readonly image2D velocity_field;

// Set 1: divergence field (writeonly)
layout(rgba32f, set = 1, binding = 0) uniform restrict writeonly image2D divergence_field;

layout(push_constant, std430) uniform Params {
    vec2 resolution;
    float half_rdx;
} params;

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(params.resolution.x) || pixel.y >= int(params.resolution.y)) return;

    vec2 v_center  = imageLoad(velocity_field, pixel).xy;
    vec2 v_right   = (pixel.x + 1 < int(params.resolution.x)) ? imageLoad(velocity_field, pixel + ivec2(1, 0)).xy : v_center;
    vec2 v_left    = (pixel.x - 1 >= 0) ? imageLoad(velocity_field, pixel + ivec2(-1, 0)).xy : v_center;
    vec2 v_up      = (pixel.y + 1 < int(params.resolution.y)) ? imageLoad(velocity_field, pixel + ivec2(0, 1)).xy : v_center;
    vec2 v_down    = (pixel.y - 1 >= 0) ? imageLoad(velocity_field, pixel + ivec2(0, -1)).xy : v_center;

    // Central differences: div = (dv_x/dx + dv_y/dy)
    float div = (v_right.x - v_left.x + v_up.y - v_down.y) * params.half_rdx;
    imageStore(divergence_field, pixel, vec4(div, 0.0, 0.0, 1.0));
}
