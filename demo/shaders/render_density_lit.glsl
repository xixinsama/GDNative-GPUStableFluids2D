#[compute]
#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Density/color texture
layout(rgba32f, set = 0, binding = 0) uniform restrict readonly image2D density_tex;

// Output lit texture
layout(rgba32f, set = 1, binding = 0) uniform restrict writeonly image2D output_tex;

layout(push_constant, std430) uniform Params {
    vec2 resolution;
    vec2 light_direction; // normalized 2D light direction
    float ambient;
    float specular_strength;
} params;

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(params.resolution.x) || pixel.y >= int(params.resolution.y)) return;

    vec4 density = imageLoad(density_tex, pixel);

    // Compute density gradient (Sobel operator) for pseudo-normal
    float d_center = density.r;
    float d_left   = (pixel.x > 0) ? imageLoad(density_tex, pixel + ivec2(-1, 0)).r : d_center;
    float d_right  = (pixel.x < int(params.resolution.x) - 1) ? imageLoad(density_tex, pixel + ivec2(1, 0)).r : d_center;
    float d_up     = (pixel.y < int(params.resolution.y) - 1) ? imageLoad(density_tex, pixel + ivec2(0, 1)).r : d_center;
    float d_down   = (pixel.y > 0) ? imageLoad(density_tex, pixel + ivec2(0, -1)).r : d_center;

    vec2 grad = vec2(d_right - d_left, d_up - d_down);
    float grad_len = length(grad);

    // Pseudo-normal from gradient (height-field approximation)
    float height_scale = 5.0;
    vec3 normal = vec3(-grad * height_scale, 1.0);
    normal = normalize(normal);

    // Lambertian diffuse
    float NdotL = clamp(dot(normal, vec3(params.light_direction, 0.2)), 0.0, 1.0);
    float diffuse = NdotL * (1.0 - params.ambient) + params.ambient;

    // Specular (Blinn-Phong)
    vec3 view_dir = vec3(0, 0, 1);
    vec3 half_vec = normalize(vec3(params.light_direction, 0.2) + view_dir);
    float spec = pow(clamp(dot(normal, half_vec), 0.0, 1.0), 32.0) * params.specular_strength * grad_len;

    vec3 lit_color = density.rgb * diffuse + vec3(spec);
    lit_color = clamp(lit_color, 0.0, 1.0);

    imageStore(output_tex, pixel, vec4(lit_color, density.a));
}
