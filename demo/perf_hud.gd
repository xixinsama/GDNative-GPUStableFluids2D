extends Control
## Performance HUD — real-time stats overlay

@onready var fps_label: Label = $Panel/VBox/FPSLabel
@onready var ft_label: Label = $Panel/VBox/FTLabel
@onready var grid_label: Label = $Panel/VBox/GridLabel
@onready var draw_label: Label = $Panel/VBox/DrawLabel
@onready var graph: Control = $Panel/VBox/Graph

var _fps_hist: Array = []
var _log_frame_count: int = 0
var _last_log_fps: float = 0.0

func _ready() -> void:
	print("[PerfHUD] _ready() START")
	print("[PerfHUD] fps_label=", fps_label, " ft_label=", ft_label)
	print("[PerfHUD] grid_label=", grid_label, " draw_label=", draw_label)
	print("[PerfHUD] graph=", graph)
	print("[PerfHUD] _ready() DONE")

func _process(dt: float) -> void:
	_log_frame_count += 1

	# Throttled logging — every 60 frames (~1s)
	var should_log := (_log_frame_count % 60 == 0)

	var fps := Engine.get_frames_per_second()
	_fps_hist.append(fps)
	if _fps_hist.size() > 180:
		_fps_hist.pop_front()

	# FPS label with color coding
	if fps_label:
		fps_label.text = "FPS: %d" % int(fps)
		fps_label.modulate = Color.GREEN if fps >= 55 else (Color.ORANGE if fps >= 30 else Color.RED)
	else:
		if should_log: printerr("[PerfHUD] ERROR: fps_label is NULL!")

	# Frame time
	if ft_label:
		ft_label.text = "FT: %.2f ms" % (dt * 1000.0)

	var t := get_tree()
	var sim = t.get_first_node_in_group("fluid_sim_nodes") if t else null
	if sim and sim.has_method("get_width"):
		if grid_label:
			grid_label.text = "Grid: %dx%d | Jacobi: %d | Vort: %s" % [
				sim.get_width(), sim.get_height(),
				sim.get_poisson_iterations(),
				"ON" if sim.is_vorticity_enabled() else "OFF"]
		if should_log:
			print("[PerfHUD] frame=%d fps=%d sim_grid=%dx%d" % [_log_frame_count, int(fps), sim.get_width(), sim.get_height()])
	else:
		if grid_label:
			grid_label.text = "Grid: -- (no sim)"
		if should_log:
			printerr("[PerfHUD] WARN: No sim found in group 'fluid_sim_nodes' at frame ", _log_frame_count)

	if draw_label:
		draw_label.text = "Draw: %d | VRAM: %dMB" % [
			Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME),
			int(Performance.get_monitor(Performance.RENDER_VIDEO_MEM_USED) / 1048576)]

	# Update graph
	if graph and graph.has_method("update_data"):
		graph.update_data(_fps_hist)

	if should_log:
		_last_log_fps = fps
