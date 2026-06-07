#pragma once

#include "godot_cpp/classes/node2d.hpp"
#include "godot_cpp/variant/node_path.hpp"

#include "core/fluid_types.h"

namespace godot {

// Debug visualizer: renders internal fields (vorticity, pressure, divergence) as colored overlay.
class FluidVorticityVisualizer2D : public Node2D {
	GDCLASS(FluidVorticityVisualizer2D, Node2D)

public:
	FluidVorticityVisualizer2D() = default;
	~FluidVorticityVisualizer2D() override = default;

	void set_sim_source_path(const NodePath &v); NodePath get_sim_source_path() const;
	void set_display_mode(int v); int get_display_mode() const;
	void set_auto_update(bool v); bool is_auto_update() const;

	void _ready() override;
	void _process(double p_delta) override;

protected:
	static void _bind_methods();

private:
	NodePath _sim_source_path;
	DisplayMode _display_mode = DisplayMode::Vorticity;
	bool _auto_update = true;
};

} // namespace godot
