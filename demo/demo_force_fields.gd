extends Node2D

@onready var sim: GPUStableFluids2D = $GPUStableFluids2D
@onready var info: Label = $UI/InfoPanel/VBox/InfoLabel
@onready var status: Label = $UI/InfoPanel/VBox/StatusLabel

const FORCES = ["Wind 风", "Gravity 重力", "Vortex 漩涡", "Explosion 爆炸", "Magnet 磁铁"]
var emitters: Array = []
var active: Array = []

func _ready():
	sim.set_viscosity(0.0)
	sim.set_poisson_iterations(30)
	sim.set_vorticity_enabled(true)
	sim.set_color_decay(0.001)

	# Create 5 force emitters around the center
	var positions = [Vector2(0, -250), Vector2(0, 200), Vector2(0, 0), Vector2(0, 0), Vector2(250, 0)]
	for i in range(5):
		var fe := FluidForceEmitter2D.new()
		fe.set_force_preset(i + 1) # Wind=1, Gravity=2, Vortex=3, Explosion=4, Magnet=5
		fe.sim_target = $"../GPUStableFluids2D"
		fe.set_active(true)
		fe.position = positions[i]
		fe.name = "Force" + str(i)
		add_child(fe)
		emitters.append(fe)
		active.append(i == 0) # Only Wind active by default
		fe.set_active(active[i])

		# Label
		var lbl := Label.new()
		lbl.text = FORCES[i] + (" [ON]" if active[i] else " [OFF]")
		lbl.position = fe.position + Vector2(-50, 30)
		lbl.add_theme_color_override("font_color", Color(1, 0.9, 0.3, 0.9))
		lbl.add_theme_font_size_override("font_size", 12)
		lbl.name = "Label" + str(i)
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
		KEY_R: sim.reset()
	if idx >= 0:
		active[idx] = !active[idx]
		emitters[idx].set_active(active[idx])
		var lbl = get_node_or_null("Label" + str(idx)) as Label
		if lbl: lbl.text = FORCES[idx] + (" [ON]" if active[idx] else " [OFF]")
	_update_info()

func _update_info():
	var parts: Array = []
	for i in range(emitters.size()):
		var mark = "●" if active[i] else "○"
		parts.append("%d:%s%s" % [i+1, mark, FORCES[i]])
	info.text = """[b]FluidForceEmitter2D[/b] — GPU力场发射器
施加结构化力场(风/重力/漩涡等)到流体。
不注入颜色, 仅影响速度场。

按键:  1-5 切换力场 | R 重置
鼠标左键拖动 — 注入颜色观察力的效果

%s""" % "  ".join(parts)
