extends Node2D

@onready var sim: GPUStableFluids2D = $GPUStableFluids2D
@onready var viz: FluidVorticityVisualizer2D = $FluidVorticityVisualizer2D
@onready var info: Label = $UI/InfoPanel/VBox/InfoLabel
@onready var status: Label = $UI/InfoPanel/VBox/StatusLabel

const MODES = ["Density", "Velocity", "Pressure", "Divergence", "Vorticity"]
var current_mode := 4 # default Vorticity

func _ready():
	sim.set_viscosity(0.0)
	sim.set_poisson_iterations(30)
	sim.set_vorticity_enabled(true)
	sim.set_vorticity_scale(0.5) # higher for better viz
	sim.set_color_decay(0.0005)

	viz.set_display_mode(current_mode)
	_update_info()

func _process(_dt):
	status.text = "涡量约束: %s | 强度: %.2f | 模式: %s" % ["ON" if sim.is_vorticity_enabled() else "OFF", sim.get_vorticity_scale(), MODES[current_mode]]

func _input(ev):
	if not (ev is InputEventKey and ev.pressed): return
	match ev.keycode:
		KEY_1: current_mode = 0
		KEY_2: current_mode = 1
		KEY_3: current_mode = 2
		KEY_4: current_mode = 3
		KEY_5: current_mode = 4
		KEY_Q: sim.set_vorticity_scale(maxf(0.05, sim.get_vorticity_scale() - 0.1))
		KEY_W: sim.set_vorticity_scale(minf(3.0, sim.get_vorticity_scale() + 0.1))
		KEY_E: sim.set_vorticity_enabled(!sim.is_vorticity_enabled())
		KEY_R: sim.reset()
	viz.set_display_mode(current_mode)
	_update_info()

func _update_info():
	info.text = """[b]FluidVorticityVisualizer2D[/b] — 内部场可视化
伪彩色渲染涡量/压力/散度等隐藏场。
帮助理解和调试流体模拟。

当前模式: [color=#ffaa00]%s[/color]

按键:
  1-5 — 切换显示模式
  Q/W — 涡量强度 -/+
  E   — 切换涡量约束
  R   — 重置

鼠标左键 — 创建湍流""" % MODES[current_mode]
