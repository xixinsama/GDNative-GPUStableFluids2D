extends Node2D

@onready var sim: GPUStableFluids2D = $GPUStableFluids2D
@onready var display_a: FluidDisplay2D = $DisplayA
@onready var display_b: FluidDisplay2D = $DisplayB
@onready var info: Label = $UI/InfoPanel/VBox/InfoLabel
@onready var status: Label = $UI/InfoPanel/VBox/StatusLabel

const MODE_NAMES = ["Density 密度", "Velocity 速度", "Pressure 压力", "Divergence 散度", "Vorticity 涡量"]
var mode_a := 0
var mode_b := 1

func _ready():
	sim.set_viscosity(0.0)
	sim.set_poisson_iterations(40)
	sim.set_vorticity_enabled(true)
	sim.set_color_decay(0.001)

	# Left display (small, offset)
	display_a.position = Vector2(-550, -300)
	display_a.scale = Vector2(0.65, 0.65)

	# Right display (small, offset)
	display_b.position = Vector2(30, -300)
	display_b.scale = Vector2(0.65, 0.65)

	_update_info()

func _input(ev):
	if not (ev is InputEventKey and ev.pressed): return
	match ev.keycode:
		KEY_1: mode_a = (mode_a + 1) % 5; display_a.set_display_mode(mode_a)
		KEY_2: mode_b = (mode_b + 1) % 5; display_b.set_display_mode(mode_b)
		KEY_3: display_a.set_auto_size(!display_a.is_auto_size())
		KEY_R: sim.reset()
	_update_info()

func _update_info():
	info.text = """[b]FluidDisplay2D[/b] — 流体可视化输出
将模拟纹理渲染到屏幕。可并排多个显示不同内部场。

左: [color=#ffaa00]%s[/color] | 右: [color=#ffaa00]%s[/color]
auto_size: %s

按键:
  1 — 切换左显示模式
  2 — 切换右显示模式
  3 — 切换 auto_size
  R — 重置

鼠标左键 — 在模拟域中央绘制流体""" % [MODE_NAMES[mode_a], MODE_NAMES[mode_b], "ON" if display_a.is_auto_size() else "OFF"]
