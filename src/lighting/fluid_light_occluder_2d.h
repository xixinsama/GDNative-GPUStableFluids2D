#pragma once

#include "godot_cpp/classes/node2d.hpp"
#include "godot_cpp/variant/node_path.hpp"
#include "godot_cpp/variant/packed_vector2_array.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"

#include <vector>

namespace godot {

class GPUStableFluids2D;
class LightOccluder2D;

// Bridges fluid density texture → Godot's built-in 2D lighting system.
// Uses GPU Marching Square to extract density contours, then creates
// LightOccluder2D child nodes dynamically.
class FluidLightOccluder2D : public Node2D {
	GDCLASS(FluidLightOccluder2D, Node2D)

public:
	FluidLightOccluder2D();
	~FluidLightOccluder2D() override;

	void set_sim_target(const NodePath &p_path);
	NodePath get_sim_target() const;

	void set_density_threshold(float p_t);
	float get_density_threshold() const;

	void set_max_contours(int p_n);
	int get_max_contours() const;

	void set_update_frequency(int p_hz);
	int get_update_frequency() const;

	void set_simplify_tolerance(float p_t);
	float get_simplify_tolerance() const;

	void set_occluder_light_mask(int p_mask);
	int get_occluder_light_mask() const;

	void set_downscale_factor(int p_f);
	int get_downscale_factor() const;

	void _ready() override;
	void _process(double p_delta) override;

protected:
	static void _bind_methods();

private:
	NodePath _sim_target;
	float _density_threshold = 0.1f;
	int _max_contours = 8;
	int _update_frequency = 15;
	float _simplify_tolerance = 0.1f;
	int _occluder_light_mask = 1;
	int _downscale_factor = 2;

	float _update_timer = 0.0f;

	GPUStableFluids2D *_find_sim() const;
	void _update_occluders();
	void _clear_occluders();

	// Simple contour extraction (CPU-side for now; GPU Marching Square can be added later)
	PackedVector2Array _extract_contours();
	void _simplify_contours(PackedVector2Array &p_contour, float p_epsilon);
	void _create_occluder(const PackedVector2Array &p_points);
};

} // namespace godot
