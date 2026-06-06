#include "GPU_stable_fluids_2D_init.h"

#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/rd_shader_file.hpp"
#include "godot_cpp/classes/rd_texture_format.hpp"
#include "godot_cpp/classes/rd_texture_view.hpp"
#include "godot_cpp/classes/rd_uniform.hpp"
#include "godot_cpp/classes/rendering_device.hpp"
#include "godot_cpp/classes/rendering_server.hpp"
#include "godot_cpp/classes/texture2drd.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/defs.hpp"
#include "godot_cpp/godot.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"
#include "godot_cpp/variant/typed_array.hpp"

#include <cstring>

namespace godot {

GPUStableFluids2D::GPUStableFluids2D() {}

GPUStableFluids2D::~GPUStableFluids2D() {
    if (rd) {
        if (velocity_tex.is_valid()) rd->free_rid(velocity_tex);
        if (density_tex.is_valid()) rd->free_rid(density_tex);
        if (pressure_tex.is_valid()) rd->free_rid(pressure_tex);
        if (divergence_tex.is_valid()) rd->free_rid(divergence_tex);
        if (vorticity_tex.is_valid()) rd->free_rid(vorticity_tex);
        if (velocity_offsets_tex.is_valid()) rd->free_rid(velocity_offsets_tex);
        if (pressure_offsets_tex.is_valid()) rd->free_rid(pressure_offsets_tex);
        if (impulse_buffer.is_valid()) rd->free_rid(impulse_buffer);
        if (shader.is_valid()) rd->free_rid(shader);
        if (pipeline.is_valid()) rd->free_rid(pipeline);
    }
}

void GPUStableFluids2D::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_resolution", "width", "height"), &GPUStableFluids2D::set_resolution);
    ClassDB::bind_method(D_METHOD("get_width"), &GPUStableFluids2D::get_width);
    ClassDB::bind_method(D_METHOD("get_height"), &GPUStableFluids2D::get_height);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "width", PROPERTY_HINT_RANGE, "16,1024"), "set_resolution", "get_width");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "height", PROPERTY_HINT_RANGE, "16,1024"), "set_resolution", "get_height");

    ClassDB::bind_method(D_METHOD("set_timestep", "timestep"), &GPUStableFluids2D::set_timestep);
    ClassDB::bind_method(D_METHOD("get_timestep"), &GPUStableFluids2D::get_timestep);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "timestep", PROPERTY_HINT_RANGE, "0.1,10.0"), "set_timestep", "get_timestep");

    ClassDB::bind_method(D_METHOD("set_grid_scale", "scale"), &GPUStableFluids2D::set_grid_scale);
    ClassDB::bind_method(D_METHOD("get_grid_scale"), &GPUStableFluids2D::get_grid_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "grid_scale", PROPERTY_HINT_RANGE, "0.1,100.0"), "set_grid_scale", "get_grid_scale");

    ClassDB::bind_method(D_METHOD("set_viscosity", "viscosity"), &GPUStableFluids2D::set_viscosity);
    ClassDB::bind_method(D_METHOD("get_viscosity"), &GPUStableFluids2D::get_viscosity);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "viscosity", PROPERTY_HINT_RANGE, "0,0.01,0.0001"), "set_viscosity", "get_viscosity");

    ClassDB::bind_method(D_METHOD("set_poisson_iterations", "iterations"), &GPUStableFluids2D::set_poisson_iterations);
    ClassDB::bind_method(D_METHOD("get_poisson_iterations"), &GPUStableFluids2D::get_poisson_iterations);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "poisson_iterations", PROPERTY_HINT_RANGE, "1,200"), "set_poisson_iterations", "get_poisson_iterations");

    ClassDB::bind_method(D_METHOD("set_ink_longevity", "longevity"), &GPUStableFluids2D::set_ink_longevity);
    ClassDB::bind_method(D_METHOD("get_ink_longevity"), &GPUStableFluids2D::get_ink_longevity);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ink_longevity", PROPERTY_HINT_RANGE, "0.9,1.0,0.001"), "set_ink_longevity", "get_ink_longevity");

    ClassDB::bind_method(D_METHOD("set_ink_color", "color"), &GPUStableFluids2D::set_ink_color);
    ClassDB::bind_method(D_METHOD("get_ink_color"), &GPUStableFluids2D::get_ink_color);
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "ink_color"), "set_ink_color", "get_ink_color");

    ClassDB::bind_method(D_METHOD("set_vorticity_enabled", "enabled"), &GPUStableFluids2D::set_vorticity_enabled);
    ClassDB::bind_method(D_METHOD("is_vorticity_enabled"), &GPUStableFluids2D::is_vorticity_enabled);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "vorticity_enabled"), "set_vorticity_enabled", "is_vorticity_enabled");

    ClassDB::bind_method(D_METHOD("set_vorticity_scale", "scale"), &GPUStableFluids2D::set_vorticity_scale);
    ClassDB::bind_method(D_METHOD("get_vorticity_scale"), &GPUStableFluids2D::get_vorticity_scale);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "vorticity_scale", PROPERTY_HINT_RANGE, "0,0.5,0.001"), "set_vorticity_scale", "get_vorticity_scale");

    ClassDB::bind_method(D_METHOD("add_impulse", "position", "strength", "radius", "add_ink"), &GPUStableFluids2D::add_impulse, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("reset"), &GPUStableFluids2D::reset);
    ClassDB::bind_method(D_METHOD("get_density_texture"), &GPUStableFluids2D::get_density_texture);
    ClassDB::bind_method(D_METHOD("get_velocity_texture"), &GPUStableFluids2D::get_velocity_texture);
}

// ===== Getter/Setter Implementations =====
void GPUStableFluids2D::set_resolution(int p_width, int p_height) {
    width = p_width;
    height = p_height;
    needs_recreate = true;
}
int GPUStableFluids2D::get_width() const { return width; }
int GPUStableFluids2D::get_height() const { return height; }

void GPUStableFluids2D::set_timestep(float p_timestep) { timestep = p_timestep; }
float GPUStableFluids2D::get_timestep() const { return timestep; }

void GPUStableFluids2D::set_grid_scale(float p_scale) { grid_scale = p_scale; }
float GPUStableFluids2D::get_grid_scale() const { return grid_scale; }

void GPUStableFluids2D::set_viscosity(float p_viscosity) { viscosity = p_viscosity; }
float GPUStableFluids2D::get_viscosity() const { return viscosity; }

void GPUStableFluids2D::set_poisson_iterations(int p_iterations) { poisson_iterations = p_iterations; }
int GPUStableFluids2D::get_poisson_iterations() const { return poisson_iterations; }

void GPUStableFluids2D::set_ink_longevity(float p_longevity) { ink_longevity = p_longevity; }
float GPUStableFluids2D::get_ink_longevity() const { return ink_longevity; }

void GPUStableFluids2D::set_ink_color(const Color &p_color) { ink_color = p_color; }
Color GPUStableFluids2D::get_ink_color() const { return ink_color; }

void GPUStableFluids2D::set_vorticity_enabled(bool p_enabled) { vorticity_enabled = p_enabled; }
bool GPUStableFluids2D::is_vorticity_enabled() const { return vorticity_enabled; }

void GPUStableFluids2D::set_vorticity_scale(float p_scale) { vorticity_scale = p_scale; }
float GPUStableFluids2D::get_vorticity_scale() const { return vorticity_scale; }

Ref<Texture2DRD> GPUStableFluids2D::get_density_texture() const {
    return density_texture_wrapper;
}
Ref<Texture2DRD> GPUStableFluids2D::get_velocity_texture() const {
    return velocity_texture_wrapper;
}

// ===== Interaction =====
void GPUStableFluids2D::add_impulse(Vector2 position, Vector2 strength, float radius, bool add_ink) {
    if (!rd || !impulse_buffer.is_valid()) {
        return;
    }
    // pack the impulse data into the uniform buffer (basic layout)
    struct ImpulseData {
        Vector2 pos;
        Vector2 vel;
        float radius;
        int add_ink;
    } data;
    data.pos = position;
    data.vel = strength;
    data.radius = radius;
    data.add_ink = add_ink ? 1 : 0;

    PackedByteArray arr;
    arr.resize(sizeof(data));
    memcpy(arr.append().ptr(), &data, sizeof(data));
    rd->buffer_update(impulse_buffer, 0, sizeof(data), arr);
}

void GPUStableFluids2D::reset() {
    if (rd) {
        _create_textures();
    }
}

// ===== Godot Lifecycle =====
void GPUStableFluids2D::_ready() {
    if (Engine::get_singleton()->is_editor_hint()) {
        return;
    }
    // obtain a pointer to the active rendering device from the server
    rd = RenderingServer::get_singleton()->get_rendering_device();
    _create_textures();
    _create_compute_pipeline();
    initialized = true;
}

void GPUStableFluids2D::_process(double delta) {
    if (!initialized || Engine::get_singleton()->is_editor_hint()) {
        return;
    }
    if (needs_recreate) {
        _create_textures();
        _create_compute_pipeline();
        needs_recreate = false;
    }
    _run_compute_shader();
}

// ===== Private Helpers =====
void GPUStableFluids2D::_create_textures() {
    if (!rd) return;

    // Free old textures
    auto free_tex = [&](RID &rid) {
        if (rid.is_valid()) {
            rd->free_rid(rid);
            rid = RID();
        }
    };
    free_tex(velocity_tex);
    free_tex(density_tex);
    free_tex(pressure_tex);
    free_tex(divergence_tex);
    free_tex(vorticity_tex);
    free_tex(velocity_offsets_tex);
    free_tex(pressure_offsets_tex);

    // Create texture format (RGBA 32-bit float)
    Ref<RDTextureFormat> format;
    format.instantiate();
    format->set_width(width);
    format->set_height(height);
    format->set_format(RenderingDevice::DATA_FORMAT_R32G32B32A32_SFLOAT);
    format->set_usage_bits(RenderingDevice::TEXTURE_USAGE_STORAGE_BIT | RenderingDevice::TEXTURE_USAGE_CAN_COPY_FROM_BIT);

    Ref<RDTextureView> view;
    view.instantiate();

    TypedArray<PackedByteArray> empty_data;

    velocity_tex = rd->texture_create(format, view, empty_data);
    density_tex = rd->texture_create(format, view, empty_data);
    pressure_tex = rd->texture_create(format, view, empty_data);
    divergence_tex = rd->texture_create(format, view, empty_data);
    vorticity_tex = rd->texture_create(format, view, empty_data);
    velocity_offsets_tex = rd->texture_create(format, view, empty_data);
    pressure_offsets_tex = rd->texture_create(format, view, empty_data);

    // Create wrapper textures (assume Texture2DRD has set_rid method)
    density_texture_wrapper.instantiate();
    density_texture_wrapper->set_texture_rd_rid(density_tex);
    velocity_texture_wrapper.instantiate();
    velocity_texture_wrapper->set_texture_rd_rid(velocity_tex);
}

void GPUStableFluids2D::_create_compute_pipeline() {
    if (!rd) return;

    String shader_path = "res://fluidsim.glsl";
    Ref<RDShaderFile> shader_file = ResourceLoader::get_singleton()->load(shader_path, "RDShaderFile");
    if (shader_file.is_null()) {
        ERR_PRINT(vformat("Failed to load compute shader: %s", shader_path));
        return;
    }
    Ref<RDShaderSPIRV> shader_spirv = shader_file->get_spirv();
    if (shader_spirv.is_null()) {
        ERR_PRINT("Failed to get SPIR-V from shader");
        return;
    }

    if (shader.is_valid()) rd->free_rid(shader);
    if (pipeline.is_valid()) rd->free_rid(pipeline);

    shader = rd->shader_create_from_spirv(shader_spirv);
    if (!shader.is_valid()) {
        ERR_PRINT("Failed to create shader");
        return;
    }

    TypedArray<RDUniform> uniforms;

    // Velocity
    Ref<RDUniform> u_velocity;
    u_velocity.instantiate();
    u_velocity->set_uniform_type(RenderingDevice::UNIFORM_TYPE_IMAGE);
    u_velocity->set_binding(0);
    u_velocity->add_id(velocity_tex);
    uniforms.append(u_velocity);

    // Density
    Ref<RDUniform> u_density;
    u_density.instantiate();
    u_density->set_uniform_type(RenderingDevice::UNIFORM_TYPE_IMAGE);
    u_density->set_binding(1);
    u_density->add_id(density_tex);
    uniforms.append(u_density);

    // Pressure
    Ref<RDUniform> u_pressure;
    u_pressure.instantiate();
    u_pressure->set_uniform_type(RenderingDevice::UNIFORM_TYPE_IMAGE);
    u_pressure->set_binding(2);
    u_pressure->add_id(pressure_tex);
    uniforms.append(u_pressure);

    // Divergence
    Ref<RDUniform> u_divergence;
    u_divergence.instantiate();
    u_divergence->set_uniform_type(RenderingDevice::UNIFORM_TYPE_IMAGE);
    u_divergence->set_binding(3);
    u_divergence->add_id(divergence_tex);
    uniforms.append(u_divergence);

    // Vorticity
    Ref<RDUniform> u_vorticity;
    u_vorticity.instantiate();
    u_vorticity->set_uniform_type(RenderingDevice::UNIFORM_TYPE_IMAGE);
    u_vorticity->set_binding(4);
    u_vorticity->add_id(vorticity_tex);
    uniforms.append(u_vorticity);

    // Velocity offsets
    Ref<RDUniform> u_vel_offsets;
    u_vel_offsets.instantiate();
    u_vel_offsets->set_uniform_type(RenderingDevice::UNIFORM_TYPE_IMAGE);
    u_vel_offsets->set_binding(5);
    u_vel_offsets->add_id(velocity_offsets_tex);
    uniforms.append(u_vel_offsets);

    // Pressure offsets
    Ref<RDUniform> u_press_offsets;
    u_press_offsets.instantiate();
    u_press_offsets->set_uniform_type(RenderingDevice::UNIFORM_TYPE_IMAGE);
    u_press_offsets->set_binding(6);
    u_press_offsets->add_id(pressure_offsets_tex);
    uniforms.append(u_press_offsets);

    // Impulse uniform buffer
    if (!impulse_buffer.is_valid()) {
        PackedByteArray empty;
        impulse_buffer = rd->uniform_buffer_create(256, empty);
    }
    Ref<RDUniform> u_impulse;
    u_impulse.instantiate();
    u_impulse->set_uniform_type(RenderingDevice::UNIFORM_TYPE_UNIFORM_BUFFER);
    u_impulse->set_binding(7);
    u_impulse->add_id(impulse_buffer);
    uniforms.append(u_impulse);

    // Simulation parameters uniform buffer
    if (!sim_params_buffer.is_valid()) {
        PackedByteArray empty;
        sim_params_buffer = rd->uniform_buffer_create(sizeof(SimParams), empty);
    }
    Ref<RDUniform> u_sim_params;
    u_sim_params.instantiate();
    u_sim_params->set_uniform_type(RenderingDevice::UNIFORM_TYPE_UNIFORM_BUFFER);
    u_sim_params->set_binding(8);
    u_sim_params->add_id(sim_params_buffer);
    uniforms.append(u_sim_params);

    uniform_set = rd->uniform_set_create(uniforms, shader, 0);
    pipeline = rd->compute_pipeline_create(shader);
}

void GPUStableFluids2D::_run_compute_shader(int pass) {
    if (!rd || !pipeline.is_valid()) {
        return;
    }

    // Update simulation parameters uniform buffer
    if (sim_params_buffer.is_valid()) {
        SimParams params;
        params.resolution[0] = (float)width;
        params.resolution[1] = (float)height;
        params.dt = timestep;
        params.viscosity = viscosity;
        params.diffusion = 0.0f; // TODO: Add diffusion parameter to class
        params.force_pos[0] = 0.5f * width; // Default center
        params.force_pos[1] = 0.5f * height;
        params.force_vel[0] = 0.0f;
        params.force_vel[1] = 0.0f;
        params.force_radius = 10.0f;
        params.force_strength = 1.0f;
        params.density_amount = 1.0f;
        params.pass = pass; // Use the provided pass parameter
        params._pad[0] = params._pad[1] = params._pad[2] = 0.0f;
        
        PackedByteArray pba;
        pba.resize(sizeof(SimParams));
        memcpy(pba.ptrw(), &params, sizeof(SimParams));
        rd->buffer_update(sim_params_buffer, 0, sizeof(SimParams), pba);
    }

    int workgroup_x = (width + 7) / 8;
    int workgroup_y = (height + 7) / 8;

    // build a compute list and dispatch it
    int64_t cl = rd->compute_list_begin();
    rd->compute_list_bind_compute_pipeline(cl, pipeline);
    rd->compute_list_bind_uniform_set(cl, uniform_set, 0);
    rd->compute_list_dispatch(cl, workgroup_x, workgroup_y, 1);
    rd->compute_list_end();

    rd->submit();
    rd->sync();
}

} // namespace godot

// ===== GDExtension Entry Point =====
extern "C" {
GDExtensionBool GDE_EXPORT gpu_fluids_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address,
                                                    GDExtensionClassLibraryPtr p_library,
                                                    GDExtensionInitialization *r_initialization) {
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer([](godot::ModuleInitializationLevel p_level) {
        if (p_level == godot::MODULE_INITIALIZATION_LEVEL_SCENE) {
            godot::ClassDB::register_class<godot::GPUStableFluids2D>();
        }
    });
    init_obj.register_terminator([](godot::ModuleInitializationLevel p_level) {});
    init_obj.set_minimum_library_initialization_level(godot::MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}
}