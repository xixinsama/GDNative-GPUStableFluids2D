#include "fluid_tile_map_obstacle_2d.h"
#include "fluid_obstacle_2d.h"

#include "godot_cpp/classes/tile_map_layer.hpp"
#include "godot_cpp/classes/tile_map.hpp"
#include "godot_cpp/classes/tile_set.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/core/class_db.hpp"

namespace godot {

void FluidTileMapObstacle2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_tile_map_path", "path"), &FluidTileMapObstacle2D::set_tile_map_path);
	ClassDB::bind_method(D_METHOD("get_tile_map_path"), &FluidTileMapObstacle2D::get_tile_map_path);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "tile_map_path"), "set_tile_map_path", "get_tile_map_path");

	ClassDB::bind_method(D_METHOD("set_obstacle_mode", "mode"), &FluidTileMapObstacle2D::set_obstacle_mode);
	ClassDB::bind_method(D_METHOD("get_obstacle_mode"), &FluidTileMapObstacle2D::get_obstacle_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "obstacle_mode", PROPERTY_HINT_ENUM, "AllTiles,LayerSpecific,PhysicsLayer"),
		"set_obstacle_mode", "get_obstacle_mode");

	ClassDB::bind_method(D_METHOD("set_fill_mode", "mode"), &FluidTileMapObstacle2D::set_fill_mode);
	ClassDB::bind_method(D_METHOD("get_fill_mode"), &FluidTileMapObstacle2D::get_fill_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "fill_mode", PROPERTY_HINT_ENUM, "Opaque,PerTileShape,CollisionShape"),
		"set_fill_mode", "get_fill_mode");

	ClassDB::bind_method(D_METHOD("set_physics_layer_index", "index"), &FluidTileMapObstacle2D::set_physics_layer_index);
	ClassDB::bind_method(D_METHOD("get_physics_layer_index"), &FluidTileMapObstacle2D::get_physics_layer_index);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "physics_layer_index", PROPERTY_HINT_RANGE, "0,31"),
		"set_physics_layer_index", "get_physics_layer_index");

	ClassDB::bind_method(D_METHOD("set_terrain_set", "set"), &FluidTileMapObstacle2D::set_terrain_set);
	ClassDB::bind_method(D_METHOD("get_terrain_set"), &FluidTileMapObstacle2D::get_terrain_set);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "terrain_set", PROPERTY_HINT_RANGE, "-1,31"),
		"set_terrain_set", "get_terrain_set");

	ClassDB::bind_method(D_METHOD("set_sim_target", "target"), &FluidTileMapObstacle2D::set_sim_target);
	ClassDB::bind_method(D_METHOD("get_sim_target"), &FluidTileMapObstacle2D::get_sim_target);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "sim_target", PROPERTY_HINT_NODE_TYPE, "GPUStableFluids2D"),
		"set_sim_target", "get_sim_target");
}

FluidTileMapObstacle2D::~FluidTileMapObstacle2D() { _clear_children(); }

void FluidTileMapObstacle2D::set_tile_map_path(const NodePath &v) { _tile_map_path = v; _rasterized = false; }
NodePath FluidTileMapObstacle2D::get_tile_map_path() const { return _tile_map_path; }
void FluidTileMapObstacle2D::set_obstacle_mode(int v) { _obstacle_mode = (TileObstacleMode)v; }
int FluidTileMapObstacle2D::get_obstacle_mode() const { return (int)_obstacle_mode; }
void FluidTileMapObstacle2D::set_fill_mode(int v) { _fill_mode = (TileFillMode)v; }
int FluidTileMapObstacle2D::get_fill_mode() const { return (int)_fill_mode; }
void FluidTileMapObstacle2D::set_physics_layer_index(int v) { _physics_layer_index = v; }
int FluidTileMapObstacle2D::get_physics_layer_index() const { return _physics_layer_index; }
void FluidTileMapObstacle2D::set_terrain_set(int v) { _terrain_set = v; }
int FluidTileMapObstacle2D::get_terrain_set() const { return _terrain_set; }
void FluidTileMapObstacle2D::set_sim_target(Object *p_obj) {
	if (p_obj) {
		Node *node = Object::cast_to<Node>(p_obj);
		if (node) {
			_sim_target = get_path_to(node);
		} else {
			_sim_target = NodePath();
		}
	} else {
		_sim_target = NodePath();
	}
}
Object *FluidTileMapObstacle2D::get_sim_target() const {
	if (_sim_target.is_empty()) return nullptr;
	return get_node_or_null(_sim_target);
}

void FluidTileMapObstacle2D::_clear_children() {
	TypedArray<Node> children = get_children();
	for (int i = children.size() - 1; i >= 0; i--) {
		Node *child = Object::cast_to<Node>(children[i]);
		if (child && Object::cast_to<FluidObstacle2D>(child)) {
			child->queue_free();
		}
	}
}

void FluidTileMapObstacle2D::_ready() {
	if (Engine::get_singleton()->is_editor_hint()) return;
	add_to_group("fluid_obstacles");
	_rasterize_tile_map();
}

void FluidTileMapObstacle2D::_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint()) return;
	// Re-rasterize if not yet done (handles late TileMapLayer availability)
	if (!_rasterized) {
		_rasterize_tile_map();
	}
}

void FluidTileMapObstacle2D::_rasterize_tile_map() {
	if (_tile_map_path.is_empty()) return;

	Node *target = get_node_or_null(_tile_map_path);
	if (!target) return;

	TileMapLayer *layer = Object::cast_to<TileMapLayer>(target);
	if (!layer) return;

	// Get tile size from the TileSet (or use default 16x16)
	Ref<TileSet> tile_set = layer->get_tile_set();
	Vector2 tile_size(16, 16);
	if (tile_set.is_valid()) {
		tile_size = tile_set->get_tile_size();
	}

	// Clear previous children
	_clear_children();

	// Iterate over used cells and create a FluidObstacle2D for each
	TypedArray<Vector2i> cells = layer->get_used_cells();
	for (int i = 0; i < cells.size(); i++) {
		Vector2i cell = cells[i];

		// Skip empty cells based on mode
		int source_id = layer->get_cell_source_id(cell);
		if (source_id < 0) continue;

		FluidObstacle2D *obs = memnew(FluidObstacle2D);
		obs->set_auto_detect_velocity(false);
		obs->set_velocity(Vector2(0, 0));

		// Position the obstacle at the tile center in world space
		Vector2 world_pos = layer->to_global(
			Vector2((float)cell.x * tile_size.x + tile_size.x * 0.5f,
			        (float)cell.y * tile_size.y + tile_size.y * 0.5f));
		obs->set_global_position(world_pos);

		add_child(obs);
		obs->set_owner(get_owner());
	}

	_rasterized = true;
}

} // namespace godot
