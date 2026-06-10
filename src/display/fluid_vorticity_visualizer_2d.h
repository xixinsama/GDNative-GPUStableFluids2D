#pragma once

#include "godot_cpp/classes/sprite2d.hpp"
#include "godot_cpp/variant/node_path.hpp"

#include "core/fluid_types.h"

namespace godot {

// Debug visualizer: renders internal simulation fields (vorticity, pressure,
// divergence, velocity) as a colored overlay.  Works like FluidDisplay2D but
// for debug data — inherits Sprite2D so it can display textures directly.
class FluidVorticityVisualizer2D : public Sprite2D {
	GDCLASS(FluidVorticityVisualizer2D, Sprite2D)

public:
	FluidVorticityVisualizer2D();
	~FluidVorticityVisualizer2D() override = default;

	void set_sim_target(const NodePath &v); NodePath get_sim_target() const;
	void set_display_mode(int v); int get_display_mode() const;
	void set_auto_update(bool v); bool is_auto_update() const;

	void _ready() override;
	void _process(double p_delta) override;

protected:
	static void _bind_methods();

private:
	NodePath _sim_target;
	DisplayMode _display_mode = DisplayMode::Vorticity;
	bool _auto_update = true;
	bool _texture_warned = false;

	class GPUStableFluids2D *_find_sim() const;
};

} // namespace godot
