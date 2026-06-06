// fluidsim.glsl
// Stable Fluids 2D GPU implementation
// Compatible with GDExtension binding layout

#version 450
#extension GL_GOOGLE_include_directive : enable

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Uniform buffers
layout(set = 0, binding = 7) uniform ImpulseBuffer {
    vec4 impulse_data; // x,y: position, z,w: velocity
    float radius;
    float strength;
    int add_ink;
    float padding;
} impulse;

layout(set = 0, binding = 8) uniform SimParams {
    vec2 resolution;
    float dt;
    float viscosity;
    float diffusion;
    vec2 force_pos;
    vec2 force_vel;
    float force_radius;
    float force_strength;
    float density_amount;
    int pass;  // 0:advection, 1:add_force, 2:compute_divergence, 3:jacobi, 4:subtract_gradient
    float _pad[3];
} params;

// Images (read-write)
layout(set = 0, binding = 0, rgba32f) uniform image2D velocity_tex;
layout(set = 0, binding = 1, rgba32f) uniform image2D density_tex;
layout(set = 0, binding = 2, rgba32f) uniform image2D pressure_tex;
layout(set = 0, binding = 3, rgba32f) uniform image2D divergence_tex;
layout(set = 0, binding = 4, rgba32f) uniform image2D vorticity_tex;
layout(set = 0, binding = 5, rgba32f) uniform image2D velocity_offsets_tex;
layout(set = 0, binding = 6, rgba32f) uniform image2D pressure_offsets_tex;

// Helper functions
vec4 sample_bilinear(image2D img, vec2 uv) {
    ivec2 size = imageSize(img);
    ivec2 pixel = ivec2(uv * vec2(size));
    pixel = clamp(pixel, ivec2(0), size - ivec2(1));
    return imageLoad(img, pixel);
}

vec2 texcoord_from_pixel(ivec2 pixel) {
    return (vec2(pixel) + 0.5) / params.resolution;
}

vec2 pixel_from_texcoord(vec2 uv) {
    return uv * params.resolution - 0.5;
}

// Advection (semi-Lagrangian)
void advection() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(params.resolution.x) || pixel.y >= int(params.resolution.y)) return;
    
    vec2 uv = texcoord_from_pixel(pixel);
    
    // Read velocity at current position
    vec2 vel = imageLoad(velocity_tex, pixel).xy;
    
    // Trace back in time
    vec2 prev_uv = uv - vel * params.dt / params.resolution;
    
    // Clamp to bounds
    prev_uv = clamp(prev_uv, 0.0, 1.0);
    
    // Sample advected quantities
    ivec2 prev_pixel = ivec2(prev_uv * params.resolution);
    prev_pixel = clamp(prev_pixel, ivec2(0), ivec2(params.resolution) - ivec2(1));
    
    vec2 advected_vel = imageLoad(velocity_tex, prev_pixel).xy;
    float advected_density = imageLoad(density_tex, prev_pixel).x;
    
    // Write results
    imageStore(velocity_tex, pixel, vec4(advected_vel, 0.0, 1.0));
    imageStore(density_tex, pixel, vec4(advected_density, 0.0, 0.0, 1.0));
}

// Add force (impulse)
void add_force() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(params.resolution.x) || pixel.y >= int(params.resolution.y)) return;
    
    vec2 uv = texcoord_from_pixel(pixel);
    vec2 pos = uv * params.resolution;
    vec2 force_pos_pixel = impulse.impulse_data.xy * params.resolution;
    vec2 force_vel = impulse.impulse_data.zw;
    float radius = impulse.radius;
    
    // Calculate distance to force center
    float dist = distance(pos, force_pos_pixel);
    
    // Apply force if within radius
    vec2 vel = imageLoad(velocity_tex, pixel).xy;
    float density = imageLoad(density_tex, pixel).x;
    
    if (dist < radius && impulse.strength > 0.0) {
        float strength = impulse.strength * (1.0 - dist / radius);
        vel += force_vel * strength * params.dt;
        if (impulse.add_ink != 0) {
            density += params.density_amount * strength * params.dt;
        }
    }
    
    // Write results
    imageStore(velocity_tex, pixel, vec4(vel, 0.0, 1.0));
    imageStore(density_tex, pixel, vec4(density, 0.0, 0.0, 1.0));
}

// Compute divergence of velocity field
void compute_divergence() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(params.resolution.x) || pixel.y >= int(params.resolution.y)) return;
    
    float dx = 1.0;
    float dy = 1.0;
    
    // Sample neighboring velocities
    vec2 vel_center = imageLoad(velocity_tex, pixel).xy;
    vec2 vel_right = imageLoad(velocity_tex, pixel + ivec2(1, 0)).xy;
    vec2 vel_left = imageLoad(velocity_tex, pixel + ivec2(-1, 0)).xy;
    vec2 vel_up = imageLoad(velocity_tex, pixel + ivec2(0, 1)).xy;
    vec2 vel_down = imageLoad(velocity_tex, pixel + ivec2(0, -1)).xy;
    
    // Handle boundaries (use center if out of bounds)
    if (pixel.x == int(params.resolution.x) - 1) vel_right = vel_center;
    if (pixel.x == 0) vel_left = vel_center;
    if (pixel.y == int(params.resolution.y) - 1) vel_up = vel_center;
    if (pixel.y == 0) vel_down = vel_center;
    
    // Compute divergence: ∇·v = ∂v_x/∂x + ∂v_y/∂y
    float divergence = (vel_right.x - vel_left.x) / (2.0 * dx) +
                       (vel_up.y - vel_down.y) / (2.0 * dy);
    
    imageStore(divergence_tex, pixel, vec4(divergence, 0.0, 0.0, 1.0));
}

// Jacobi iteration for pressure solve
void jacobi() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(params.resolution.x) || pixel.y >= int(params.resolution.y)) return;
    
    float alpha = -1.0; // -dx * dy where dx=dy=1
    float beta = 4.0;
    
    // Sample neighboring pressures
    float p_center = imageLoad(pressure_tex, pixel).x;
    float p_right = imageLoad(pressure_tex, pixel + ivec2(1, 0)).x;
    float p_left = imageLoad(pressure_tex, pixel + ivec2(-1, 0)).x;
    float p_up = imageLoad(pressure_tex, pixel + ivec2(0, 1)).x;
    float p_down = imageLoad(pressure_tex, pixel + ivec2(0, -1)).x;
    
    // Handle boundaries
    if (pixel.x == int(params.resolution.x) - 1) p_right = p_center;
    if (pixel.x == 0) p_left = p_center;
    if (pixel.y == int(params.resolution.y) - 1) p_up = p_center;
    if (pixel.y == 0) p_down = p_center;
    
    // Sample divergence at current cell
    float divergence = imageLoad(divergence_tex, pixel).x;
    
    // Jacobi iteration: p_new = (p_left + p_right + p_up + p_down + alpha * divergence) / beta
    float p_new = (p_left + p_right + p_up + p_down + alpha * divergence) / beta;
    
    imageStore(pressure_tex, pixel, vec4(p_new, 0.0, 0.0, 1.0));
}

// Subtract pressure gradient from velocity
void subtract_gradient() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(params.resolution.x) || pixel.y >= int(params.resolution.y)) return;
    
    // Sample pressure at neighboring cells
    float p_center = imageLoad(pressure_tex, pixel).x;
    float p_right = imageLoad(pressure_tex, pixel + ivec2(1, 0)).x;
    float p_left = imageLoad(pressure_tex, pixel + ivec2(-1, 0)).x;
    float p_up = imageLoad(pressure_tex, pixel + ivec2(0, 1)).x;
    float p_down = imageLoad(pressure_tex, pixel + ivec2(0, -1)).x;
    
    // Handle boundaries
    if (pixel.x == int(params.resolution.x) - 1) p_right = p_center;
    if (pixel.x == 0) p_left = p_center;
    if (pixel.y == int(params.resolution.y) - 1) p_up = p_center;
    if (pixel.y == 0) p_down = p_center;
    
    // Compute pressure gradient: ∇p = (∂p/∂x, ∂p/∂y)
    vec2 gradient = vec2(
        (p_right - p_left) / 2.0,
        (p_up - p_down) / 2.0
    );
    
    // Subtract gradient from velocity: v_new = v_old - ∇p
    vec2 vel = imageLoad(velocity_tex, pixel).xy;
    vel -= gradient;
    
    imageStore(velocity_tex, pixel, vec4(vel, 0.0, 1.0));
}

void main() {
    switch (params.pass) {
        case 0: advection(); break;
        case 1: add_force(); break;
        case 2: compute_divergence(); break;
        case 3: jacobi(); break;
        case 4: subtract_gradient(); break;
        default: break;
    }
}