#include "gpu_resource_manager.h"
#include "fluid_debug_config.h"
#include "utils/draw_request.h"

#include "godot_cpp/classes/rd_shader_file.hpp"
#include "godot_cpp/classes/rd_shader_spirv.hpp"
#include "godot_cpp/classes/resource_loader.hpp"
#include "godot_cpp/core/error_macros.hpp"

namespace godot {

// ──────────────────────────────────────────────────────────
//  Lifecycle
// ──────────────────────────────────────────────────────────

void GPUResourceManager::initialize(RenderingDevice *p_device, int p_width, int p_height,
                                    bool p_subtractive_mixing, const Color &p_clear_color,
                                    int p_max_batch_points, int p_max_force_emitters) {
	device = p_device;
	width  = p_width;
	height = p_height;

	if (!device) {
		FLUID_PRINTERR("[GPUResourceManager] ERROR: Null RenderingDevice passed to initialize()");
		return;
	}

	// --- Sampler ---
	Ref<RDSamplerState> sampler_state;
	sampler_state.instantiate();
	sampler_state->set_min_filter(RenderingDevice::SAMPLER_FILTER_LINEAR);
	sampler_state->set_mag_filter(RenderingDevice::SAMPLER_FILTER_LINEAR);
	sampler_state->set_repeat_u(RenderingDevice::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE);
	sampler_state->set_repeat_v(RenderingDevice::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE);
	sampler = device->sampler_create(sampler_state);

	// --- Texture format ---
	// CAN_COPY_FROM_BIT is REQUIRED for Forward+ renderer to read RD textures
	// for display (e.g., Texture2DRD used in Sprite2D). Without it, Vulkan
	// rejects the copy-source operation and Forward+ crashes.
	constexpr uint32_t usage = RenderingDevice::TEXTURE_USAGE_STORAGE_BIT |
	                           RenderingDevice::TEXTURE_USAGE_SAMPLING_BIT |
	                           RenderingDevice::TEXTURE_USAGE_CAN_COPY_TO_BIT |
	                           RenderingDevice::TEXTURE_USAGE_CAN_COPY_FROM_BIT |
	                           RenderingDevice::TEXTURE_USAGE_CAN_UPDATE_BIT;

	// Display-only texture usage (no STORAGE_BIT → SHADER_READ_ONLY_OPTIMAL layout
	// after any operation, safe for Forward+ renderer to sample)
	constexpr uint32_t usage_display = RenderingDevice::TEXTURE_USAGE_SAMPLING_BIT |
	                                   RenderingDevice::TEXTURE_USAGE_CAN_COPY_TO_BIT;

	const RenderingDevice::DataFormat fmt = RenderingDevice::DATA_FORMAT_R32G32B32A32_SFLOAT;

	// --- Simulation textures ---
	tex_velocity   = create_texture(width, height, fmt, usage);
	tex_pressure   = create_texture(width, height, fmt, usage);
	tex_color      = create_texture(width, height, fmt, usage);
	tex_divergence = create_texture(width, height, fmt, usage);
	tex_temp       = create_texture(width, height, fmt, usage);
	tex_obstacle      = create_texture(width, height, fmt, usage);
	tex_obstacle_pre  = create_texture(width, height, fmt, usage);
	// Display texture — no STORAGE_BIT so its Vulkan layout is always
	// SHADER_READ_ONLY_OPTIMAL, which Forward+ can safely sample from.
	tex_display      = create_texture(width, height, fmt, usage_display);

	// NOTE: texture_clear is NOT called here because RenderingDevice
	// command submission MUST happen on the render thread. Callers must
	// defer clear_textures() via RenderingServer::call_on_render_thread.

	// --- Compute pipelines ---
	FLUID_PRINT("[GPUResourceManager] Loading shader pipelines...");
	advect_pipeline         = create_compute_pipeline("res://shaders/advect.glsl");
	FLUID_PRINT("[GPUResourceManager] advect: valid=", advect_pipeline.is_valid());
	jacobi_pipeline         = create_compute_pipeline("res://shaders/jacobi.glsl");
	FLUID_PRINT("[GPUResourceManager] jacobi: valid=", jacobi_pipeline.is_valid());
	divergence_pipeline     = create_compute_pipeline("res://shaders/divergence.glsl");
	FLUID_PRINT("[GPUResourceManager] divergence: valid=", divergence_pipeline.is_valid());
	subtract_pipeline       = create_compute_pipeline("res://shaders/subtract.glsl");
	FLUID_PRINT("[GPUResourceManager] subtract: valid=", subtract_pipeline.is_valid());
	boundary_pipeline       = create_compute_pipeline("res://shaders/boundary.glsl");
	FLUID_PRINT("[GPUResourceManager] boundary: valid=", boundary_pipeline.is_valid());
	vorticity_pipeline      = create_compute_pipeline("res://shaders/vorticity.glsl");
	FLUID_PRINT("[GPUResourceManager] vorticity: valid=", vorticity_pipeline.is_valid());
	shift_texture_pipeline  = create_compute_pipeline("res://shaders/shift_texture.glsl");
	FLUID_PRINT("[GPUResourceManager] shift_texture: valid=", shift_texture_pipeline.is_valid());
	splat_batch_pipeline    = create_compute_pipeline("res://shaders/splat_batch.glsl");
	FLUID_PRINT("[GPUResourceManager] splat_batch: valid=", splat_batch_pipeline.is_valid());
	obstacle_force_pipeline = create_compute_pipeline("res://shaders/obstacle_force.glsl");
	FLUID_PRINT("[GPUResourceManager] obstacle_force: valid=", obstacle_force_pipeline.is_valid());
	copy_texture_pipeline   = create_compute_pipeline("res://shaders/copy_texture.glsl");
	FLUID_PRINT("[GPUResourceManager] copy_texture: valid=", copy_texture_pipeline.is_valid());

	// --- Storage buffers ---
	{
		PackedByteArray empty;
		batch_buffer = device->storage_buffer_create(p_max_batch_points * sizeof(BatchPoint), empty); // 48 bytes / point
		force_emitter_buffer = device->storage_buffer_create(p_max_force_emitters * sizeof(float) * 12, empty); // 48 bytes / emitter
	}
}

void GPUResourceManager::terminate() {
	if (!device) return;

	clear_uniform_set_cache();

	safe_free_rid(sampler);

	safe_free_rid(tex_velocity);
	safe_free_rid(tex_pressure);
	safe_free_rid(tex_color);
	safe_free_rid(tex_divergence);
	safe_free_rid(tex_temp);
	safe_free_rid(tex_obstacle);
	safe_free_rid(tex_obstacle_pre);
	safe_free_rid(tex_display);

	safe_free_rid(batch_buffer);
	safe_free_rid(force_emitter_buffer);

	free_pipeline(advect_pipeline);
	free_pipeline(jacobi_pipeline);
	free_pipeline(divergence_pipeline);
	free_pipeline(subtract_pipeline);
	free_pipeline(boundary_pipeline);
	free_pipeline(vorticity_pipeline);
	free_pipeline(shift_texture_pipeline);
	free_pipeline(splat_batch_pipeline);
	free_pipeline(obstacle_force_pipeline);
	free_pipeline(copy_texture_pipeline);

	device = nullptr;
}

void GPUResourceManager::clear_textures(const Color &p_clear_color) {
	if (!device) return;
	Color black(0, 0, 0, 1);
	device->texture_clear(tex_velocity,   black, 0, 1, 0, 1);
	device->texture_clear(tex_pressure,   black, 0, 1, 0, 1);
	device->texture_clear(tex_color,      p_clear_color, 0, 1, 0, 1);
	device->texture_clear(tex_divergence, black, 0, 1, 0, 1);
	device->texture_clear(tex_temp,       black, 0, 1, 0, 1);
	device->texture_clear(tex_display,   p_clear_color, 0, 1, 0, 1);
}

// ──────────────────────────────────────────────────────────
//  Texture helpers
// ──────────────────────────────────────────────────────────

RID GPUResourceManager::create_texture(uint32_t p_width, uint32_t p_height,
                                        RenderingDevice::DataFormat p_format,
                                        uint32_t p_usage_bits) {
	Ref<RDTextureFormat> fmt;
	fmt.instantiate();
	fmt->set_width(p_width);
	fmt->set_height(p_height);
	fmt->set_format(p_format);
	fmt->set_usage_bits(p_usage_bits);

	Ref<RDTextureView> view;
	view.instantiate();

	TypedArray<PackedByteArray> empty_data;
	return device->texture_create(fmt, view, empty_data);
}

void GPUResourceManager::safe_free_rid(RID &r_rid) {
	if (r_rid.is_valid()) {
		device->free_rid(r_rid);
		r_rid = RID();
	}
}

// ──────────────────────────────────────────────────────────
//  Pipeline helpers
// ──────────────────────────────────────────────────────────

ComputePipeline GPUResourceManager::create_compute_pipeline(const String &p_shader_path) {
	ComputePipeline result;

	Ref<RDShaderFile> shader_file = ResourceLoader::get_singleton()->load(p_shader_path, "RDShaderFile");
	ERR_FAIL_COND_V_MSG(shader_file.is_null(), result,
		vformat("GPUResourceManager: Failed to load shader '%s'", p_shader_path));

	Ref<RDShaderSPIRV> spirv = shader_file->get_spirv();
	ERR_FAIL_COND_V_MSG(spirv.is_null(), result,
		vformat("GPUResourceManager: No SPIR-V in '%s'", p_shader_path));

	result.shader_id = device->shader_create_from_spirv(spirv);
	ERR_FAIL_COND_V_MSG(!result.shader_id.is_valid(), result,
		vformat("GPUResourceManager: Failed to create shader from '%s'", p_shader_path));

	result.pipeline_id = device->compute_pipeline_create(result.shader_id);
	result.name = p_shader_path.get_file().get_basename();

	return result;
}

void GPUResourceManager::free_pipeline(ComputePipeline &r_pipeline) {
	if (r_pipeline.pipeline_id.is_valid()) {
		device->free_rid(r_pipeline.pipeline_id);
		r_pipeline.pipeline_id = RID();
	}
	if (r_pipeline.shader_id.is_valid()) {
		device->free_rid(r_pipeline.shader_id);
		r_pipeline.shader_id = RID();
	}
}

// ──────────────────────────────────────────────────────────
//  Uniform set helpers
// ──────────────────────────────────────────────────────────

RID GPUResourceManager::create_uniform_set_image(const ComputePipeline &p_pipeline, RID p_texture, uint32_t p_set_index) {
	Ref<RDUniform> uniform;
	uniform.instantiate();
	uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_IMAGE);
	uniform->set_binding(0);
	uniform->add_id(p_texture);

	return device->uniform_set_create(Array::make(uniform), p_pipeline.shader_id, p_set_index);
}

RID GPUResourceManager::create_uniform_set_sampler(const ComputePipeline &p_pipeline, RID p_texture, uint32_t p_set_index) {
	Ref<RDUniform> uniform;
	uniform.instantiate();
	uniform->set_uniform_type(RenderingDevice::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE);
	uniform->set_binding(0);
	uniform->add_id(sampler);
	uniform->add_id(p_texture);

	return device->uniform_set_create(Array::make(uniform), p_pipeline.shader_id, p_set_index);
}

RID GPUResourceManager::get_or_create_cached_uniform_set(RID p_shader, RID p_texture,
                                                          uint32_t p_set_index, int p_uniform_type) {
	UniformSetKey key{p_shader.get_id(), p_texture.get_id(), p_set_index, p_uniform_type};
	auto it = _uniform_set_cache.find(key);
	if (it != _uniform_set_cache.end()) {
		return it->second;
	}

	Ref<RDUniform> uniform;
	uniform.instantiate();
	uniform->set_uniform_type((RenderingDevice::UniformType)p_uniform_type);
	uniform->set_binding(0);

	if (p_uniform_type == RenderingDevice::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE) {
		uniform->add_id(sampler);
		uniform->add_id(p_texture);
	} else {
		uniform->add_id(p_texture);
	}

	RID us = device->uniform_set_create(Array::make(uniform), p_shader, p_set_index);
	_uniform_set_cache[key] = us;
	return us;
}

void GPUResourceManager::clear_uniform_set_cache() {
	for (auto &kv : _uniform_set_cache) {
		if (kv.second.is_valid()) {
			device->free_rid(kv.second);
		}
	}
	_uniform_set_cache.clear();
}

} // namespace godot
