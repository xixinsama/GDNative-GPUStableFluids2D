extends Node2D
## Demo controller — keyboard shortcuts + real-time parameter control

@onready var sim = $GPUStableFluids2D
@onready var mi = $GPUStableFluids2D/FluidMouseInteractor2D
var _ctrl_vis := true

func _ready() -> void:
	sim.set_viscosity(0.0); sim.set_poisson_iterations(40)
	sim.set_vorticity_enabled(true); sim.set_vorticity_scale(0.4)

func _input(ev: InputEvent) -> void:
	if not (ev is InputEventKey and ev.pressed): return
	match ev.keycode:
		KEY_H: $ControlPanel.visible = !$ControlPanel.visible
		KEY_R: sim.reset()
		KEY_1: _burst(0); KEY_2: _burst(1); KEY_3: _burst(2)
		KEY_4: mi.set_brush_size(mi.get_brush_size() + 1)
		KEY_5: mi.set_brush_size(mi.get_brush_size() - 1)

func _burst(mode: int) -> void:
	for i in 32:
		var a := randf() * TAU; var d := randf() * 200.0
		var p := Vector2(cos(a) * d, sin(a) * d)
		var c := Color(randf(), randf()*.5, 1-randf(), .8) if mode==0 else Color(.9,.9,.9,.3)
		sim.queue_draw_request(p, c, p.normalized()*randf()*3, randf()*2+1, randf()*3+1)
