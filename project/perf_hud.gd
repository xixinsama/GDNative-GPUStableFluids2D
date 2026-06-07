extends Control
## Performance HUD — real-time stats overlay
## Attached to a Control node; expects children: Panel/VBox/[FPSLabel,FTLabel,GridLabel,DrawLabel]

var _fps_hist: Array = []
var _labels: Dictionary = {}
var _graph_ref = null

func _ready() -> void:
	var panel := get_node_or_null("Panel")
	if not panel: return
	var vbox := panel.get_node_or_null("VBox")
	if not vbox: return

	_labels["fps"] = vbox.get_node_or_null("FPSLabel")
	_labels["ft"] = vbox.get_node_or_null("FTLabel")
	_labels["grid"] = vbox.get_node_or_null("GridLabel")
	_labels["draw"] = vbox.get_node_or_null("DrawLabel")
	_graph_ref = vbox.get_node_or_null("Graph")

func _process(dt: float) -> void:
	var fps := Engine.get_frames_per_second()
	_fps_hist.append(fps)
	if _fps_hist.size() > 180:
		_fps_hist.pop_front()

	# FPS label with color coding
	var fps_lbl := _labels.get("fps")
	if fps_lbl:
		fps_lbl.text = "FPS: %d" % int(fps)
		fps_lbl.modulate = Color.GREEN if fps >= 55 else (Color.ORANGE if fps >= 30 else Color.RED)

	# Frame time
	var ft_lbl := _labels.get("ft")
	if ft_lbl:
		ft_lbl.text = "FT: %.2f ms" % (dt * 1000.0)

	# Simulation stats
	var grid_lbl := _labels.get("grid")
	if grid_lbl:
		var t := get_tree()
		var sim = t.get_first_node_in_group("fluid_sim_nodes") if t else null
		if sim and sim.has_method("get_width"):
			grid_lbl.text = "Grid: %dx%d | Jacobi: %d | Vort: %s" % [
				sim.get_width(), sim.get_height(),
				sim.get_poisson_iterations(),
				"ON" if sim.is_vorticity_enabled() else "OFF"]
		else:
			grid_lbl.text = "Grid: -- (no sim)"

	# Draw stats
	var draw_lbl := _labels.get("draw")
	if draw_lbl:
		draw_lbl.text = "Draw: %d | VRAM: %dMB" % [
			Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME),
			int(Performance.get_monitor(Performance.RENDER_VIDEO_MEM_USED) / 1048576)]

	# Update graph
	if _graph_ref and _graph_ref.has_method("update_data"):
		_graph_ref.update_data(_fps_hist)
