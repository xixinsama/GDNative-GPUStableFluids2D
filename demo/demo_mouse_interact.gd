extends Node2D

@onready var sim: GPUStableFluids2D = $GPUStableFluids2D
@onready var mouse: FluidMouseInteractor2D = $GPUStableFluids2D/FluidMouseInteractor2D
@onready var info: Label = $UI/InfoPanel/VBox/InfoLabel
@onready var status: Label = $UI/InfoPanel/VBox/StatusLabel

const MODES = ["Draw 绘制", "Push 推", "Pull 拉", "Swirl 漩涡"]
const COLORS = [Color(1,0.6,0.2,0.8), Color(0.3,0.8,1,0.3), Color(1,0.3,1,0.3), Color(0.3,1,1,0.8)]

func _ready():
	sim.set_viscosity(0.0)
	sim.set_poisson_iterations(40)
	sim.set_vorticity_enabled(true)
	sim.set_color_decay(0.0003)
	_set_mode(3) # default to Swirl
	_update_info()

func _process(_dt):
	if not mouse: return
	status.text = "模式: %s | 笔刷: %.0f | 速度缩放: %.2f" % [MODES[mouse.get_interaction_mode()], mouse.get_brush_size(), mouse.get_velocity_scale()]

func _input(ev):
	if not (ev is InputEventKey and ev.pressed): return
	match ev.keycode:
		KEY_1: _set_mode(0)
		KEY_2: _set_mode(1)
		KEY_3: _set_mode(2)
		KEY_4: _set_mode(3)
		KEY_Q: mouse.set_velocity_scale(clampf(mouse.get_velocity_scale() - 0.05, 0.01, 1.0)); _update_info()
		KEY_W: mouse.set_velocity_scale(clampf(mouse.get_velocity_scale() + 0.05, 0.01, 1.0)); _update_info()
		KEY_BRACKETLEFT: mouse.set_brush_size(maxf(1, mouse.get_brush_size() - 1))
		KEY_BRACKETRIGHT: mouse.set_brush_size(mouse.get_brush_size() + 1)
		KEY_R: sim.reset()

func _set_mode(m: int):
	if mouse:
		mouse.set_interaction_mode(m)
		mouse.set_draw_color(COLORS[m])
	_update_info()

func _update_info():
	var m = mouse.get_interaction_mode() if mouse else 0
	info.text = """[b]FluidMouseInteractor2D[/b] — 鼠标/触摸交互
四种模式注入颜色+速度到流体。

当前的模式: [color=#ffaa00]%s[/color]

按键:
  1/2/3/4 — 切换模式
  Q/W   — 速度缩放 -/+
  [/]   — 笔刷大小 -/+
  R     — 重置

鼠标左键拖动 — 与流体交互""" % MODES[m]
