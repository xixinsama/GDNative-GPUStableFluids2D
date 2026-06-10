#pragma once

#include "godot_cpp/classes/node2d.hpp"
#include "godot_cpp/variant/color.hpp"
#include "godot_cpp/variant/vector2.hpp"
#include "godot_cpp/variant/node_path.hpp"
#include "godot_cpp/classes/object.hpp"

#include "core/fluid_types.h"

namespace godot {

class GPUStableFluids2D;

// Converts mouse / touch input into fluid draw requests.
//
// Resolution order: if sim_target is set, use that node.  Otherwise walk up
// the parent chain looking for a GPUStableFluids2D.  This allows the node to
// work both as a direct child (auto-detect) and as a sibling / remote node
// (explicit sim_target).
//
// Coordinates: uses CanvasItem::get_global_mouse_position() which returns
// the mouse in world space, correctly accounting for Camera2D transforms.
class FluidMouseInteractor2D : public Node2D {
	GDCLASS(FluidMouseInteractor2D, Node2D)

public:
	FluidMouseInteractor2D() = default;
	~FluidMouseInteractor2D() override = default;

	void set_sim_target(Object *p_obj);
	Object *get_sim_target() const;

	void set_mouse_button(int p_button);
	int get_mouse_button() const;

	void set_draw_color(const Color &p_color);
	Color get_draw_color() const;

	void set_brush_size(float p_size);
	float get_brush_size() const;

	void set_velocity_scale(float p_scale);
	float get_velocity_scale() const;

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
	InteractionMode _interaction_mode = InteractionMode::Draw;

	GPUStableFluids2D *_sim = nullptr;
	Vector2 _prev_mouse_pos;
	bool _was_pressed = false;
	bool _has_prev_pos = false;

	void _resolve_sim();
};

} // namespace godot
