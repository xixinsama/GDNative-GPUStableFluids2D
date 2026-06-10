#pragma once

#include "godot_cpp/classes/node2d.hpp"
#include "godot_cpp/variant/node_path.hpp"

#include "core/fluid_types.h"

namespace godot {

// Converts a TileMapLayer into fluid obstacle data.
// Place in scene, point at a TileMapLayer, and it auto-generates obstacle boundaries.
class FluidTileMapObstacle2D : public Node2D {
	GDCLASS(FluidTileMapObstacle2D, Node2D)

public:
	FluidTileMapObstacle2D() = default;
	~FluidTileMapObstacle2D() override = default;

	void set_tile_map_path(const NodePath &p_path);
	NodePath get_tile_map_path() const;

	void set_obstacle_mode(int p_mode);
	int get_obstacle_mode() const;

	void set_fill_mode(int p_mode);
	int get_fill_mode() const;

	void set_physics_layer_index(int p_idx);
	int get_physics_layer_index() const;

	void set_terrain_set(int p_set);
	int get_terrain_set() const;

	void set_sim_target(const NodePath &p_path);
	NodePath get_sim_target() const;

	void _ready() override;

protected:
	static void _bind_methods();

private:
	NodePath _tile_map_path;
	TileObstacleMode _obstacle_mode = TileObstacleMode::AllTiles;
	TileFillMode _fill_mode = TileFillMode::Opaque;
	int _physics_layer_index = 0;
	int _terrain_set = -1;
	NodePath _sim_target;

	void _rasterize_tile_map();
};

} // namespace godot
