extends Node2D

@onready var sim: GPUStableFluids2D = $GPUStableFluids2D
@onready var info: Label = $UI/InfoPanel/VBox/InfoLabel
@onready var status: Label = $UI/InfoPanel/VBox/StatusLabel

const PRESETS = ["Custom", "Explosion", "Mist", "Smoke", "Fountain", "Steam", "Fire"]
var emitters: Array = []
var active: Array = []

func _ready():
	sim.set_viscosity(0.0)
	sim.set_poisson_iterations(30)
	sim.set_vorticity_enabled(true)
	sim.set_color_decay(0.0008)

	# Create 7 emitters in a row
	for i in range(7):
		var e := FluidEmitter2D.new()
		e.set_emission_preset(i + 1) # skip Custom
		e.set_sim_target(NodePath("../GPUStableFluids2D"))
		e.set_active(true)
		var x = -300.0 + i * 100.0
		e.position = Vector2(x, -200)
		e.name = "Emitter" + str(i)
		add_child(e)
		emitters.append(e)
		active.append(true)

		# Label for each emitter
		var lbl := Label.new()
		lbl.text = PRESETS[i + 1]
		lbl.position = e.position + Vector2(-30, 20)
		lbl.add_theme_color_override("font_color", Color(1, 1, 1, 0.8))
		lbl.add_theme_font_size_override("font_size", 12)
		add_child(lbl)

	_update_info()

func _input(ev):
	if not (ev is InputEventKey and ev.pressed): return
	var idx = -1
	match ev.keycode:
		KEY_1: idx = 0
		KEY_2: idx = 1
		KEY_3: idx = 2
		KEY_4: idx = 3
		KEY_5: idx = 4
		KEY_6: idx = 5
		KEY_7: idx = 6
		KEY_0:
			for i in range(emitters.size()):
				active[i] = !active[i]
				emitters[i].set_active(active[i])
		KEY_R: sim.reset()

	if idx >= 0 and idx < emitters.size():
		active[idx] = !active[idx]
		emitters[idx].set_active(active[idx])
	_update_info()

func _update_info():
	var parts: Array = []
	for i in range(emitters.size()):
		var mark = "[color=#00ff00]●[/color]" if active[i] else "[color=#ff4444]○[/color]"
		parts.append("%d %s" % [i + 1, mark])
	info.text = """[b]FluidEmitter2D[/b] — 流体粒子发射器
7 种预设效果, 持续发射颜色+速度到流体。

按键:
  1-7 — 切换各个发射器
  0   — 全部切换
  R   — 重置模拟

%s""" % "  ".join(parts)
