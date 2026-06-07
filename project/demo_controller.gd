extends Node2D
@onready var sim = $GPUStableFluids2D
@onready var mi = $GPUStableFluids2D/FluidMouseInteractor2D
func _ready() -> void: sim.set_viscosity(0.0); sim.set_poisson_iterations(40); sim.set_vorticity_enabled(true); sim.set_vorticity_scale(0.4)
func _input(ev: InputEvent) -> void:
	if not (ev is InputEventKey and ev.pressed): return
	match ev.keycode:
		KEY_H: $ControlPanel.visible = !$ControlPanel.visible
		KEY_R: sim.reset()
		KEY_1,KEY_2,KEY_3:
			for i in 32:
				var a := randf()*TAU; var d := randf()*200.0; var p := Vector2(cos(a)*d, sin(a)*d)
				sim.queue_draw_request(p, Color(randf(),randf()*.5,1-randf(),.8), p.normalized()*randf()*3, randf()*2+1, randf()*3+1)
		KEY_4: mi.set_brush_size(mi.get_brush_size()+1)
		KEY_5: mi.set_brush_size(max(1, mi.get_brush_size()-1))
