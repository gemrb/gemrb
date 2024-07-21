#ifndef MAP_REVERB_H
#define MAP_REVERB_H

#include "globals.h"

#define EFX_MAX_REVERB_PROFILE_INDEX 19
#define EFX_PROFILE_REVERB_INVALID 0xFF

#define EFX_PROFILE_DEFAULT 1
#define EFX_PROFILE_OUTSIDE 4
#define EFX_PROFILE_DUNGEON 5

namespace GemRB {

typedef struct {
	struct ReverbData {
		float flDensity;
		float flDiffusion;
		float flGain;
		float flGainHF;
		float flGainLF;
		float flDecayTime;
		float flDecayHFRatio;
		float flDecayLFRatio;
		float flReflectionsGain;
		float flReflectionsDelay;
		float flReflectionsPan1;
		float flReflectionsPan2;
		float flReflectionsPan3;
		float flLateReverbGain;
		float flLateReverbDelay;
		float flLateReverbPan1;
		float flLateReverbPan2;
		float flLateReverbPan3;
		float flEchoTime;
		float flEchoDepth;
		float flModulationTime;
		float flModulationDepth;
		float flAirAbsorptionGainHF;
		float flHFReference;
		float flLFReference;
		float flRoomRolloffFactor;
		int	iDecayHFLimit;
	} reverbData;
	bool reverbDisabled;
} MapReverbProperties;

}

// eaxenvir.ids mapping
#define EFX_REVERB_GENERIC			 { 1.000f, 1.000f, 0.316f, 0.891f, 1.000f, 1.490f, 0.830f, 1.000f, 0.050f, 0.007f, 0.000f, 0.000f, 0.000f, 1.259f, 0.011f, 0.000f, 0.000f, 0.000f, 0.250f, 0.000f, 0.250f, 0.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 1 }
#define EFX_REVERB_PADDEDCELL		 { 0.171f, 1.000f, 0.316f, 0.001f, 1.000f, 0.170f, 0.100f, 1.000f, 0.250f, 0.001f, 0.000f, 0.000f, 0.000f, 1.269f, 0.002f, 0.000f, 0.000f, 0.000f, 0.250f, 0.000f, 0.250f, 0.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 1 }
#define EFX_REVERB_ROOM				 { 0.429f, 1.000f, 0.316f, 0.593f, 1.000f, 0.400f, 0.830f, 1.000f, 0.150f, 0.002f, 0.000f, 0.000f, 0.000f, 1.063f, 0.003f, 0.000f, 0.000f, 0.000f, 0.250f, 0.000f, 0.250f, 0.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 1 }
#define EFX_REVERB_BATHROOM			 { 0.171f, 1.000f, 0.316f, 0.251f, 1.000f, 1.490f, 0.540f, 1.000f, 0.653f, 0.007f, 0.000f, 0.000f, 0.000f, 3.273f, 0.011f, 0.000f, 0.000f, 0.000f, 0.250f, 0.000f, 0.250f, 0.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 1 }
#define EFX_REVERB_LIVINGROOM		 { 0.977f, 1.000f, 0.316f, 0.001f, 1.000f, 0.500f, 0.100f, 1.000f, 0.205f, 0.003f, 0.000f, 0.000f, 0.000f, 0.281f, 0.004f, 0.000f, 0.000f, 0.000f, 0.250f, 0.000f, 0.250f, 0.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 1 }
#define EFX_REVERB_STONEROOM		 { 1.000f, 1.000f, 0.316f, 0.708f, 1.000f, 2.310f, 0.640f, 1.000f, 0.441f, 0.012f, 0.000f, 0.000f, 0.000f, 1.100f, 0.017f, 0.000f, 0.000f, 0.000f, 0.250f, 0.000f, 0.250f, 0.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 1 }
#define EFX_REVERB_AUDITORIUM		 { 1.000f, 1.000f, 0.316f, 0.578f, 1.000f, 4.320f, 0.590f, 1.000f, 0.403f, 0.020f, 0.000f, 0.000f, 0.000f, 0.717f, 0.030f, 0.000f, 0.000f, 0.000f, 0.250f, 0.000f, 0.250f, 0.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 1 }
#define EFX_REVERB_CONCERTHALL		 { 1.000f, 1.000f, 0.316f, 0.562f, 1.000f, 3.920f, 0.700f, 1.000f, 0.243f, 0.020f, 0.000f, 0.000f, 0.000f, 0.998f, 0.029f, 0.000f, 0.000f, 0.000f, 0.250f, 0.000f, 0.250f, 0.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 1 }
#define EFX_REVERB_CAVE				 { 1.000f, 1.000f, 0.316f, 1.000f, 1.000f, 2.910f, 1.300f, 1.000f, 0.500f, 0.015f, 0.000f, 0.000f, 0.000f, 0.706f, 0.022f, 0.000f, 0.000f, 0.000f, 0.250f, 0.000f, 0.250f, 0.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 0 }
#define EFX_REVERB_ARENA			 { 1.000f, 1.000f, 0.316f, 0.448f, 1.000f, 7.240f, 0.330f, 1.000f, 0.261f, 0.020f, 0.000f, 0.000f, 0.000f, 1.019f, 0.030f, 0.000f, 0.000f, 0.000f, 0.250f, 0.000f, 0.250f, 0.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 1 }
#define EFX_REVERB_HANGAR			 { 1.000f, 1.000f, 0.316f, 0.316f, 1.000f, 10.05f, 0.230f, 1.000f, 0.500f, 0.020f, 0.000f, 0.000f, 0.000f, 1.256f, 0.030f, 0.000f, 0.000f, 0.000f, 0.250f, 0.000f, 0.250f, 0.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 1 }
#define EFX_REVERB_CARPETTEDHALLWAY	 { 0.429f, 1.000f, 0.316f, 0.010f, 1.000f, 0.300f, 0.100f, 1.000f, 0.121f, 0.002f, 0.000f, 0.000f, 0.000f, 0.153f, 0.030f, 0.000f, 0.000f, 0.000f, 0.250f, 0.000f, 0.250f, 0.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 1 }
#define EFX_REVERB_HALLWAY			 { 0.364f, 1.000f, 0.316f, 0.708f, 1.000f, 1.490f, 0.590f, 1.000f, 0.246f, 0.007f, 0.000f, 0.000f, 0.000f, 1.661f, 0.011f, 0.000f, 0.000f, 0.000f, 0.250f, 0.000f, 0.250f, 0.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 1 }
#define EFX_REVERB_STONECORRIDOR	 { 1.000f, 1.000f, 0.316f, 0.761f, 1.000f, 2.700f, 0.790f, 1.000f, 0.247f, 0.013f, 0.000f, 0.000f, 0.000f, 1.576f, 0.020f, 0.000f, 0.000f, 0.000f, 0.250f, 0.000f, 0.250f, 0.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 1 }
#define EFX_REVERB_ALLEY			 { 1.000f, 0.300f, 0.316f, 0.733f, 1.000f, 1.490f, 0.860f, 1.000f, 0.250f, 0.007f, 0.000f, 0.000f, 0.000f, 0.995f, 0.011f, 0.000f, 0.000f, 0.000f, 0.125f, 0.950f, 0.250f, 0.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 1 }
#define EFX_REVERB_FOREST			 { 1.000f, 0.300f, 0.316f, 0.022f, 1.000f, 1.490f, 0.540f, 1.000f, 0.052f, 0.162f, 0.000f, 0.000f, 0.000f, 0.768f, 0.088f, 0.000f, 0.000f, 0.000f, 0.125f, 1.000f, 0.250f, 0.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 1 }
#define EFX_REVERB_CITY				 { 1.000f, 0.500f, 0.316f, 0.398f, 1.000f, 1.490f, 0.670f, 1.000f, 0.073f, 0.007f, 0.000f, 0.000f, 0.000f, 0.143f, 0.011f, 0.000f, 0.000f, 0.000f, 0.250f, 0.000f, 0.250f, 0.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 1 }
#define EFX_REVERB_MOUNTAINS		 { 1.000f, 0.270f, 0.316f, 0.056f, 1.000f, 1.490f, 0.210f, 1.000f, 0.041f, 0.300f, 0.000f, 0.000f, 0.000f, 0.192f, 0.100f, 0.000f, 0.000f, 0.000f, 0.250f, 1.000f, 0.250f, 0.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 0 }
#define EFX_REVERB_QUARRY			 { 1.000f, 1.000f, 0.316f, 0.316f, 1.000f, 1.490f, 0.830f, 1.000f, 0.000f, 0.061f, 0.000f, 0.000f, 0.000f, 1.778f, 0.025f, 0.000f, 0.000f, 0.000f, 0.125f, 0.700f, 0.250f, 0.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 1 }
#define EFX_REVERB_PLAIN			 { 1.000f, 0.210f, 0.316f, 0.100f, 1.000f, 1.490f, 0.500f, 1.000f, 0.058f, 0.179f, 0.000f, 0.000f, 0.000f, 0.109f, 0.100f, 0.000f, 0.000f, 0.000f, 0.250f, 1.000f, 0.250f, 0.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 1 }
#define EFX_REVERB_PARKINGLOT		 { 1.000f, 1.000f, 0.316f, 1.000f, 1.000f, 1.650f, 1.500f, 1.000f, 0.208f, 0.008f, 0.000f, 0.000f, 0.000f, 0.265f, 0.012f, 0.000f, 0.000f, 0.000f, 0.250f, 0.000f, 0.250f, 0.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 0 }
#define EFX_REVERB_SEWERPIPE		 { 0.307f, 0.800f, 0.316f, 0.316f, 1.000f, 2.810f, 0.140f, 1.000f, 1.639f, 0.014f, 0.000f, 0.000f, 0.000f, 3.247f, 0.021f, 0.000f, 0.000f, 0.000f, 0.250f, 0.000f, 0.250f, 0.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 1 }
#define EFX_REVERB_UNDERWATER		 { 0.364f, 1.000f, 0.316f, 0.010f, 1.000f, 1.490f, 0.100f, 1.000f, 0.596f, 0.007f, 0.000f, 0.000f, 0.000f, 7.079f, 0.011f, 0.000f, 0.000f, 0.000f, 0.250f, 0.000f, 1.180f, 0.348f, 0.994f, 5000.000f, 250.000f, 0.000f, 1 }
#define EFX_REVERB_DRUGGED			 { 0.429f, 0.500f, 0.316f, 1.000f, 1.000f, 8.390f, 1.390f, 1.000f, 0.876f, 0.002f, 0.000f, 0.000f, 0.000f, 3.108f, 0.030f, 0.000f, 0.000f, 0.000f, 0.250f, 0.000f, 0.250f, 1.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 0 }
#define EFX_REVERB_DIZZY			 { 0.364f, 0.600f, 0.316f, 0.631f, 1.000f, 17.23f, 0.560f, 1.000f, 0.139f, 0.020f, 0.000f, 0.000f, 0.000f, 0.494f, 0.030f, 0.000f, 0.000f, 0.000f, 0.250f, 1.000f, 0.810f, 0.310f, 0.994f, 5000.000f, 250.000f, 0.000f, 0 }
#define EFX_REVERB_PSYCHOTIC		 { 0.063f, 0.500f, 0.316f, 0.840f, 1.000f, 7.560f, 0.910f, 1.000f, 0.486f, 0.020f, 0.000f, 0.000f, 0.000f, 2.438f, 0.030f, 0.000f, 0.000f, 0.000f, 0.250f, 0.000f, 4.000f, 1.000f, 0.994f, 5000.000f, 250.000f, 0.000f, 0 }

#endif
