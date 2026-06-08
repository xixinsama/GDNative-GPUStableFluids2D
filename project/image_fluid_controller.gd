extends Node2D
## Image-as-fluid test — loads an image into the fluid simulation,
## lets the mouse disturb it, and displays a perf HUD.

@onready var sim: GPUStableFluids2D = $GPUStableFluids2D
@onready var mouse: FluidMouseInteractor2D = $GPUStableFluids2D/FluidMouseInteractor2D
@onready var help_panel: Control = $UI/HelpPanel
@onready var display: FluidDisplay2D = $FluidDisplay2D

const IMAGE_PATH := "res://icon.svg"
const SPLAT_GRID  := 60          # N×N sample grid (3600 points < 4096 limit)
const CELL_RADIUS := 5.0         # Splat radius per pixel

func _ready() -> void:
	# ── Simulation parameters ──
	sim.set_viscosity(0.0)
	sim.set_poisson_iterations(40)
	sim.set_vorticity_enabled(true)
	sim.set_vorticity_scale(0.2)
	sim.set_color_decay(0.0001)      # Very slow color fade to keep the image
	sim.set_velocity_decay(0.0)      # Minimal velocity damping

	# ── Splat the image into the fluid colour field ──
	_splat_image()

func _splat_image() -> void:
	var img := Image.new()
	var err := img.load(IMAGE_PATH)
	if err != OK:
		printerr("[ImageFluid] Failed to load image: ", IMAGE_PATH)
		return

	img.convert(Image.FORMAT_RGBA8)

	var img_w := img.get_width()
	var img_h := img.get_height()
	var world  := sim.get_fluid_world_size()
	var grid   := SPLAT_GRID

	var count := 0
	for y in grid:
		for x in grid:
			var px := int(float(x) / grid * img_w)
			var py := int(float(y) / grid * img_h)
			var c := img.get_pixel(px, py)
			if c.a < 0.01:
				continue

			# Convert grid [0,grid)→UV [0,1]→world position
			var u := (float(x) + 0.5) / grid
			var v := (float(y) + 0.5) / grid
			var wx := (u - 0.5) * world.x
			var wy := (v - 0.5) * world.y

			sim.queue_draw_request(
				Vector2(wx, wy),
				c,
				Vector2.ZERO,         # no initial velocity
				CELL_RADIUS,          # colour radius
				0.0                    # velocity radius (colour-only splat)
			)
			count += 1

	print("[ImageFluid] Splatted ", count, " pixels from ", IMAGE_PATH,
		" (", img_w, "×", img_h, " → ", grid, "×", grid, " grid)")

# ── Keyboard shortcuts ────────────────────────────────────

func _input(ev: InputEvent) -> void:
	if not (ev is InputEventKey and ev.pressed):
		return

	match ev.keycode:
		KEY_H:
			help_panel.visible = !help_panel.visible
		KEY_R:
			sim.reset()
			_splat_image()
		KEY_1:
			if mouse:
				mouse.set_interaction_mode(0)
				print("[ImageFluid] Mode: Draw")
		KEY_2:
			if mouse:
				mouse.set_interaction_mode(1)
				print("[ImageFluid] Mode: Push")
		KEY_3:
			if mouse:
				mouse.set_interaction_mode(2)
				print("[ImageFluid] Mode: Pull")
		KEY_4:
			if mouse:
				mouse.set_interaction_mode(3)
				print("[ImageFluid] Mode: Swirl")
		KEY_BRACKETLEFT:
			if mouse:
				var bs := maxf(1.0, mouse.get_brush_size() - 1.0)
				mouse.set_brush_size(bs)
				print("[ImageFluid] Brush size: ", bs)
		KEY_BRACKETRIGHT:
			if mouse:
				var bs := mouse.get_brush_size() + 1.0
				mouse.set_brush_size(bs)
				print("[ImageFluid] Brush size: ", bs)
