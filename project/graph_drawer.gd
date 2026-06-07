extends Control
## Simple FPS history line graph

var _data: Array = []
var _max_fps := 144.0

func update_data(d: Array) -> void:
	_data = d
	queue_redraw()

func _draw() -> void:
	if _data.size() < 2:
		return
	var w := size.x
	var h := size.y
	if w <= 0 or h <= 0:
		return

	var step: float = w / float(_data.size())

	# 60 fps reference line
	var y60: float = h - (60.0 / _max_fps) * h
	draw_line(Vector2(0, y60), Vector2(w, y60), Color(1, 1, 1, 0.12))

	# 30 fps reference line
	var y30: float = h - (30.0 / _max_fps) * h
	draw_line(Vector2(0, y30), Vector2(w, y30), Color(1, 1, 1, 0.06))

	# FPS line
	for i in range(1, _data.size()):
		var x0: float = (i - 1) * step
		var x1: float = i * step
		var y0: float = h - (_data[i - 1] / _max_fps) * h
		var y1: float = h - (_data[i] / _max_fps) * h
		draw_line(Vector2(x0, y0), Vector2(x1, y1), Color.GREEN, 1.5)
