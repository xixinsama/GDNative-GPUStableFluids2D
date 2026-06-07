#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Set 0: pressure field (readonly)
layout(rgba32f, set = 0, binding = 0) uniform restrict readonly image2D pressure_field;

// Set 1: velocity field (readonly)
layout(rgba32f, set = 1, binding = 0) uniform restrict readonly image2D velocity_field;

// Set 2: new velocity (writeonly)
layout(rgba32f, set = 2, binding = 0) uniform restrict writeonly image2D velocity_new;

layout(push_constant, std430) uniform Params {
    vec2 resolution;
    float half_rdx;
} params;

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(params.resolution.x) || pixel.y >= int(params.resolution.y)) return;

    float p_center = imageLoad(pressure_field, pixel).x;
    float p_right  = (pixel.x + 1 < int(params.resolution.x)) ? imageLoad(pressure_field, pixel + ivec2(1, 0)).x : p_center;
    float p_left   = (pixel.x - 1 >= 0) ? imageLoad(pressure_field, pixel + ivec2(-1, 0)).x : p_center;
    float p_up     = (pixel.y + 1 < int(params.resolution.y)) ? imageLoad(pressure_field, pixel + ivec2(0, 1)).x : p_center;
    float p_down   = (pixel.y - 1 >= 0) ? imageLoad(pressure_field, pixel + ivec2(0, -1)).x : p_center;

    // grad(p) via central differences
    vec2 grad = vec2(p_right - p_left, p_up - p_down) * params.half_rdx;

    vec2 vel = imageLoad(velocity_field, pixel).xy;
    vel -= grad;
    imageStore(velocity_new, pixel, vec4(vel, 0.0, 1.0));
}
