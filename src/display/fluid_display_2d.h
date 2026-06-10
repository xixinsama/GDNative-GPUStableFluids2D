#pragma once

#include "godot_cpp/classes/sprite2d.hpp"
#include "godot_cpp/classes/texture2drd.hpp"

#include "core/fluid_types.h"

namespace godot {

class GPUStableFluids2D;

// Renders the fluid simulation output to the screen.
// Must be a direct child of a GPUStableFluids2D node.
//
// The parent GPUStableFluids2D calls bind_to_sim() after its own GPU
// initialization is complete — the display does NOT reach up to the
// parent on its own.  This avoids the Godot _ready() order problem
// (children _ready runs before parent _ready).
class FluidDisplay2D : public Sprite2D {
	GDCLASS(FluidDisplay2D, Sprite2D)

public:
	FluidDisplay2D();
	~FluidDisplay2D() override = default;

	void set_display_mode(int p_mode);
	int get_display_mode() const;

	void set_show_debug_bounds(bool p_show);
	bool is_showing_debug_bounds() const;

	// Called by the parent GPUStableFluids2D once the GPU is ready.
	// Passes the output textures and auto-sizes the display.
	void bind_to_sim(GPUStableFluids2D *p_sim);

	void _ready() override;
	void _process(double p_delta) override;

protected:
	static void _bind_methods();

private:
	DisplayMode _display_mode = DisplayMode::Density;
	bool _show_debug_bounds = true;
	bool _texture_warned = false;

	GPUStableFluids2D *_sim = nullptr;

	void _update_texture();
};

} // namespace godot
