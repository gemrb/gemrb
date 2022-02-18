#include "MapReverb.h"
#include "Map.h"

namespace GemRB {

MapReverb::MapReverb (Map& _map) :
		reverbMapping(gamedata->LoadTable("reverbs"))
	, reverbs(gamedata->LoadTable("reverb"))
	, map(_map)
	, reverbProfile(EFX_PROFILE_REVERB_INVALID) {
	MapReverbProperties _properties = {EFX_REVERB_GENERIC, true};
	properties = _properties;

	/* Map says nothing, attempt to read from 2DA. */
	if (EFX_PROFILE_REVERB_INVALID == map.SongHeader.reverbID) {
		reverbProfile = obtainProfile();
	} else {
		reverbProfile = loadProperties(map.SongHeader.reverbID);
	}

	/* If still nothing, fallback to something Ok. */
	if (EFX_PROFILE_REVERB_INVALID == reverbProfile && reverbs) {
		if (map.AreaType & (AT_OUTDOOR|AT_CITY|AT_FOREST)) {
			reverbProfile = loadProperties(EFX_PROFILE_OUTSIDE);
		} else if (map.AreaType & AT_DUNGEON) {
			reverbProfile = loadProperties(EFX_PROFILE_DUNGEON);
		} else {
			reverbProfile = loadProperties(EFX_PROFILE_DEFAULT);
		}
	}
}

void MapReverb::getReverbProperties(MapReverbProperties& props) const
{
	memcpy(&props, &properties, sizeof(MapReverbProperties));
}

unsigned char MapReverb::loadProperties (unsigned char profileNumber) {
	if (0 == profileNumber) {
		properties.reverbDisabled = true;
		return 0;
	} else if (profileNumber > reverbs->GetRowCount()) {
		return EFX_PROFILE_REVERB_INVALID;
	}

	std::string efxProfileName = reverbs->QueryField(profileNumber, 0);
	StringToUpper(efxProfileName);

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

	float decay = strtof(reverbs->QueryField(profileNumber, 2).c_str(), NULL);
	if (decay >= 0.0f && decay <= 20.0f) {
		properties.reverbData.flDecayTime = decay;
	}

	/* TODO: deal with DAMPING, REVERB_LEVEL, VOLUME */

	return profileNumber;
}

unsigned char MapReverb::obtainProfile () {
	if (!reverbMapping || !reverbs) {
		return loadProperties(0);
	}

	TableMgr::index_t rows = reverbMapping->GetRowCount();
	unsigned char configValue = 0;

	for (TableMgr::index_t i = 0; i < rows; ++i) {
		ResRef mapName = reverbMapping->GetRowName(i);
		if (mapName == map.WEDResRef) {
			uint8_t profile = reverbMapping->QueryFieldUnsigned<uint8_t>(i, 0);

			if (profile < EFX_MAX_REVERB_PROFILE_INDEX) {
				configValue = profile;
			}
			break;
		}
	}

	return loadProperties(configValue);
}

}
