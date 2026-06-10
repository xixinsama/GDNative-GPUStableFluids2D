# CLAUDE.md — GPUStableFluids2D

Godot 4 GDExtension plugin for real-time 2D Navier-Stokes fluid simulation on GPU via
RenderingDevice compute shaders.

## Project Layout

```
GDNative-GPUStableFluids2D/
├── src/                          # C++ GDExtension source
│   ├── register_types.cpp        # Module registration (entry: fluid_sim_library_init)
│   ├── GPU_stable_fluids_2D_init.{h,cpp}  # Main Node2D — CPU-side sim node
│   ├── core/
│   │   ├── gpu_resource_manager.{h,cpp}   # RD textures, pipelines, buffers
│   │   ├── fluid_render_pipeline.{h,cpp}  # 11-step compute dispatch
│   │   ├── fluid_domain.{h,cpp}           # Domain wrapping / flow control
│   │   ├── fluid_types.h                  # FollowMode, DomainWrapMode enums
│   │   └── sim_params.h                   # SimParams struct
│   ├── display/
│   │   ├── fluid_display_2d.{h,cpp}       # Sprite2D — renders output texture
│   │   ├── fluid_mouse_interactor_2d.{h,cpp}  # Mouse input → draw requests
│   │   └── fluid_vorticity_visualizer_2d.{h,cpp}
│   ├── emitters/
│   │   ├── fluid_emitter_2d.{h,cpp}       # Node2D — continuous/burst/periodic emission
│   │   └── fluid_force_emitter_2d.{h,cpp} # GPU force emitter
│   ├── obstacles/
│   │   ├── fluid_obstacle_2d.{h,cpp}      # Obstacle node for static/rigid bodies
│   │   ├── fluid_obstacle_drawer.{h,cpp}  # CPU raster → GPU obstacle texture
│   │   └── fluid_tile_map_obstacle_2d.{h,cpp}
│   ├── lighting/
│   │   └── fluid_light_occluder_2d.{h,cpp}
│   └── utils/
│       ├── draw_request.h                 # DrawRequest + BatchPoint (48B std430) structs
│       ├── fluid_coord_utils.h
│       └── polygon_rasterizer.h
├── demo/                      # Godot demo project
│   ├── project.godot            # Godot 4.6 project config
│   ├── demo_main.tscn           # Demo scene
│   ├── demo_controller.gd       # Keyboard shortcuts, emitter presets
│   ├── perf_hud.gd              # FPS/FT/Grid performance overlay
│   ├── graph_drawer.gd          # FPS history graph
│   └── shaders/                 # 12 GLSL compute shaders (see below)
├── godot-cpp/                   # Git submodule — Godot C++ bindings
├── SConstruct                   # SCons build (primary)
├── CMakeLists.txt               # CMake build (alternative)
├── ARCHITECTURE.md              # Detailed architecture doc
└── CLAUDE.md                    # This file
```

## Build

```bash
# Windows — SCons (primary)
scons target=editor -j4           # Editor build
scons target=template_debug -j4   # Debug build

# Output goes to bin/windows/ and demo/bin/windows/
```

Build system: **SCons** (reads `SConstruct` + `godot-cpp/SConstruct`).
The CMakeLists.txt is a stale template — use SCons.

No special dependencies beyond Python 3 + SCons + MSVC/MinGW toolchain.

## Architecture

### Registered Node Types (9 classes)
| Class | Base | Role |
|---|---|---|
| `GPUStableFluids2D` | Node2D | Main sim — owns GPU resources, runs compute pipeline |
| `FluidDisplay2D` | Sprite2D | Renders output via Texture2DRD |
| `FluidMouseInteractor2D` | Node2D | Mouse input → draw requests |
| `FluidEmitter2D` | Node2D | Continuous/burst/periodic particle emission |
| `FluidForceEmitter2D` | Node2D | GPU-side force field emission |
| `FluidObstacle2D` | Node2D | Static/kinematic obstacle (attach to bodies) |
| `FluidTileMapObstacle2D` | Node2D | TileMap-based obstacles |
| `FluidLightOccluder2D` | Node2D | Light occlusion for fluid |
| `FluidVorticityVisualizer2D` | Node2D | Vorticity debug visualization |
| `FluidDomain2D` | Node2D | Domain wrapping control |

### GPU Compute Pipeline (11 steps in `FluidRenderPipeline::execute()`)

1. `splat_batch` — Draw requests → velocity + color textures
2. `obstacle_force` — Obstacle interaction (apply force near obstacles)
3. `advect_velocity` — Backtrace advection of velocity field
4. `diffuse_velocity` — Jacobi diffusion (only if viscosity > 0)
5. `vorticity` — Vorticity confinement (only if enabled)
6. `divergence` — Compute velocity divergence
7. `solve_pressure` — Jacobi pressure solve (N iterations)
8. `subtract_gradient` — Pressure gradient subtraction
9. `boundary` — Neumann boundary conditions (velocity=-1, pressure=+1)
10. `advect_color` — Backtrace advection of color/density field
11. `copy_obstacle` — Copy current→previous obstacle texture

### Shader Files (12 compute shaders)

| Shader | Sets | Purpose |
|---|---|---|
| `advect.glsl` | sampler2D, sampler2D, image2D | Semi-Lagrangian advection |
| `boundary.glsl` | image2D, image2D | Neumann boundary conditions |
| `copy_texture.glsl` | image2D, image2D | Texture copy |
| `divergence.glsl` | image2D, image2D | Velocity divergence |
| `jacobi.glsl` | image2D, image2D, image2D | Jacobi iterative solver |
| `marching_square_extract.glsl` | — | (unused, for future iso-surface extraction) |
| `obstacle_force.glsl` | image2D×2, sampler2D×2 | Obstacle boundary forces |
| `render_density_lit.glsl` | — | (unused, for future lit rendering) |
| `shift_texture.glsl` | sampler2D, image2D | Domain shift for camera following |
| `splat_batch.glsl` | storage_buffer, image2D, image2D | Batch splat from draw requests |
| `subtract.glsl` | image2D, image2D, image2D | Pressure gradient subtraction |
| `vorticity.glsl` | image2D, image2D | Vorticity confinement |

### Push Constant Layouts (all use `std430`, 16 bytes total)

```cpp
struct AdvectionPushConstant   { float resolution[2]; float dt; float rdx; };
struct JacobiPushConstant      { float resolution[2]; float alpha; float rbeta; };
// ... (see fluid_render_pipeline.h for all)
```

### GPU Texture Format
All simulation textures: `DATA_FORMAT_R32G32B32A32_SFLOAT` (128bpp)
Usage: `STORAGE | SAMPLING | CAN_COPY_TO | CAN_COPY_FROM | CAN_UPDATE`

### Texture Layout (7 RGBA32F textures)
| RID | Purpose |
|---|---|
| `tex_velocity` | Velocity field (RG = vx,vy) |
| `tex_pressure` | Pressure scalar |
| `tex_color` | Color/density (RGB = color, A = density) |
| `tex_divergence` | Divergence scalar |
| `tex_temp` | Ping-pong swap buffer |
| `tex_obstacle` | Current obstacle mask (RG = velocity, A = alpha) |
| `tex_obstacle_pre` | Previous frame obstacle mask |

## Known Fixes Applied (2026-06-08)

### Forward+ Crash Fix
- **Root cause**: Textures missing `TEXTURE_USAGE_CAN_COPY_FROM_BIT`. Forward+ renderer
  tries to copy RD textures for canvas rendering; without copy-from permission, Vulkan
  rejects the operation.
- **Fix**: Added `CAN_COPY_FROM_BIT` to texture usage in `gpu_resource_manager.cpp`.
- **Additional**: Insert `rd->barrier()` after compute pipeline to transition textures
  from storage-image layout to a sampling-compatible layout.
- **Additional**: Only call `set_texture_rd_rid()` when the RID actually changes
  (avoid redundant updates every frame).

### Previous Fixes (2026-06-07)
- GDScript parse error in demo_controller.gd → fixed
- Null RD SIGSEGV → guarded with `device != null` checks
- BatchPoint struct mismatch (C++ vs GLSL padding) → fixed to 48 bytes std430
- Push constant type mismatch (int vs float bit pattern) → used properly aligned structs
- Hardcoded dt → parameterized with `p_delta`
- Emitter preset overwrite → removed redundant `_apply_preset()` call in `_ready()`

## Debugging

### Enable pipeline debug logging
Set `FLUID_DEBUG_LOG` to 1 in `src/core/fluid_render_pipeline.cpp:14` before building.

### Check Godot renderer
The sim requires Forward+ or Mobile renderer (RenderingDevice is not available in Compatibility).

### Common issues
- If `GPUStableFluids2D` log stops at "first frame DONE": Forward+ renderer crash
  (this was the CAN_COPY_FROM_BIT issue)
- If no RenderingDevice: project is using Compatibility renderer
- If pipeline fails: check SPIR-V compilation in Godot editor shader import

## FluidPlayGround Reference
This project is based on https://github.com/3c0tr/FluidPlayGround
Key differences: GDExtension (C++) vs GDScript, more compute shaders, obstacle system,
emitter system with presets, and domain-following camera support.
