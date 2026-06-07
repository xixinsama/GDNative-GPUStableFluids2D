extends Control
var fps_hist: Array[float] = []
func _draw() -> void:
	if fps_hist.size() < 2: return
	var w := size.x; var h := size.y
	var step := w / float(fps_hist.size()); var mf := 144.0
	draw_line(Vector2(0, h-(60.0/mf)*h), Vector2(w, h-(60.0/mf)*h), Color(1,1,1,0.15))
	for i in range(1, fps_hist.size()):
		draw_line(Vector2((i-1)*step, h-(fps_hist[i-1]/mf)*h), Vector2(i*step, h-(fps_hist[i]/mf)*h), Color.GREEN, 1.5)
