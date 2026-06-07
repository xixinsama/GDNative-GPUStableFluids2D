#pragma once

#include "godot_cpp/variant/vector2.hpp"
#include "godot_cpp/variant/rid.hpp"

#include <cstdint>
#include <vector>

namespace godot {

class Node;
class FluidObstacle2D;
class IFluidObstacle;
class GPUStableFluids2D;

// CPU-side rasterizer that collects all nodes in the "fluid_obstacles" group,
// rasterizes their shapes into a raw byte buffer, and uploads to the GPU.
class FluidObstacleDrawer {
public:
	FluidObstacleDrawer() = default;

	void initialize(GPUStableFluids2D *p_sim);

	// Called each frame: scan the group, rasterize, upload dirty data.
	void process_frame();

	// Mark as dirty (force full redraw next frame).
	void mark_dirty() { _needs_redraw = true; }

private:
	GPUStableFluids2D *_sim = nullptr;
	int _obs_w = 0, _obs_h = 0;
	Vector2 _res;
	Vector2 _world_size;
	Vector2 _domain_center;

	std::vector<uint8_t> _obs_data;
	bool _needs_redraw = true;

	// Previous-frame hash for dirty detection
	int _last_obs_hash = 0;
	Vector2 _last_domain_center;

	void _clear_buffer();
	void _draw_node(Node *p_node);
	void _draw_obstacle_node(FluidObstacle2D *p_obs, IFluidObstacle *p_iface);
	void _upload();

	int _compute_hash();
	static constexpr int BYTES_PER_PIXEL = 16;
};

} // namespace godot
