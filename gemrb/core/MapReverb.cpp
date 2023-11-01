
#include "MapReverb.h"

#include "GameData.h"
#include "Map.h"

namespace GemRB {

Map::MapReverb::MapReverb (MapEnv env, id_t reverbID)
{
	AutoTable reverbs = gamedata->LoadTable("reverb");
	
	MapReverbProperties _properties = {EFX_REVERB_GENERIC, true};
	properties = _properties;

	profile_t reverbProfile = EFX_PROFILE_REVERB_INVALID;
	if (reverbID == 0) {
		properties.reverbDisabled = true;
	} else if (reverbs) {
		reverbProfile = loadProperties(reverbs, reverbID);

		/* If still nothing, fallback to something Ok. */
		if (EFX_PROFILE_REVERB_INVALID == reverbProfile) {
			if (env & (AT_OUTDOOR|AT_CITY|AT_FOREST)) {
				reverbProfile = loadProperties(reverbs, EFX_PROFILE_OUTSIDE);
			} else if (env & AT_DUNGEON) {
				reverbProfile = loadProperties(reverbs, EFX_PROFILE_DUNGEON);
			} else {
				reverbProfile = loadProperties(reverbs, EFX_PROFILE_DEFAULT);
			}
		}
	}
}

Map::MapReverb::MapReverb(MapEnv env, const ResRef& mapref)
: MapReverb(env, obtainProfile(mapref))
{}

Map::MapReverb::profile_t Map::MapReverb::loadProperties(const AutoTable& reverbs, id_t reverbIdx)
{
	if (reverbIdx > reverbs->GetRowCount()) {
		return EFX_PROFILE_REVERB_INVALID;
	}

	const ieVariable efxProfileName = reverbs->QueryField(reverbIdx, 0);

	/* Limited to values seemingly used. */
	if (efxProfileName == "ARENA") {
		MapReverbProperties _properties = {EFX_REVERB_ARENA, false};
		properties = _properties;
	} else if (efxProfileName == "AUDITORIUM") {
		MapReverbProperties _properties = {EFX_REVERB_AUDITORIUM, false};
		properties = _properties;
	} else if (efxProfileName == "CITY") {
		MapReverbProperties _properties = {EFX_REVERB_CITY, false};
		properties = _properties;
	} else if (efxProfileName == "FOREST") {
		MapReverbProperties _properties = {EFX_REVERB_FOREST, false};
		properties = _properties;
	} else if (efxProfileName == "ROOM") {
		MapReverbProperties _properties = {EFX_REVERB_ROOM, false};
		properties = _properties;
	}

	float decay = strtof(reverbs->QueryField(reverbIdx, 2).c_str(), nullptr);
	if (decay >= 0.0f && decay <= 20.0f) {
		properties.reverbData.flDecayTime = decay;
	}

	/* TODO: deal with DAMPING, REVERB_LEVEL, VOLUME */

	return reverbIdx;
}

Map::MapReverb::id_t Map::MapReverb::obtainProfile(const ResRef& mapref) {
	const AutoTable& reverbMapping = gamedata->LoadTable("reverbs", true);
	if (!reverbMapping) return EFX_PROFILE_REVERB_INVALID;
	TableMgr::index_t rows = reverbMapping->GetRowCount();
	id_t configValue = 0;

	for (TableMgr::index_t i = 0; i < rows; ++i) {
		ResRef mapName = reverbMapping->GetRowName(i);
		if (mapName == mapref) {
			id_t profile = reverbMapping->QueryFieldUnsigned<uint8_t>(i, 0);

			if (profile < EFX_MAX_REVERB_PROFILE_INDEX) {
				configValue = profile;
			}
			break;
		}
	}

	return configValue;
}

}
