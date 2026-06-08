#pragma once

// ──────────────────────────────────────────────────────────
//  Central debug toggle for GPUStableFluids2D plugin.
//  Set to 1 to enable verbose logging (pipeline steps,
//  pipeline validation, first-frame markers, etc.).
//  Set to 0 for release builds — ALL print/printerr
//  calls guarded by this flag are compiled out.
// ──────────────────────────────────────────────────────────
#define FLUID_DEBUG_LOG 0

#if FLUID_DEBUG_LOG
	#define FLUID_PRINT(...) UtilityFunctions::print(__VA_ARGS__)
	#define FLUID_PRINTERR(...) UtilityFunctions::printerr(__VA_ARGS__)
#else
	#define FLUID_PRINT(...)
	#define FLUID_PRINTERR(...)
#endif
