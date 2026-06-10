extends Node2D

@onready var sim: GPUStableFluids2D = $GPUStableFluids2D
@onready var info: Label = $UI/InfoPanel/VBox/InfoLabel
@onready var status: Label = $UI/InfoPanel/VBox/StatusLabel

func _ready():
	sim.set_viscosity(0.0)
	sim.set_poisson_iterations(40)
	sim.set_vorticity_enabled(true)
	sim.set_obstacle_force_strength(10.0)
	sim.set_color_decay(0.0003)
	_update_info()

func _process(_dt):
	status.text = "障碍物力强度: %.0f | 障碍物组节点数: %d" % [sim.get_obstacle_force_strength(), get_tree().get_node_count_in_group("fluid_obstacles")]

func _input(ev):
	if not (ev is InputEventKey and ev.pressed): return
	match ev.keycode:
		KEY_Q: sim.set_obstacle_force_strength(maxf(1, sim.get_obstacle_force_strength() - 2)); _update_info()
		KEY_W: sim.set_obstacle_force_strength(sim.get_obstacle_force_strength() + 2); _update_info()
		KEY_R: sim.reset(); _update_info()

func _update_info():
	info.text = """[b]FluidObstacle2D[/b] — 流体障碍物
定义不可穿透的固体边界。
自动加入 "fluid_obstacles" 组, 被障碍物绘制器发现。

上方有 StaticBody2D 障碍物 + 下方 RigidBody2D 运动小球
鼠标在障碍物间绘制流体, 观察阻挡和运动效果。

按键:
  Q/W — 障碍物力强度 -/+
  R   — 重置
  鼠标左键拖动 — 绘制流体
  当前强度: %.0f""" % sim.get_obstacle_force_strength()
