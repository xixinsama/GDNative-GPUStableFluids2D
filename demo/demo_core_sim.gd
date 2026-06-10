extends Node2D

@onready var sim: GPUStableFluids2D = $GPUStableFluids2D
@onready var mouse: FluidMouseInteractor2D = $FluidMouseInteractor2D
@onready var info: Label = $UI/InfoPanel/VBox/InfoLabel
@onready var fps_label: Label = $UI/InfoPanel/VBox/FPSLabel

func _ready():
	sim.set_viscosity(0.0)
	sim.set_poisson_iterations(40)
	sim.set_vorticity_enabled(true)
	sim.set_vorticity_scale(0.4)
	sim.set_color_decay(0.0005)
	sim.set_velocity_decay(-1.0)
	_update_info()

func _process(_dt):
	fps_label.text = "FPS: %d | Grid: %dx%d" % [Engine.get_frames_per_second(), sim.get_width(), sim.get_height()]

func _input(ev):
	if not (ev is InputEventKey and ev.pressed): return
	match ev.keycode:
		KEY_1: sim.set_viscosity(0.0); _update_info()
		KEY_2: sim.set_viscosity(0.005); _update_info()
		KEY_3: sim.set_vorticity_enabled(!sim.is_vorticity_enabled()); _update_info()
		KEY_4: sim.set_poisson_iterations(clampi(sim.get_poisson_iterations() + 10, 10, 200)); _update_info()
		KEY_5: sim.set_poisson_iterations(clampi(sim.get_poisson_iterations() - 10, 10, 200)); _update_info()
		KEY_R: sim.reset()
		KEY_BRACKETLEFT: if mouse: mouse.set_brush_size(maxf(1, mouse.get_brush_size() - 1))
		KEY_BRACKETRIGHT: if mouse: mouse.set_brush_size(mouse.get_brush_size() + 1)

func _update_info():
	info.text = """[b]GPUStableFluids2D[/b] — 核心模拟节点
管理所有 GPU 资源和 11 步计算管线。
其他所有节点均依赖此节点。

按键:
  1/2 — 粘度 0 / 0.005
  3   — 切换涡量约束: %s
  4/5 — Poisson 迭代: %d
  [/] — 笔刷大小
  R   — 重置模拟
  鼠标左键拖动 — 绘制流体""" % ["ON" if sim.is_vorticity_enabled() else "OFF", sim.get_poisson_iterations()]
