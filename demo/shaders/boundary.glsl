#[compute]
#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Set 0: field to apply boundary to (readonly)
layout(rgba32f, set = 0, binding = 0) uniform restrict readonly image2D input_field;

// Set 1: output (writeonly)
layout(rgba32f, set = 1, binding = 0) uniform restrict writeonly image2D output_field;

layout(push_constant, std430) uniform Params {
    vec2 resolution;
    float scale;
} params;

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(params.resolution.x) || pixel.y >= int(params.resolution.y)) return;

    bool on_left   = (pixel.x == 0);
    bool on_right  = (pixel.x == int(params.resolution.x) - 1);
    bool on_bottom = (pixel.y == 0);
    bool on_top    = (pixel.y == int(params.resolution.y) - 1);

    if (!on_left && !on_right && !on_bottom && !on_top) {
        // Not a boundary cell -- passthrough
        imageStore(output_field, pixel, imageLoad(input_field, pixel));
        return;
    }

    vec4 result = vec4(0, 0, 0, 1);

    if (on_left) {
        // Reflect: set x-component from right neighbor, scaled
        vec4 neighbor = imageLoad(input_field, pixel + ivec2(1, 0));
        result.xy = vec2(params.scale * neighbor.x, neighbor.y);
    } else if (on_right) {
        vec4 neighbor = imageLoad(input_field, pixel + ivec2(-1, 0));
        result.xy = vec2(params.scale * neighbor.x, neighbor.y);
    } else if (on_bottom) {
        vec4 neighbor = imageLoad(input_field, pixel + ivec2(0, 1));
        result.xy = vec2(neighbor.x, params.scale * neighbor.y);
    } else if (on_top) {
        vec4 neighbor = imageLoad(input_field, pixel + ivec2(0, -1));
        result.xy = vec2(neighbor.x, params.scale * neighbor.y);
    } else {
        result.xy = imageLoad(input_field, pixel).xy;
    }

    // For pressure (scalar): flip sign based on scale
    // scale = -1.0 -> no-slip for velocity
    // scale =  1.0 -> Neumann for pressure
    result.zw = vec2(0, 1);

    imageStore(output_field, pixel, result);
}
