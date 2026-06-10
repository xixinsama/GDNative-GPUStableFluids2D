#[compute]
#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Current velocity field (read-write)
layout(rgba32f, set = 0, binding = 0) uniform restrict readonly image2D velocity_in;
layout(rgba32f, set = 1, binding = 0) uniform restrict writeonly image2D velocity_out;

// Current and previous obstacle textures
layout(set = 2, binding = 0) uniform sampler2D obstacle_tex;
layout(set = 3, binding = 0) uniform sampler2D obstacle_pre_tex;

layout(push_constant, std430) uniform Params {
    vec2 resolution;
    float dt;
    float obstacle_force_strength;
} params;

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(params.resolution.x) || pixel.y >= int(params.resolution.y)) return;

    vec2 uv = (vec2(pixel) + 0.5) / params.resolution;

    // Sample obstacle alpha at current pixel
    float obs_current = texture(obstacle_tex, uv).a;

    // --- Hard barrier: zero out fluid velocity inside obstacles ---
    if (obs_current > 0.1) {
        imageStore(velocity_out, pixel, vec4(0.0, 0.0, 0.0, 1.0));
        return;
    }

    vec2 vel = imageLoad(velocity_in, pixel).xy;

    // --- Check if any neighbor is an obstacle (boundary cell) ---
    // For boundary cells adjacent to obstacles, adopt the obstacle's velocity
    // (no-slip condition: fluid at obstacle surface moves with the obstacle).
    // This allows moving obstacles to push fluid.
    float wx = 1.0 / params.resolution.x;
    float wy = 1.0 / params.resolution.y;

    vec2 dirs[4] = { vec2(wx, 0), vec2(-wx, 0), vec2(0, wy), vec2(0, -wy) };
    bool adjacent_to_obstacle = false;
    vec2 obstacle_surface_vel = vec2(0.0);

    for (int i = 0; i < 4; i++) {
        vec2 sample_uv = uv + dirs[i];
        if (sample_uv.x >= 0.0 && sample_uv.x <= 1.0 && sample_uv.y >= 0.0 && sample_uv.y <= 1.0) {
            float obs_neighbor = texture(obstacle_tex, sample_uv).a;
            if (obs_neighbor > 0.1) {
                adjacent_to_obstacle = true;
                // Read the obstacle's own velocity (encoded in RG of obstacle texture)
                obstacle_surface_vel += texture(obstacle_tex, sample_uv).rg;
            }
        }
    }

    if (adjacent_to_obstacle) {
        // Average the obstacle velocities from all adjacent obstacle cells
        obstacle_surface_vel /= max(1.0, float(4));
        // Blend fluid velocity toward obstacle's surface velocity (no-slip boundary)
        // obstacle_force_strength acts as blend factor (higher = tighter coupling)
        float blend = clamp(params.obstacle_force_strength * params.dt, 0.0, 1.0);
        vel = mix(vel, obstacle_surface_vel, blend);
    }

    // --- Handle obstacle departure ---
    // If an obstacle has just left this cell (was present in previous frame, not now),
    // the departing obstacle's velocity pushes the fluid outward.
    float obs_pre = texture(obstacle_pre_tex, uv).a;
    if (obs_current < 0.1 && obs_pre > 0.1) {
        vec2 obs_vel = texture(obstacle_pre_tex, uv).rg;
        vel += obs_vel * params.obstacle_force_strength * params.dt;
    }

    imageStore(velocity_out, pixel, vec4(vel, 0.0, 1.0));
}
