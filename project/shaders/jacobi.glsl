#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Set 0: current solution x (readonly image)
layout(rgba32f, set = 0, binding = 0) uniform restrict readonly image2D x_field;

// Set 1: right-hand side b (readonly image)
layout(rgba32f, set = 1, binding = 0) uniform restrict readonly image2D b_field;

// Set 2: new solution x' (writeonly image)
layout(rgba32f, set = 2, binding = 0) uniform restrict writeonly image2D x_new_field;

layout(push_constant, std430) uniform Params {
    vec2 resolution;
    float alpha;
    float rbeta;
} params;

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(params.resolution.x) || pixel.y >= int(params.resolution.y)) return;

    // Sample neighbors (clamp to boundaries)
    float x_center = imageLoad(x_field, pixel).x;
    float x_right  = (pixel.x + 1 < int(params.resolution.x)) ? imageLoad(x_field, pixel + ivec2(1, 0)).x : x_center;
    float x_left   = (pixel.x - 1 >= 0) ? imageLoad(x_field, pixel + ivec2(-1, 0)).x : x_center;
    float x_up     = (pixel.y + 1 < int(params.resolution.y)) ? imageLoad(x_field, pixel + ivec2(0, 1)).x : x_center;
    float x_down   = (pixel.y - 1 >= 0) ? imageLoad(x_field, pixel + ivec2(0, -1)).x : x_center;

    float b = imageLoad(b_field, pixel).x;

    // Jacobi: x_new = (sum_neighbors + alpha * b) * rbeta
    float x_new = (x_left + x_right + x_up + x_down + params.alpha * b) * params.rbeta;
    imageStore(x_new_field, pixel, vec4(x_new, 0.0, 0.0, 1.0));
}
