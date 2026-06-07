extends Control
@onready var fps_label = $Panel/VBox/FPSLabel
@onready var ft_label = $Panel/VBox/FTLabel
@onready var sim_label = $Panel/VBox/SimLabel
@onready var draw_label = $Panel/VBox/DrawLabel
@onready var graph = $Panel/VBox/Graph
var fps_hist: Array[float] = []

func _process(dt: float) -> void:
	var fps := Engine.get_frames_per_second()
	fps_hist.append(fps)
	if fps_hist.size() > 180: fps_hist.pop_front()
	fps_label.text = "FPS: %d" % int(fps)
	fps_label.modulate = Color.GREEN if fps >= 55 else (Color.ORANGE if fps >= 30 else Color.RED)
	ft_label.text = "FT: %.2f ms" % (dt * 1000.0)
	var sim := get_tree().get_first_node_in_group("fluid_sim_nodes") if get_tree() else null
	if sim:
		sim_label.text = "Grid: %dx%d | Jacobi: %d | Vort: %s" % [sim.get_width(), sim.get_height(), sim.get_poisson_iterations(), "ON" if sim.is_vorticity_enabled() else "OFF"]
	draw_label.text = "Draw: %d | VRAM: %dMB" % [Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME), int(Performance.get_monitor(Performance.RENDER_VIDEO_MEM_USED)/(1024*1024))]
	if graph.has_method("set_fps_data"): graph.fps_hist = fps_hist; graph.queue_redraw()
