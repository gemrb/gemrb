// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef AREIMPORTER_H
#define AREIMPORTER_H

#include "Map.h"
#include "MapMgr.h"

namespace GemRB {

class ActorMgr;
class EffectQueue;
class TileMapMgr;

class AREImporter : public MapMgr {
private:
	AREVersion bigheader = AREVersion::V1_0;
	ResRef WEDResRef;
	ieDword LastSave = 0;
	ieDword AreaFlags = 0;
	MapEnv AreaType = AT_UNINITIALIZED;
	ieWord WRain = 0, WSnow = 0, WFog = 0, WLightning = 0, WUnknown = 0;
	ieDword ActorStubOffset = 0, EmbeddedCreOffset = 0, AnimOffset = 0, AnimCount = 0;
	ieDword VerticesOffset = 0;
	ieDword DoorsCount = 0, DoorsOffset = 0;
	ieDword ExploredBitmapSize = 0, ExploredBitmapOffset = 0;
	ieDword EntrancesOffset = 0, EntrancesCount = 0;
	ieDword SongHeader = 0, RestHeader = 0;
	ieWord ActorCount = 0, VerticesCount = 0, AmbiCount = 0;
	ieWord ContainersCount = 0, InfoPointsCount = 0, ItemsCount = 0;
	ieDword VariablesCount = 0;
	ieDword ContainersOffset = 0, InfoPointsOffset = 0, ItemsOffset = 0;
	ieDword AmbiOffset = 0, VariablesOffset = 0;
	ieDword SpawnOffset = 0, SpawnCount = 0;
	ieDword TileOffset = 0, TileCount = 0;
	ieDword NoteOffset = 0, NoteCount = 0;
	ieDword TrapOffset = 0, TrapCount = 0; // only in ToB?
	ieDword EffectOffset = 0;
	ResRef Script;
	ResRef Dream1; // only in ToB
	ResRef Dream2; // only in ToB
	ieByte AreaDifficulty = 0;

public:
	AREImporter() noexcept = default;
	bool Import(DataStream* stream) override;
	bool ChangeMap(Map* map, bool day_or_night) override;
	Map* GetMap(const ResRef& resRef, bool day_or_night) override;
	int GetStoredFileSize(Map* map) override;
	/* stores an area in the Cache (swaps it out) */
	int PutArea(DataStream* stream, const Map* map) const override;

private:
	ieWord SavedAmbientCount(const Map*) const;
	void AdjustPSTFlags(AreaAnimation&) const;
	void ReadEffects(DataStream* ds, EffectQueue* fx, ieDword EffectsCount) const;
	CREItem* GetItem();
	void PutHeader(DataStream* stream, const Map* map) const;
	void PutPoints(DataStream* stream, const std::vector<Point>&) const;
	void PutPoints(DataStream* stream, const std::vector<SearchmapPoint>& points) const;
	void PutDoors(DataStream* stream, const Map* map, ieDword VertIndex) const;
	void PutItems(DataStream* stream, const Map* map) const;
	void PutContainers(DataStream* stream, const Map* map, ieDword VertIndex) const;
	void PutRegions(DataStream* stream, const Map* map, ieDword& VertIndex) const;
	void PutVertices(DataStream* stream, const Map* map, ieDword& regionVertStart, ieDword& doorVertStart) const;
	void PutSpawns(DataStream* stream, const Map* map) const;
	void PutScript(DataStream* stream, const Actor* ac, unsigned int index) const;
	void PutActors(DataStream* stream, const Map* map) const;
	void PutAnimations(DataStream* stream, const Map* map) const;
	void PutEntrances(DataStream* stream, const Map* map) const;
	void PutVariables(DataStream* stream, const Map* map) const;
	void PutAmbients(DataStream* stream, const Map* map) const;
	void PutMapnotes(DataStream* stream, const Map* map) const;
	void PutEffects(DataStream* stream, const EffectQueue& fxqueue) const;
	void PutTraps(DataStream* stream, const Map* map) const;
	void PutExplored(DataStream* stream, const Map* map) const;
	void PutTiles(DataStream* stream, const Map* map) const;
	void PutRestHeader(DataStream* stream, const Map* map) const;
	void PutMapAmbients(DataStream* stream, const Map* map) const;

	void GetSongs(DataStream* str, Map* map, std::vector<Ambient*>& ambients) const;
	void GetRestHeader(DataStream* str, Map* map) const;
	void GetInfoPoint(DataStream* str, int idx, Map* map) const;
	void GetContainer(DataStream* str, int idx, Map* map);
	void GetDoor(DataStream* str, int idx, Map* map, PluginHolder<TileMapMgr> tmm) const;
	void GetSpawnPoint(DataStream* str, int idx, Map* map) const;
	bool GetActor(DataStream* str, PluginHolder<ActorMgr> actorMgr, Map* map) const;
	void GetAreaAnimation(DataStream* str, Map* map) const;
	void GetAmbient(DataStream* str, std::vector<Ambient*>& ambients) const;
	void GetAutomapNotes(DataStream* str, Map* map) const;
	bool GetTrap(DataStream* str, int idx, Map* map) const;
	void GetTile(DataStream* str, Map* map) const;

	static Ambient* SetupMainAmbients(const Map::MainAmbients& mainAmbients);
};

}

#endif
