extends Node2D

@onready var sim: GPUStableFluids2D = $GPUStableFluids2D
@onready var info: Label = $UI/InfoPanel/VBox/InfoLabel
@onready var status: Label = $UI/InfoPanel/VBox/StatusLabel

func _ready():
	sim.set_viscosity(0.0)
	sim.set_poisson_iterations(40)
	sim.set_vorticity_enabled(true)
	sim.set_color_decay(0.001)
	_update_info()

func _input(ev):
	if not (ev is InputEventKey and ev.pressed): return
	match ev.keycode:
		KEY_1: _apply_water()
		KEY_2: _apply_smoke()
		KEY_3: _apply_fire()
		KEY_4: _apply_swirl()
		KEY_5: _apply_calm()
		KEY_R: sim.reset(); _update_info()

func _apply_water():
	sim.reset()
	sim.set_viscosity(0.0)
	sim.set_poisson_iterations(40)
	sim.set_vorticity_enabled(false)
	sim.set_vorticity_scale(0.1)
	sim.set_color_decay(0.0003)
	sim.set_velocity_decay(0.0)
	sim.set_ink_color(Color(0.2, 0.4, 1, 0.7))
	var e := _ensure_emitter()
	e.set_emission_preset(4) # Fountain
	e.set_emit_color(Color(0.2, 0.4, 1, 0.7))
	e.set_active(true)
	status.text = "已应用: 水 (Water) 预设"
	_update_info()

func _apply_smoke():
	sim.reset()
	sim.set_viscosity(0.0)
	sim.set_poisson_iterations(30)
	sim.set_vorticity_enabled(true)
	sim.set_vorticity_scale(0.3)
	sim.set_color_decay(0.002)
	sim.set_velocity_decay(-1.0)
	sim.set_ink_color(Color(0.3, 0.3, 0.3, 0.4))
	var e := _ensure_emitter()
	e.set_emission_preset(3) # Smoke
	e.set_emit_color(Color(0.3, 0.3, 0.3, 0.4))
	e.set_active(true)
	status.text = "已应用: 烟 (Smoke) 预设"
	_update_info()

func _apply_fire():
	sim.reset()
	sim.set_viscosity(0.0)
	sim.set_poisson_iterations(30)
	sim.set_vorticity_enabled(true)
	sim.set_vorticity_scale(0.5)
	sim.set_color_decay(0.003)
	sim.set_velocity_decay(-1.0)
	sim.set_ink_color(Color(1, 0.4, 0.1, 0.7))
	var e := _ensure_emitter()
	e.set_emission_preset(6) # Fire
	e.set_emit_color(Color(1, 0.4, 0.1, 0.7))
	e.set_active(true)
	status.text = "已应用: 火 (Fire) 预设"
	_update_info()

func _apply_swirl():
	sim.reset()
	sim.set_viscosity(0.0)
	sim.set_poisson_iterations(50)
	sim.set_vorticity_enabled(true)
	sim.set_vorticity_scale(1.5)
	sim.set_color_decay(0.0005)
	sim.set_velocity_decay(-1.0)
	sim.set_ink_color(Color(0.8, 0.2, 1, 0.8))
	var e := _ensure_emitter()
	e.set_emission_preset(2) # Mist
	e.set_emit_color(Color(0.8, 0.2, 1, 0.8))
	e.set_active(true)
	status.text = "已应用: 强漩涡 (Swirl) 预设"
	_update_info()

func _apply_calm():
	sim.reset()
	sim.set_viscosity(0.001)
	sim.set_poisson_iterations(20)
	sim.set_vorticity_enabled(false)
	sim.set_color_decay(0.0008)
	sim.set_velocity_decay(0.001)
	sim.set_ink_color(Color(0.5, 0.8, 0.5, 0.5))
	if has_node("PresetEmitter"): get_node("PresetEmitter").set_active(false)
	status.text = "已应用: 平静 (Calm) 预设"
	_update_info()

func _ensure_emitter() -> FluidEmitter2D:
	if has_node("PresetEmitter"):
		return $PresetEmitter as FluidEmitter2D
	var e := FluidEmitter2D.new()
	e.set_sim_target(NodePath("../GPUStableFluids2D"))
	e.position = Vector2(0, -200)
	e.name = "PresetEmitter"
	add_child(e)
	return e

func _update_info():
	info.text = """[b]FluidDomain2D[/b] — 域配置资源
可保存/加载流体模拟的完整参数预设。
每个预设封装了粘度、涡量、颜色衰减等配置。

按键:
  1 — 水 (Water):  无涡量, 蓝色喷泉
  2 — 烟 (Smoke):  有涡量, 灰色烟雾
  3 — 火 (Fire):   强涡量, 橙红火焰
  4 — 强旋 (Swirl): 高涡量, 紫色薄雾
  5 — 平静 (Calm):  有粘度, 柔和流动
  R — 重置

鼠标左键 — 扰动流体"""
