#[compute]
#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba32f, set = 0, binding = 0) uniform restrict readonly image2D velocity_in;
layout(rgba32f, set = 1, binding = 0) uniform restrict writeonly image2D velocity_out;

layout(push_constant, std430) uniform Params {
	vec2 resolution;
	float dt;
	float vorticity_scale;
} params;

void main() {
	ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
	if (pixel.x >= int(params.resolution.x) || pixel.y >= int(params.resolution.y)) return;

	int w = int(params.resolution.x);
	int h = int(params.resolution.y);

	// Read velocity at center + 4 neighbors (clamped)
	ivec2 pr = ivec2(min(pixel.x + 1, w - 1), pixel.y);
	ivec2 pl = ivec2(max(pixel.x - 1, 0), pixel.y);
	ivec2 pu = ivec2(pixel.x, min(pixel.y + 1, h - 1));
	ivec2 pd = ivec2(pixel.x, max(pixel.y - 1, 0));

	vec2 vc = imageLoad(velocity_in, pixel).xy;
	vec2 vr = imageLoad(velocity_in, pr).xy;
	vec2 vl = imageLoad(velocity_in, pl).xy;
	vec2 vu = imageLoad(velocity_in, pu).xy;
	vec2 vd = imageLoad(velocity_in, pd).xy;

	// Vorticity at center
	float omega = 0.5 * ((vr.y - vl.y) - (vu.x - vd.x));

	// Vorticity at far neighbors for gradient of |omega|
	ivec2 prr = ivec2(min(pixel.x + 2, w - 1), pixel.y);
	ivec2 pll = ivec2(max(pixel.x - 2, 0), pixel.y);
	ivec2 puu = ivec2(pixel.x, min(pixel.y + 2, h - 1));
	ivec2 pdd = ivec2(pixel.x, max(pixel.y - 2, 0));

	vec2 vrr = imageLoad(velocity_in, prr).xy;
	vec2 vll = imageLoad(velocity_in, pll).xy;
	vec2 vuu = imageLoad(velocity_in, puu).xy;
	vec2 vdd = imageLoad(velocity_in, pdd).xy;

	float wr = 0.5 * ((vrr.y - vc.y) - (vu.x - vd.x));
	float wl = 0.5 * ((vc.y - vll.y) - (vu.x - vd.x));
	float wu = 0.5 * ((vr.y - vl.y) - (vuu.x - vc.x));
	float wd = 0.5 * ((vr.y - vl.y) - (vc.x - vdd.x));

	vec2 grad_abs = 0.5 * vec2(abs(wr) - abs(wl), abs(wu) - abs(wd));
	float len_g = length(grad_abs);

	vec2 force = vec2(0.0);
	if (len_g > 0.0001) {
		vec2 N = grad_abs / len_g;
		force = params.vorticity_scale * params.dt * vec2(N.y * omega, -N.x * omega);
	}

	imageStore(velocity_out, pixel, vec4(vc + force, 0.0, 1.0));
}
