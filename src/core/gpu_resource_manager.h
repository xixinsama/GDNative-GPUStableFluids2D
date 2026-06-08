#pragma once

#include "godot_cpp/classes/rendering_device.hpp"
#include "godot_cpp/classes/rd_uniform.hpp"
#include "godot_cpp/classes/rd_texture_format.hpp"
#include "godot_cpp/classes/rd_texture_view.hpp"
#include "godot_cpp/classes/rd_sampler_state.hpp"
#include "godot_cpp/variant/rid.hpp"
#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/variant/color.hpp"

#include <unordered_map>
#include <tuple>

namespace godot {

// Hash-compatible key type for uniform set cache
struct UniformSetKey {
	int64_t  shader_id;
	int64_t  texture_id;
	uint32_t set_index;
	int32_t  uniform_type;

	bool operator==(const UniformSetKey &o) const {
		return shader_id == o.shader_id && texture_id == o.texture_id &&
		       set_index == o.set_index && uniform_type == o.uniform_type;
	}
};

} // namespace godot

namespace std {
	template<>
	struct hash<godot::UniformSetKey> {
		size_t operator()(const godot::UniformSetKey &k) const {
			return (size_t)(k.shader_id ^ k.texture_id ^
			       ((uint64_t)k.set_index << 32) ^ (uint64_t)k.uniform_type);
		}
	};
}

namespace godot {

// Simple container for a compute pipeline pair
struct ComputePipeline {
	String name;
	RID    shader_id;
	RID    pipeline_id;

	bool is_valid() const { return shader_id.is_valid() && pipeline_id.is_valid(); }
};

// GPU resource lifecycle manager
// Owns all textures, pipelines, buffers, and the sampler.
// FluidRenderPipeline reads from this, GPUStableFluids2D owns it.
class GPUResourceManager {
public:
	GPUResourceManager() = default;
	~GPUResourceManager() { terminate(); }

	// Lifecycle — call on render thread
	void initialize(RenderingDevice *p_device, int p_width, int p_height,
	                bool p_subtractive_mixing, const Color &p_clear_color,
	                int p_max_batch_points, int p_max_force_emitters);
	void terminate();
	void clear_textures(const Color &p_clear_color);

	// Texture helpers
	RID  create_texture(uint32_t p_width, uint32_t p_height,
	                    RenderingDevice::DataFormat p_format,
	                    uint32_t p_usage_bits);
	void safe_free_rid(RID &r_rid);

	// Pipeline helpers
	ComputePipeline create_compute_pipeline(const String &p_shader_path);
	void free_pipeline(ComputePipeline &r_pipeline);

	// Uniform set creation with caching
	RID create_uniform_set_image(const ComputePipeline &p_pipeline, RID p_texture, uint32_t p_set_index);
	RID create_uniform_set_sampler(const ComputePipeline &p_pipeline, RID p_texture, uint32_t p_set_index);
	RID get_or_create_cached_uniform_set(RID p_shader, RID p_texture,
	                                     uint32_t p_set_index, int p_uniform_type);
	void clear_uniform_set_cache();

	// --- Public GPU resources ---
	RenderingDevice *device = nullptr;
	RID sampler;

	// Simulation textures (rgba32f)
	RID tex_velocity;
	RID tex_pressure;
	RID tex_color;
	RID tex_divergence;
	RID tex_temp;       // Ping-pong swap buffer
	RID tex_display;    // Display-only (no STORAGE_BIT → SHADER_READ_ONLY_OPTIMAL layout)

	// Obstacle textures
	RID tex_obstacle;
	RID tex_obstacle_pre;

	// Storage buffers
	RID batch_buffer;
	RID force_emitter_buffer;

	// Compute pipelines
	ComputePipeline advect_pipeline;
	ComputePipeline jacobi_pipeline;
	ComputePipeline divergence_pipeline;
	ComputePipeline subtract_pipeline;
	ComputePipeline boundary_pipeline;
	ComputePipeline vorticity_pipeline;
	ComputePipeline shift_texture_pipeline;
	ComputePipeline splat_batch_pipeline;
	ComputePipeline obstacle_force_pipeline;
	ComputePipeline copy_texture_pipeline;

	// Metadata
	int width  = 512;
	int height = 512;

private:
	std::unordered_map<UniformSetKey, RID> _uniform_set_cache;
};

} // namespace godot
