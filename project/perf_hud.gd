extends Control
## Real-time performance overlay — FPS, frame time, simulation stats, graph

@onready var fps_label = $Panel/VBox/FPSLabel
@onready var ft_label = $Panel/VBox/FTLabel
@onready var sim_label = $Panel/VBox/SimLabel
@onready var draw_label = $Panel/VBox/DrawLabel
@onready var graph = $Panel/VBox/Graph

var _fps_hist: Array[float] = []
var _max_samp := 180

func _ready() -> void: graph.draw.connect(_draw_graph)

func _process(dt: float) -> void:
	var fps := Engine.get_frames_per_second()
	_fps_hist.append(fps)
	if _fps_hist.size() > _max_samp: _fps_hist.pop_front()

	fps_label.text = "FPS: %d" % int(fps)
	var c := Color.GREEN if fps >= 55 else (Color.ORANGE if fps >= 30 else Color.RED)
	fps_label.modulate = c
	ft_label.text = "FT: %.2f ms" % (dt * 1000.0)

	var sim := _find_sim()
	if sim:
		sim_label.text = "Grid: %dx%d | Jacobi: %d | Vort: %s" % [
			sim.get_width(), sim.get_height(),
			sim.get_poisson_iterations(),
			"ON" if sim.is_vorticity_enabled() else "OFF"]

	draw_label.text = "DrawCalls: %d | VRAM: %dMB" % [
		Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME),
		int(Performance.get_monitor(Performance.RENDER_VIDEO_MEM_USED) / (1024*1024))]

	graph.queue_redraw()

func _find_sim():
	var t := get_tree(); return t.get_first_node_in_group("fluid_sim_nodes") if t else null

func _draw_graph() -> void:
	if _fps_hist.size() < 2: return
	var r := graph.get_rect(); var w := r.size.x; var h := r.size.y
	var step := w / _fps_hist.size(); var mf := 144.0
	var pts := PackedVector2Array()
	for i in _fps_hist.size():
		pts.append(Vector2(i * step, h - (_fps_hist[i] / mf) * h))
	graph.draw_polyline(pts, Color.GREEN, 1.5)
	graph.draw_line(Vector2(0, h - (60.0/mf)*h), Vector2(w, h - (60.0/mf)*h), Color(1,1,1,0.2))
