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
    vec2 vel = imageLoad(velocity_in, pixel).xy;

    // If inside an obstacle, kill velocity (no-slip boundary)
    if (obs_current > 0.1) {
        imageStore(velocity_out, pixel, vec4(0.0, 0.0, 0.0, 1.0));
        return;
    }

    // Check neighboring pixels for obstacle motion (differential force)
    vec2 force = vec2(0.0);
    float wx = 1.0 / params.resolution.x;
    float wy = 1.0 / params.resolution.y;

    vec2 dirs[4] = { vec2(wx, 0), vec2(-wx, 0), vec2(0, wy), vec2(0, -wy) };

    for (int i = 0; i < 4; i++) {
        vec2 sample_uv = uv + dirs[i];
        if (sample_uv.x >= 0.0 && sample_uv.x <= 1.0 && sample_uv.y >= 0.0 && sample_uv.y <= 1.0) {
            float obs_neighbor = texture(obstacle_tex, sample_uv).a;
            if (obs_neighbor > 0.1) {
                // Obstacle neighbor — read its velocity (encoded in RG channels)
                vec2 obs_vel = texture(obstacle_tex, sample_uv).rg;
                force += obs_vel * params.obstacle_force_strength;
            }
        }
    }

    // Check if obstacle is approaching (compare current vs previous)
    float obs_pre = texture(obstacle_pre_tex, uv).a;
    if (obs_current < 0.1 && obs_pre > 0.1) {
        // Obstacle just left this cell — push fluid away
        vec2 obs_vel = texture(obstacle_pre_tex, uv).rg;
        force += obs_vel * params.obstacle_force_strength * 2.0;
    }

    vel += force * params.dt;
    imageStore(velocity_out, pixel, vec4(vel, 0.0, 1.0));
}
