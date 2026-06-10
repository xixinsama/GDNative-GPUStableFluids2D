#pragma once

#include "godot_cpp/classes/sprite2d.hpp"
#include "godot_cpp/classes/texture2drd.hpp"
#include "godot_cpp/variant/node_path.hpp"
#include "godot_cpp/variant/color.hpp"

#include "core/fluid_types.h"

namespace godot {

// Sprite2D sub-class that auto-binds to a GPUStableFluids2D output texture.
// Drop it into the scene as child of GPUStableFluids2D, or point sim_target at one.
class FluidDisplay2D : public Sprite2D {
	GDCLASS(FluidDisplay2D, Sprite2D)

public:
	FluidDisplay2D();
	~FluidDisplay2D() override = default;

	void set_sim_target(const NodePath &p_path);
	NodePath get_sim_target() const;

	void set_display_mode(int p_mode);
	int get_display_mode() const;

	void set_auto_size(bool p_enable);
	bool is_auto_size() const;

	void _ready() override;
	void _process(double p_delta) override;

protected:
	static void _bind_methods();

private:
	NodePath _sim_target;
	DisplayMode _display_mode = DisplayMode::Density;
	bool _auto_size = true;
	bool _texture_warned = false;
};

} // namespace godot
