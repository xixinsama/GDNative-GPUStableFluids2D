# GPUStableFluids2D — Usage Guide

## Quick Start

1. Copy `project/bin/gpu_stable_fluids_2d.gdextension` and the `bin/` folder into your Godot project
2. Add a `GPUStableFluids2D` node to your scene
3. Add a `FluidDisplay2D` node, point `sim_source_path` at the simulation node
4. Add a `FluidMouseInteractor2D` as child of the simulation node to enable drawing
5. Run!

## Node Reference

### Core — `GPUStableFluids2D` (Node2D)
Main simulation controller. Manages full Navier-Stokes simulation on GPU.

**Key Properties:**
- `width`, `height` (32-2048): Simulation grid resolution. Higher = more detail, slower.
- `timestep` (0.1-10.0): Simulation speed. 1.0 = real-time.
- `viscosity` (0-0.01): Fluid thickness. 0 = water-like, higher = honey-like.
- `poisson_iterations` (1-200): Pressure solve quality. 40 = default.
- `vorticity_enabled`, `vorticity_scale`: Adds swirl detail to prevent blurring.
- `follow_mode`: Camera2D (auto-follow camera) / Node2D / None.
- `follow_path`: Node path for follow target (typically your Camera2D).
- `fluid_world_size`: World-space size of the fluid domain.
- `subtractive_mixing`: CMY color mixing (useful for ink/watercolor effects).

**Key Methods:**
- `add_impulse(pos, strength, radius, add_ink)`: Single force+color injection.
- `queue_draw_request(pos, color, vel, color_r, vel_r)`: Queue a draw point.
- `reset()`: Clear all fluid state.
- `get_output_texture()`: Returns Texture2DRD for display.

### Display — `FluidDisplay2D` (Sprite2D)
Renders fluid output. Set `sim_source_path` → GPUStableFluids2D node.

### Interaction — `FluidMouseInteractor2D` (Node2D)
Converts mouse input to fluid draw requests.
- `interaction_mode`: Draw / Push / Pull / Swirl.
- `brush_size`, `velocity_scale`, `draw_color`: Configure brush behavior.

### Emitters — `FluidEmitter2D` (Node2D)
Particle/color emitter with 7 presets.
- `emission_mode`: Continuous / Burst / Periodic.
- `emission_shape`: Point / Circle / Rect / LineSegment.
- `emission_preset`: Explosion / Mist / Smoke / Fountain / Steam / Fire.
- `emit_interval`, `burst_count`: Control emission rate.

### Emitters — `FluidForceEmitter2D` (Node2D)
Force field emitter with 6 presets.
- `force_pattern`: Directional / PointRadial / Swirl / Centripetal / Centrifugal.
- `force_preset`: Wind / Gravity / Vortex / Explosion / Magnet.

### Obstacles — `FluidObstacle2D` (Node2D)
Marks scene objects as fluid obstacles. Auto-detects velocity from RigidBody2D/CharacterBody2D parent.

### TileMap — `FluidTileMapObstacle2D` (Node2D)
Converts TileMapLayer into fluid obstacles.

### Lighting — `FluidLightOccluder2D` (Node2D)
Bridges fluid density to Godot's 2D lighting system. Creates LightOccluder2D child nodes dynamically. Use with PointLight2D + CanvasModulate for dynamic fluid shadows.

### Config — `FluidDomain2D` (RefCounted)
Saveable resource with fluid simulation parameters. Call `apply_to(sim)` to configure a simulation node.

### Debug — `FluidVorticityVisualizer2D` (Node2D)
Debug overlay for visualizing internal simulation fields (vorticity, pressure, divergence).

## Typical Scene Setup

```
Node2D (World)
├── Camera2D
├── CanvasModulate (color: dark gray for ambient)
├── PointLight2D (for fluid lighting)
├── GPUStableFluids2D
│   ├── FluidMouseInteractor2D
│   └── FluidLightOccluder2D
├── FluidDisplay2D (sim_source_path → GPUStableFluids2D)
├── FluidObstacle2D (child of StaticBody2D/RigidBody2D)
├── FluidEmitter2D (for particle effects)
└── FluidForceEmitter2D (for wind/vortex effects)
```

## Building from Source

```bash
git submodule update --init --recursive
scons platform=windows target=template_debug -j4
```

Output: `project/bin/windows/GPUStableFluids2D.windows.template_debug.x86_64.dll`
