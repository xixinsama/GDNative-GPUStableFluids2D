#pragma once

namespace godot {

// --- Camera / Domain following ---
enum class FollowMode {
	Camera2D = 0, // Follow first Camera2D in scene
	Node2D   = 1, // Follow specific Node2D via follow_path
	None     = 2, // Fluid domain stays fixed at origin
};

enum class DomainWrapMode {
	Toroidal = 0, // Textures wrap at boundaries (infinite field illusion)
	Clamp    = 1, // Hard clamp at domain edges
};

// --- Emitter modes ---
enum class EmissionMode {
	Continuous = 0, // Emit at regular intervals
	Burst      = 1, // Emit all at once
	Periodic   = 2, // Emit bursts periodically
};

enum class EmissionShape {
	Point        = 0,
	Circle       = 1,
	Rect         = 2,
	LineSegment  = 3,
	TextureMask  = 4,
};

enum class EmitterPreset {
	Custom    = 0,
	Explosion = 1,
	Mist      = 2,
	Smoke     = 3,
	Fountain  = 4,
	Steam     = 5,
	Fire      = 6,
};

// --- Force patterns ---
enum class ForcePattern {
	Directional    = 0,
	PointRadial    = 1,
	Swirl          = 2,
	Centripetal    = 3,
	Centrifugal    = 4,
};

enum class ForcePreset {
	Custom    = 0,
	Wind      = 1,
	Gravity   = 2,
	Vortex    = 3,
	Explosion = 4,
	Magnet    = 5,
};

// --- Velocity generation ---
enum class VelocityPattern {
	Directional = 0,
	Radial      = 1,
	Random      = 2,
};

// --- Display ---
enum class DisplayMode {
	Density     = 0,
	Velocity    = 1,
	Pressure    = 2,
	Divergence  = 3,
	Vorticity   = 4,
};

// --- Interaction ---
enum class InteractionMode {
	Draw  = 0, // Inject color + velocity
	Push  = 1, // Push fluid away from cursor
	Pull  = 2, // Pull fluid toward cursor
	Swirl = 3, // Rotate fluid around cursor
};

// --- Tile obstacle ---
enum class TileObstacleMode {
	AllTiles      = 0,
	LayerSpecific = 1,
	PhysicsLayer  = 2,
};

enum class TileFillMode {
	Opaque         = 0,
	PerTileShape   = 1,
	CollisionShape = 2,
};

} // namespace godot
