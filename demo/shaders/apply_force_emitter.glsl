#[compute]
#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Force emitter data (must match C++ ForceEmitterData struct, 48 bytes)
struct ForceEmitterData {
    vec2 center;
    vec2 force;
    vec2 shape_size;
    float force_radius;
    float falloff_exponent;
    float swirl_strength;
    int force_pattern;
    int emission_shape;
    float _pad;
};

// Set 0: storage buffer with force emitter data
layout(set = 0, binding = 0, std430) readonly buffer ForceEmitterBuffer {
    ForceEmitterData emitters[];
} force_emitters;

// Set 1: velocity field (read-write)
layout(rgba32f, set = 1, binding = 0) uniform restrict image2D velocity_field;

// Set 2: color field (read-write, optional)
layout(rgba32f, set = 2, binding = 0) uniform restrict image2D color_field;

layout(push_constant, std430) uniform Params {
    vec2 resolution;
    int emitter_count;
    float dt;
} params;

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(params.resolution.x) || pixel.y >= int(params.resolution.y)) return;
    if (params.emitter_count <= 0) return;

    vec2 uv = (vec2(pixel) + 0.5) / params.resolution;
    vec2 vel = imageLoad(velocity_field, pixel).xy;
    vec4 color = imageLoad(color_field, pixel);

    for (int e = 0; e < params.emitter_count; e++) {
        ForceEmitterData em = force_emitters.emitters[e];

        // Compute distance from pixel to emitter center
        vec2 delta = uv - em.center;
        float dist = length(delta / max(em.shape_size, vec2(0.001)));

        // Skip if outside force radius
        float radius_uv = em.force_radius / max(params.resolution.x, params.resolution.y);
        if (dist > 1.0) continue;

        // Falloff
        float strength = 1.0 - pow(dist, em.falloff_exponent);
        strength = clamp(strength, 0.0, 1.0);

        // Apply force pattern
        vec2 force_contrib = vec2(0.0);
        switch (em.force_pattern) {
            case 0: // Directional
                force_contrib = em.force * strength;
                break;
            case 1: // PointRadial
                force_contrib = normalize(delta) * length(em.force) * strength;
                break;
            case 2: // Swirl
                force_contrib = vec2(-delta.y, delta.x) * em.swirl_strength * strength;
                break;
            case 3: // Centripetal
                force_contrib = -normalize(delta) * length(em.force) * strength;
                break;
            case 4: // Centrifugal
                force_contrib = normalize(delta) * length(em.force) * strength;
                break;
        }

        vel += force_contrib * params.dt;
    }

    imageStore(velocity_field, pixel, vec4(vel, 0.0, 1.0));
    imageStore(color_field, pixel, color);
}
