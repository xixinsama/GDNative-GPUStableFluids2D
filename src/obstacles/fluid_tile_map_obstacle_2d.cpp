#include "fluid_tile_map_obstacle_2d.h"
#include "fluid_obstacle_2d.h"

#include "godot_cpp/classes/tile_map_layer.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/core/class_db.hpp"

namespace godot {

void FluidTileMapObstacle2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_tile_map_path", "path"), &FluidTileMapObstacle2D::set_tile_map_path);
	ClassDB::bind_method(D_METHOD("get_tile_map_path"), &FluidTileMapObstacle2D::get_tile_map_path);
	ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "tile_map_path"), "set_tile_map_path", "get_tile_map_path");

	ClassDB::bind_method(D_METHOD("set_obstacle_mode", "mode"), &FluidTileMapObstacle2D::set_obstacle_mode);
	ClassDB::bind_method(D_METHOD("get_obstacle_mode"), &FluidTileMapObstacle2D::get_obstacle_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "obstacle_mode", PROPERTY_HINT_ENUM, "AllTiles,LayerSpecific,PhysicsLayer"), "set_obstacle_mode", "get_obstacle_mode");

	ClassDB::bind_method(D_METHOD("set_fill_mode", "mode"), &FluidTileMapObstacle2D::set_fill_mode);
	ClassDB::bind_method(D_METHOD("get_fill_mode"), &FluidTileMapObstacle2D::get_fill_mode);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "fill_mode", PROPERTY_HINT_ENUM, "Opaque,PerTileShape,CollisionShape"), "set_fill_mode", "get_fill_mode");

	ClassDB::bind_method(D_METHOD("set_physics_layer_index", "index"), &FluidTileMapObstacle2D::set_physics_layer_index);
	ClassDB::bind_method(D_METHOD("get_physics_layer_index"), &FluidTileMapObstacle2D::get_physics_layer_index);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "physics_layer_index", PROPERTY_HINT_RANGE, "0,31"), "set_physics_layer_index", "get_physics_layer_index");

	ClassDB::bind_method(D_METHOD("set_terrain_set", "set"), &FluidTileMapObstacle2D::set_terrain_set);
	ClassDB::bind_method(D_METHOD("get_terrain_set"), &FluidTileMapObstacle2D::get_terrain_set);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "terrain_set", PROPERTY_HINT_RANGE, "-1,31"), "set_terrain_set", "get_terrain_set");
}

void FluidTileMapObstacle2D::set_tile_map_path(const NodePath &v) { _tile_map_path = v; }
NodePath FluidTileMapObstacle2D::get_tile_map_path() const { return _tile_map_path; }
void FluidTileMapObstacle2D::set_obstacle_mode(int v) { _obstacle_mode = (TileObstacleMode)v; }
int FluidTileMapObstacle2D::get_obstacle_mode() const { return (int)_obstacle_mode; }
void FluidTileMapObstacle2D::set_fill_mode(int v) { _fill_mode = (TileFillMode)v; }
int FluidTileMapObstacle2D::get_fill_mode() const { return (int)_fill_mode; }
void FluidTileMapObstacle2D::set_physics_layer_index(int v) { _physics_layer_index = v; }
int FluidTileMapObstacle2D::get_physics_layer_index() const { return _physics_layer_index; }
void FluidTileMapObstacle2D::set_terrain_set(int v) { _terrain_set = v; }
int FluidTileMapObstacle2D::get_terrain_set() const { return _terrain_set; }

void FluidTileMapObstacle2D::_ready() {
	if (Engine::get_singleton()->is_editor_hint()) return;
	// Create child FluidObstacle2D instances for each tile cell
	// This is a simplified version — a full implementation would do per-tile rasterization
	add_to_group("fluid_obstacles");
}

void FluidTileMapObstacle2D::_rasterize_tile_map() {
	// Full implementation would iterate tile cells and write to obstacle buffer
	// For now, the node itself acts as a marker in the "fluid_obstacles" group
}

} // namespace godot
