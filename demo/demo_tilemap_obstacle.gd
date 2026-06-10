extends Node2D

@onready var sim: GPUStableFluids2D = $GPUStableFluids2D
@onready var info: Label = $UI/InfoPanel/InfoLabel
@onready var tm: TileMapLayer = $TileMapLayer

func _ready():
	sim.set_viscosity(0.0)
	sim.set_poisson_iterations(40)
	sim.set_vorticity_enabled(true)
	sim.set_obstacle_force_strength(12.0)
	sim.set_color_decay(0.0003)

	## Build a simple TileMapLayer maze programmatically
	#var tiles := {
		#0: Vector2i(0,0), 1:Vector2i(1,0), 2:Vector2i(2,0), 3:Vector2i(3,0),
		#4:Vector2i(4,0), 5:Vector2i(5,0), 6:Vector2i(6,0), 7:Vector2i(7,0),
		#8:Vector2i(8,0), 9:Vector2i(9,0)
	#}
#
	## Top wall
	#for x in range(-10, 11): tm.set_cell(Vector2i(x, -8), 0, Vector2i(0,0))
	## Bottom wall
	#for x in range(-10, 11): tm.set_cell(Vector2i(x, 8), 0, Vector2i(0,0))
	## Left wall
	#for y in range(-8, 9): tm.set_cell(Vector2i(-10, y), 0, Vector2i(0,0))
	## Right wall
	#for y in range(-8, 9): tm.set_cell(Vector2i(10, y), 0, Vector2i(0,0))
	## Internal baffles
	#for x in range(-6, 7): tm.set_cell(Vector2i(x, -4), 0, Vector2i(0,0))
	#for x in range(-8, 9): tm.set_cell(Vector2i(x, 4), 0, Vector2i(0,0))
	#for y in range(-4, 5): tm.set_cell(Vector2i(0, y), 0, Vector2i(0,0))
#
	## Connect FluidTileMapObstacle2D to our TileMapLayer
	#var obs = $FluidTileMapObstacle2D
	#obs.set_tile_map_path(NodePath("../TileMapLayer"))
	#obs.set_obstacle_mode(0) # AllTiles

	info.text = """FluidTileMapObstacle2D
		自动将 TileMapLayer 的瓦片转换为流体障碍物。

		上方和下方各有一个发射器释放流体,
		流体在 TileMap 墙壁间流动。
		障碍物模式: AllTiles
		填充模式: Opaque

		鼠标左键 — 在迷宫中额外绘制流体
		R — 重置"""
