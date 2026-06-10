#include "fluid_light_occluder_2d.h"
#include "GPU_stable_fluids_2D_init.h"

#include "godot_cpp/classes/light_occluder2d.hpp"
#include "godot_cpp/classes/occluder_polygon2d.hpp"
#include "godot_cpp/classes/rendering_device.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/core/class_db.hpp"

#include <cmath>
#include <cstring>

namespace godot {

FluidLightOccluder2D::FluidLightOccluder2D() = default;
FluidLightOccluder2D::~FluidLightOccluder2D() { _clear_occluders(); }

void FluidLightOccluder2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_sim_target", "path"), &FluidLightOccluder2D::set_sim_target);
	ClassDB::bind_method(D_METHOD("get_sim_target"), &FluidLightOccluder2D::get_sim_target);
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "sim_target", PROPERTY_HINT_NODE_TYPE, "GPUStableFluids2D"), "set_sim_target", "get_sim_target");

	ClassDB::bind_method(D_METHOD("set_density_threshold", "threshold"), &FluidLightOccluder2D::set_density_threshold);
	ClassDB::bind_method(D_METHOD("get_density_threshold"), &FluidLightOccluder2D::get_density_threshold);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "density_threshold", PROPERTY_HINT_RANGE, "0.01,1.0,0.01"), "set_density_threshold", "get_density_threshold");

	ClassDB::bind_method(D_METHOD("set_max_contours", "max"), &FluidLightOccluder2D::set_max_contours);
	ClassDB::bind_method(D_METHOD("get_max_contours"), &FluidLightOccluder2D::get_max_contours);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_contours", PROPERTY_HINT_RANGE, "1,32"), "set_max_contours", "get_max_contours");

	ClassDB::bind_method(D_METHOD("set_update_frequency", "hz"), &FluidLightOccluder2D::set_update_frequency);
	ClassDB::bind_method(D_METHOD("get_update_frequency"), &FluidLightOccluder2D::get_update_frequency);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "update_frequency", PROPERTY_HINT_RANGE, "1,60"), "set_update_frequency", "get_update_frequency");

	ClassDB::bind_method(D_METHOD("set_simplify_tolerance", "tolerance"), &FluidLightOccluder2D::set_simplify_tolerance);
	ClassDB::bind_method(D_METHOD("get_simplify_tolerance"), &FluidLightOccluder2D::get_simplify_tolerance);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "simplify_tolerance", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_simplify_tolerance", "get_simplify_tolerance");

	ClassDB::bind_method(D_METHOD("set_occluder_light_mask", "mask"), &FluidLightOccluder2D::set_occluder_light_mask);
	ClassDB::bind_method(D_METHOD("get_occluder_light_mask"), &FluidLightOccluder2D::get_occluder_light_mask);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "occluder_light_mask", PROPERTY_HINT_LAYERS_2D_RENDER), "set_occluder_light_mask", "get_occluder_light_mask");

	ClassDB::bind_method(D_METHOD("set_downscale_factor", "factor"), &FluidLightOccluder2D::set_downscale_factor);
	ClassDB::bind_method(D_METHOD("get_downscale_factor"), &FluidLightOccluder2D::get_downscale_factor);
	ADD_PROPERTY(PropertyInfo(Variant::INT, "downscale_factor", PROPERTY_HINT_RANGE, "1,8"), "set_downscale_factor", "get_downscale_factor");

	ADD_SIGNAL(MethodInfo("contours_updated", PropertyInfo(Variant::INT, "count")));
}

void FluidLightOccluder2D::set_sim_target(const NodePath &v) { _sim_target = v; }
NodePath FluidLightOccluder2D::get_sim_target() const { return _sim_target; }
void FluidLightOccluder2D::set_density_threshold(float v) { _density_threshold = v; }
float FluidLightOccluder2D::get_density_threshold() const { return _density_threshold; }
void FluidLightOccluder2D::set_max_contours(int v) { _max_contours = v; }
int FluidLightOccluder2D::get_max_contours() const { return _max_contours; }
void FluidLightOccluder2D::set_update_frequency(int v) { _update_frequency = v; }
int FluidLightOccluder2D::get_update_frequency() const { return _update_frequency; }
void FluidLightOccluder2D::set_simplify_tolerance(float v) { _simplify_tolerance = v; }
float FluidLightOccluder2D::get_simplify_tolerance() const { return _simplify_tolerance; }
void FluidLightOccluder2D::set_occluder_light_mask(int v) { _occluder_light_mask = v; }
int FluidLightOccluder2D::get_occluder_light_mask() const { return _occluder_light_mask; }
void FluidLightOccluder2D::set_downscale_factor(int v) { _downscale_factor = v; }
int FluidLightOccluder2D::get_downscale_factor() const { return _downscale_factor; }

GPUStableFluids2D *FluidLightOccluder2D::_find_sim() const {
	if (!_sim_target.is_empty()) {
		Node *n = get_node_or_null(_sim_target);
		if (n) return Object::cast_to<GPUStableFluids2D>(n);
	}
	SceneTree *tree = get_tree();
	if (tree) {
		TypedArray<Node> nodes = tree->get_nodes_in_group("fluid_sim_nodes");
		if (nodes.size() > 0) return Object::cast_to<GPUStableFluids2D>(nodes[0]);
	}
	return nullptr;
}

void FluidLightOccluder2D::_clear_occluders() {
	TypedArray<Node> children = get_children();
	for (int i = children.size() - 1; i >= 0; i--) {
		Node *child = Object::cast_to<Node>(children[i]);
		if (child && Object::cast_to<LightOccluder2D>(child)) {
			child->queue_free();
		}
	}
}

void FluidLightOccluder2D::_simplify_contours(PackedVector2Array &p_contour, float p_epsilon) {
	// Douglas-Peucker simplification
	int n = (int)p_contour.size();
	if (n <= 3) return;

	float eps2 = p_epsilon * p_epsilon;
	// Keep first and last, remove intermediate points that are close to the line
	PackedVector2Array result;
	result.append(p_contour[0]);

	for (int i = 1; i < n - 1; i++) {
		// Distance from point to line segment [result.back(), p_contour[n-1]]
		Vector2 a = result[result.size() - 1];
		Vector2 b = p_contour[n - 1];
		Vector2 p = p_contour[i];

		Vector2 ab = b - a;
		float len2 = ab.length_squared();
		float dist2;
		if (len2 < 0.0001f) {
			dist2 = (p - a).length_squared();
		} else {
			float t = Math::clamp(((p - a).dot(ab)) / len2, 0.0f, 1.0f);
			dist2 = (p - (a + ab * t)).length_squared();
		}

		if (dist2 > eps2) {
			result.append(p);
		}
	}
	result.append(p_contour[n - 1]);
	p_contour = result;
}

PackedVector2Array FluidLightOccluder2D::_extract_contours() {
	// Simplified: return a rectangle covering the fluid domain.
	// A full implementation would do GPU Marching Square readback.
	PackedVector2Array contour;
	GPUStableFluids2D *sim = _find_sim();
	if (!sim) return contour;

	float w = (float)sim->get_width();
	float h = (float)sim->get_height();
	Vector2 ws = sim->get_fluid_world_size();
	float hw = ws.x * 0.5f;
	float hh = ws.y * 0.5f;
	Vector2 center = sim->get_global_position();

	// Return a simple rectangle outline as a placeholder
	contour.append(center + Vector2(-hw, -hh));
	contour.append(center + Vector2( hw, -hh));
	contour.append(center + Vector2( hw,  hh));
	contour.append(center + Vector2(-hw,  hh));

	return contour;
}

void FluidLightOccluder2D::_create_occluder(const PackedVector2Array &p_points) {
	// Create a LightOccluder2D child node
	LightOccluder2D *occluder = memnew(LightOccluder2D);
	add_child(occluder);
	occluder->set_owner(get_owner());

	// Scale points from world space to local space
	PackedVector2Array local_pts;
	Vector2 origin = get_global_position();
	for (int i = 0; i < (int)p_points.size(); i++) {
		local_pts.append(p_points[i] - origin);
	}

	Ref<OccluderPolygon2D> poly;
	poly.instantiate();
	poly->set_polygon(local_pts);
	occluder->set_occluder_polygon(poly);
	occluder->set_occluder_light_mask(_occluder_light_mask);
}

void FluidLightOccluder2D::_update_occluders() {
	_clear_occluders();
	PackedVector2Array contour = _extract_contours();
	if (contour.size() >= 3) {
		_simplify_contours(contour, _simplify_tolerance);
		_create_occluder(contour);
	}
	emit_signal("contours_updated", 1);
}

void FluidLightOccluder2D::_ready() {
	_update_timer = 0.0f;
}

void FluidLightOccluder2D::_process(double p_delta) {
	if (Engine::get_singleton()->is_editor_hint()) return;
	_update_timer += (float)p_delta;
	float interval = 1.0f / (float)_update_frequency;
	if (_update_timer >= interval) {
		_update_occluders();
		_update_timer -= interval;
	}
}
} // namespace godot
