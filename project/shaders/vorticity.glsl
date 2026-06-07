#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Set 0: velocity field (readonly input)
layout(rgba32f, set = 0, binding = 0) uniform restrict readonly image2D velocity_in;

// Set 1: velocity field (write — modified output)
layout(rgba32f, set = 1, binding = 0) uniform restrict writeonly image2D velocity_out;

layout(push_constant, std430) uniform Params {
    vec2 resolution;
    float dt;
    float vorticity_scale;
} params;

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(params.resolution.x) || pixel.y >= int(params.resolution.y)) return;

    vec2 v_center  = imageLoad(velocity_in, pixel).xy;
    vec2 v_right   = (pixel.x + 1 < int(params.resolution.x)) ? imageLoad(velocity_in, pixel + ivec2(1, 0)).xy : v_center;
    vec2 v_left    = (pixel.x - 1 >= 0) ? imageLoad(velocity_in, pixel + ivec2(-1, 0)).xy : v_center;
    vec2 v_up      = (pixel.y + 1 < int(params.resolution.y)) ? imageLoad(velocity_in, pixel + ivec2(0, 1)).xy : v_center;
    vec2 v_down    = (pixel.y - 1 >= 0) ? imageLoad(velocity_in, pixel + ivec2(0, -1)).xy : v_center;

    // Vorticity: ω = curl(v) = dv_y/dx - dv_x/dy
    float omega = 0.5 * ((v_right.y - v_left.y) - (v_up.x - v_down.x));

    // Gradient of |ω| — vortex location indicator
    float abs_right = abs(0.5 * ((imageLoad(velocity_in, pixel + ivec2(min(1, int(params.resolution.x)-1-pixel.x), 0))).y - v_left.y) - (v_up.x - v_down.x)));
    float abs_left  = abs(0.5 * ((v_right.y - imageLoad(velocity_in, pixel + ivec2(max(-1, -pixel.x), 0)).y) - (v_up.x - v_down.x)));
    float abs_up    = abs(0.5 * ((v_right.y - v_left.y) - (imageLoad(velocity_in, pixel + ivec2(0, min(1, int(params.resolution.y)-1-pixel.y))).x - v_down.x)));
    float abs_down  = abs(0.5 * ((v_right.y - v_left.y) - (v_up.x - imageLoad(velocity_in, pixel + ivec2(0, max(-1, -pixel.y))).x)));

    vec2 grad_abs_omega = vec2(abs_right - abs_left, abs_up - abs_down) * 0.5;
    float len_grad = length(grad_abs_omega);

    vec2 force = vec2(0);
    if (len_grad > 0.0001) {
        vec2 N = grad_abs_omega / len_grad;
        // Confinement force: ε * N × ω  (2D cross: N × scalar = (Ny*ω, -Nx*ω))
        force = vec2(N.y * omega, -N.x * omega) * params.vorticity_scale * params.dt;
    }

    vec2 new_vel = v_center + force;
    imageStore(velocity_out, pixel, vec4(new_vel, 0.0, 1.0));
}
