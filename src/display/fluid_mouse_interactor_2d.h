#pragma once

#include "godot_cpp/classes/node2d.hpp"
#include "godot_cpp/variant/node_path.hpp"
#include "godot_cpp/variant/color.hpp"
#include "godot_cpp/variant/vector2.hpp"

#include "core/fluid_types.h"

namespace godot {

// Converts mouse/touch input into fluid draw requests.
// Place as a child of the GPUStableFluids2D node (auto-detects parent).
class FluidMouseInteractor2D : public Node2D {
	GDCLASS(FluidMouseInteractor2D, Node2D)

public:
	FluidMouseInteractor2D() = default;
	~FluidMouseInteractor2D() override = default;

	void set_sim_target(const NodePath &p_path);
	NodePath get_sim_target() const;

	void set_mouse_button(int p_button);
	int get_mouse_button() const;

	void set_draw_color(const Color &p_color);
	Color get_draw_color() const;

	void set_brush_size(float p_size);
	float get_brush_size() const;

	void set_velocity_scale(float p_scale);
	float get_velocity_scale() const;

	void set_continuous_draw(bool p_enable);
	bool is_continuous_draw() const;

	void set_interaction_mode(int p_mode);
	int get_interaction_mode() const;

	void _ready() override;
	void _process(double p_delta) override;

protected:
	static void _bind_methods();

private:
	NodePath _sim_target;
	int _mouse_button     = 1; // MOUSE_BUTTON_LEFT
	Color _draw_color     = Color(1, 1, 1, 1);
	float _brush_size     = 5.0f;
	float _velocity_scale = 0.1f;
	bool _continuous_draw = true;
	InteractionMode _interaction_mode = InteractionMode::Draw;

	Vector2 _prev_mouse_pos;
	bool _was_pressed = false;

	class GPUStableFluids2D *_find_sim() const;
};

} // namespace godot
