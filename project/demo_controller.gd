extends Node2D
## Demo controller — keyboard shortcuts, emitter presets, parameter control

var _sim = null
var _mi = null

func _ready() -> void:
	# Find simulation and mouse interactor by path (defensive)
	_sim = get_node_or_null("GPUStableFluids2D")
	if _sim:
		_sim.set_viscosity(0.0)
		_sim.set_poisson_iterations(40)
		_sim.set_vorticity_enabled(true)
		_sim.set_vorticity_scale(0.4)
		_mi = _sim.get_node_or_null("FluidMouseInteractor2D")

func _input(ev: InputEvent) -> void:
	if not (ev is InputEventKey and ev.pressed):
		return
	if not _sim:
		return

	match ev.keycode:
		KEY_H:
			var ctrl := get_node_or_null("ControlPanel")
			if ctrl: ctrl.visible = !ctrl.visible

		KEY_R:
			_sim.reset()
			# Re-spawn burst for visual feedback
			_spawn_burst(16)

		KEY_1: _spawn_burst(8)
		KEY_2: _spawn_burst(32)
		KEY_3: _spawn_burst(64)

		KEY_4:
			if _mi: _mi.set_brush_size(_mi.get_brush_size() + 1.0)
		KEY_5:
			if _mi: _mi.set_brush_size(max(1.0, _mi.get_brush_size() - 1.0))

		KEY_SPACE:
			_spawn_burst(16)

func _spawn_burst(count: int) -> void:
	if not _sim: return
	for _i in count:
		var angle := randf() * TAU
		var dist := randf() * 300.0
		var pos := Vector2(cos(angle) * dist, sin(angle) * dist)
		var vel := pos.normalized() * randf() * 5.0
		var col := Color(randf(), randf() * 0.5, 1.0 - randf(), 0.8)
		_sim.queue_draw_request(pos, col, vel, randf() * 2.0 + 1.0, randf() * 3.0 + 1.0)
