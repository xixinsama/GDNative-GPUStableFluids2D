extends Node2D

@onready var sim: GPUStableFluids2D = $GPUStableFluids2D
@onready var info: Label = $UI/InfoPanel/VBox/InfoLabel
@onready var status: Label = $UI/InfoPanel/VBox/StatusLabel

func _ready():
	sim.set_viscosity(0.0)
	sim.set_poisson_iterations(30)
	sim.set_vorticity_enabled(true)
	sim.set_color_decay(0.0005)
	sim.set_ink_color(Color(1, 1, 1, 1))

	# Configure the light occluder
	var occ = $FluidLightOccluder2D
	occ.set_density_threshold(0.05)
	occ.set_update_frequency(20)
	occ.set_max_contours(6)

	# Set dark clear color for visibility
	sim.set_clear_color(Color(0, 0, 0, 0))
	sim.reset()

	_update_info()

func _process(_dt):
	var occ = $FluidLightOccluder2D
	status.text = "密度阈值: %.2f | 遮挡器: %d/%d | 更新频率: %dHz" % [occ.get_density_threshold(), 0, occ.get_max_contours(), occ.get_update_frequency()]

func _input(ev):
	if not (ev is InputEventKey and ev.pressed): return
	var occ = $FluidLightOccluder2D
	match ev.keycode:
		KEY_Q: occ.set_density_threshold(maxf(0.01, occ.get_density_threshold() - 0.02))
		KEY_W: occ.set_density_threshold(minf(0.5, occ.get_density_threshold() + 0.02))
		KEY_R: sim.reset()
	_update_info()

func _update_info():
	var occ = $FluidLightOccluder2D
	info.text = """[b]FluidLightOccluder2D[/b] — 光照遮挡
	将流体密度场转为 LightOccluder2D。
	密度高于阈值的区域会遮挡光源。
	顶部有点光源，流体遮挡产生动态阴影。

	按键:
	  Q/W — 密度阈值 -/+: %.2f
	  R   — 重置
	  鼠标左键 — 绘制流体（观察阴影变化）""" % occ.get_density_threshold()
