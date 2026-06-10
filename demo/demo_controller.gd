extends Node2D
## Demo controller — keyboard shortcuts, emitter presets, parameter control

@onready var gpu_stable_fluids_2d: GPUStableFluids2D = $GPUStableFluids2D
@onready var fluid_mouse_interactor_2d: FluidMouseInteractor2D = $GPUStableFluids2D/FluidMouseInteractor2D
@onready var control_panel: Control = $UI/ControlPanel

func _ready() -> void:
	print("[DemoController] _ready() START")
	print("[DemoController] gpu_stable_fluids_2d = ", gpu_stable_fluids_2d)
	#print("[DemoController] fluid_mouse_interactor_2d = ", fluid_mouse_interactor_2d)
	print("[DemoController] control_panel = ", control_panel)

	if gpu_stable_fluids_2d:
		print("[DemoController] Setting sim params...")
		gpu_stable_fluids_2d.set_viscosity(0.0)
		gpu_stable_fluids_2d.set_poisson_iterations(40)
		gpu_stable_fluids_2d.set_vorticity_enabled(true)
		gpu_stable_fluids_2d.set_vorticity_scale(0.4)
		print("[DemoController] Sim params set OK")
	else:
		printerr("[DemoController] ERROR: gpu_stable_fluids_2d is NULL!")

	print("[DemoController] _ready() DONE")

func _input(ev: InputEvent) -> void:
	if not (ev is InputEventKey and ev.pressed):
		return
	if not gpu_stable_fluids_2d:
		printerr("[DemoController] ERROR: gpu_stable_fluids_2d is NULL in _input!")
		return

	print("[DemoController] Key pressed: ", ev.keycode, " key_label=", ev.key_label)

	match ev.keycode:
		KEY_H:
			control_panel.visible = !control_panel.visible
			print("[DemoController] Toggle panel: ", control_panel.visible)

		KEY_R:
			print("[DemoController] Reset sim + spawn burst 16")
			gpu_stable_fluids_2d.reset()
			# Re-spawn burst for visual feedback
			_spawn_burst(16)

		KEY_1:
			print("[DemoController] Spawn burst 8")
			_spawn_burst(8)
		KEY_2:
			print("[DemoController] Spawn burst 32")
			_spawn_burst(32)
		KEY_3:
			print("[DemoController] Spawn burst 64")
			_spawn_burst(64)

		KEY_4:
			if fluid_mouse_interactor_2d:
				var bs: float = fluid_mouse_interactor_2d.get_brush_size() + 1.0
				fluid_mouse_interactor_2d.set_brush_size(bs)
				print("[DemoController] Brush size +: ", bs)
		KEY_5:
			if fluid_mouse_interactor_2d:
				var bs: float = maxf(1.0, fluid_mouse_interactor_2d.get_brush_size() - 1.0)
				fluid_mouse_interactor_2d.set_brush_size(bs)
				print("[DemoController] Brush size -: ", bs)

		KEY_SPACE:
			print("[DemoController] Spawn burst 16 (SPACE)")
			_spawn_burst(16)

func _spawn_burst(count: int) -> void:
	if not gpu_stable_fluids_2d:
		printerr("[DemoController] ERROR: gpu_stable_fluids_2d is NULL in _spawn_burst!")
		return
	print("[DemoController] _spawn_burst count=", count)
	for _i in range(count):
		var angle := randf() * TAU
		var dist := randf() * 300.0
		var pos := Vector2(cos(angle) * dist, sin(angle) * dist)
		var vel := pos.normalized() * randf() * 5.0
		var col := Color(randf(), randf() * 0.5, 1.0 - randf(), 0.8)
		gpu_stable_fluids_2d.queue_draw_request(pos, col, vel, randf() * 2.0 + 1.0, randf() * 3.0 + 1.0)
	print("[DemoController] _spawn_burst DONE, count=", count)
