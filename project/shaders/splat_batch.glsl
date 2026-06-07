#version 450

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

// Storage buffer with batch point data
struct BatchPoint {
    vec2 pos;        // 8 bytes
    vec2 vel;        // 8 bytes
    vec4 color;      // 16 bytes
    float color_r;   // 4 bytes
    float vel_r;     // 4 bytes
};

layout(set = 0, binding = 0, std430) restrict readonly buffer BatchBuffer {
    BatchPoint points[];
} batch;

// Target textures (read-write)
layout(rgba32f, set = 1, binding = 0) uniform image2D velocity_tex;
layout(rgba32f, set = 2, binding = 0) uniform image2D color_tex;

layout(push_constant, std430) uniform Params {
    vec2 resolution;
    int point_count;
    float dt;
} params;

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= params.point_count) return;

    BatchPoint bp = batch.points[idx];

    // Convert world/UV position to pixel coordinate
    ivec2 pixel = ivec2(bp.pos * params.resolution);
    pixel = clamp(pixel, ivec2(0), ivec2(params.resolution) - ivec2(1));

    float brush_size = 5.0; // pixel radius base

    // Simple circular brush: check pixel neighborhood
    int radius_i = int(max(1.0, bp.color_r * brush_size));
    int vel_radius_i = int(max(1.0, bp.vel_r * brush_size));

    // Apply color to neighborhood
    for (int dy = -radius_i; dy <= radius_i; dy++) {
        for (int dx = -radius_i; dx <= radius_i; dx++) {
            ivec2 tp = pixel + ivec2(dx, dy);
            if (tp.x < 0 || tp.y < 0 || tp.x >= int(params.resolution.x) || tp.y >= int(params.resolution.y))
                continue;

            float dist = length(vec2(dx, dy));
            float alpha = 1.0 - smoothstep(0.0, float(radius_i), dist);
            if (alpha > 0.01) {
                vec4 old = imageLoad(color_tex, tp);
                vec4 new_col = mix(old, bp.color, alpha * bp.color.a);
                imageStore(color_tex, tp, new_col);
            }
        }
    }

    // Apply velocity to neighborhood
    for (int dy = -vel_radius_i; dy <= vel_radius_i; dy++) {
        for (int dx = -vel_radius_i; dx <= vel_radius_i; dx++) {
            ivec2 tp = pixel + ivec2(dx, dy);
            if (tp.x < 0 || tp.y < 0 || tp.x >= int(params.resolution.x) || tp.y >= int(params.resolution.y))
                continue;

            float dist = length(vec2(dx, dy));
            float alpha = 1.0 - smoothstep(0.0, float(vel_radius_i), dist);
            if (alpha > 0.01) {
                vec4 old_vel = imageLoad(velocity_tex, tp);
                old_vel.xy += bp.vel * alpha * params.dt;
                imageStore(velocity_tex, tp, old_vel);
            }
        }
    }
}
