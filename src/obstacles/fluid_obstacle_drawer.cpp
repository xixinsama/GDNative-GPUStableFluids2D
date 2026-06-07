#include "fluid_obstacle_drawer.h"
#include "fluid_obstacle_2d.h"
#include "GPU_stable_fluids_2D_init.h"
#include "utils/fluid_coord_utils.h"
#include "utils/polygon_rasterizer.h"

#include "godot_cpp/classes/node2d.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/classes/rendering_device.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"

#include <cstring>

namespace godot {

void FluidObstacleDrawer::initialize(GPUStableFluids2D *p_sim) {
	_sim = p_sim;
	_obs_w = p_sim->get_width();
	_obs_h = p_sim->get_height();
	_obs_data.resize(_obs_w * _obs_h * BYTES_PER_PIXEL);
	_needs_redraw = true;
}

void FluidObstacleDrawer::_clear_buffer() {
	std::memset(_obs_data.data(), 0, _obs_data.size());
}

int FluidObstacleDrawer::_compute_hash() {
	// Simple hash: just use the number of children and domain center
	int h = 0;
	SceneTree *tree = _sim->get_tree();
	if (tree) {
		h ^= tree->get_node_count_in_group("fluid_obstacles");
	}
	h ^= (int)(_domain_center.x * 100.0f);
	h ^= (int)(_domain_center.y * 100.0f);
	return h;
}

void FluidObstacleDrawer::process_frame() {
	if (!_sim) return;

	_res        = Vector2((float)_obs_w, (float)_obs_h);
	_world_size = _sim->get_fluid_world_size();
	_domain_center = _sim->get_global_position();

	int hash = _compute_hash();
	if (hash != _last_obs_hash || _domain_center != _last_domain_center) {
		_needs_redraw = true;
		_last_obs_hash = hash;
		_last_domain_center = _domain_center;
	}

	if (!_needs_redraw) return;

	_clear_buffer();

	// Scan all obstacles in the group
	SceneTree *tree = _sim->get_tree();
	if (tree) {
		TypedArray<Node> nodes = tree->get_nodes_in_group("fluid_obstacles");
		for (int i = 0; i < nodes.size(); i++) {
			Node *node = Object::cast_to<Node>(nodes[i]);
			if (node) _draw_node(node);
		}
	}

	_upload();
	_needs_redraw = false;
}

void FluidObstacleDrawer::_draw_node(Node *p_node) {
	// Try FluidObstacle2D first
	FluidObstacle2D *obs = Object::cast_to<FluidObstacle2D>(p_node);
	if (obs) {
		IFluidObstacle *iface = obs; // FluidObstacle2D implements IFluidObstacle
		_draw_obstacle_node(obs, iface);
		return;
	}

	// Check parent for IFluidObstacle (e.g. CollisionShape2D as child of FluidObstacle2D)
	Node *parent = p_node->get_parent();
	while (parent) {
		IFluidObstacle *iface = dynamic_cast<IFluidObstacle *>(parent);
		if (iface) {
			// This node is a child of a FluidObstacle2D (e.g. CollisionShape2D)
			// Draw using the parent's velocity info
			// For now, simple rect covering the node's position
			Vector2 pos(0, 0);
			Node2D *n2d = Object::cast_to<Node2D>(p_node);
			if (n2d) pos = n2d->get_global_position();
			Vector2 size(16, 16); // Default small size
			Vector2 half = size * 0.5f;

			if (iface->get_object_linear_velocity().length_squared() > 0.001f ||
			    iface->get_object_angular_velocity() > 0.001f) {
				PolygonRasterizer::fill_rect_velocity(
					pos - half, pos + half,
					_obs_data.data(), _obs_w, _obs_h,
					_domain_center, _world_size, _res,
					iface->get_object_linear_velocity(),
					iface->get_object_angular_velocity(),
					iface->get_object_center());
			} else {
				PolygonRasterizer::fill_rect(
					pos - half, pos + half,
					_obs_data.data(), _obs_w, _obs_h,
					_domain_center, _world_size, _res);
			}
			return;
		}
		parent = parent->get_parent();
	}
}

void FluidObstacleDrawer::_draw_obstacle_node(FluidObstacle2D *p_obs, IFluidObstacle *p_iface) {
	Vector2 pos  = p_obs->get_global_position();
	Vector2 vel  = p_iface->get_object_linear_velocity();
	float   ang  = p_iface->get_object_angular_velocity();
	Vector2 ctr  = p_iface->get_object_center();

	// Default: use a 32x32 rect centered on the obstacle
	Vector2 size(32, 32);
	Vector2 half = size * 0.5f;

	bool has_velocity = (vel.length_squared() > 0.001f || ang > 0.001f);

	if (has_velocity) {
		PolygonRasterizer::fill_rect_velocity(
			pos - half, pos + half,
			_obs_data.data(), _obs_w, _obs_h,
			_domain_center, _world_size, _res,
			vel, ang, ctr);
	} else {
		PolygonRasterizer::fill_rect(
			pos - half, pos + half,
			_obs_data.data(), _obs_w, _obs_h,
			_domain_center, _world_size, _res);
	}
}

void FluidObstacleDrawer::_upload() {
	RenderingDevice *rd = _sim->get_gpu_resources()->device;
	if (!rd) return;

	PackedByteArray pba;
	pba.resize((int)_obs_data.size());
	std::memcpy(pba.ptrw(), _obs_data.data(), _obs_data.size());

	rd->texture_update(_sim->get_gpu_resources()->tex_obstacle, 0, pba);
}

} // namespace godot
