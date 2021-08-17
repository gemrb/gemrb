/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef MAP_H
#define MAP_H

#include "exports.h"
#include "globals.h"

#include "Bitmap.h"
#include "Interface.h"
#include "MapReverb.h"
#include "Scriptable/Scriptable.h"
#include "PathFinder.h"
#include "WorldMap.h"

#include <algorithm>
#include <queue>
#include <unordered_map>

template <class V> class FibonacciHeap;

namespace GemRB {

class Actor;
class Ambient;
class Animation;
class Bitmap;
class CREItem;
class GameControl;
class IniSpawn;
class Palette;
using PaletteHolder = Holder<Palette>;
class Particles;
struct PathNode;
class Projectile;
class ScriptedAnimation;
class TileMap;
class VEFObject;
class Wall_Polygon;

//distance of actors from spawn point
#define SPAWN_RANGE       400

//spawn flags
#define SPF_NOSPAWN		0x0001	//if set don't span if WAIT is set
#define SPF_ONCE		0x0002	//only spawn a single time
#define SPF_WAIT		0x0004	//spawn temporarily disabled

//area flags (pst uses them only for resting purposes!)
#define AF_NOSAVE         1
#define AF_TUTORIAL       2 // pst: "You cannot rest here."
#define AF_DEADMAGIC      4 // pst: "You cannot rest right now.", TODO iwd2: LOCKBATTLEMUSIC in areaflag.ids
//                        6 // pst: "You must obtain permission to rest here."
#define AF_DREAM          8 // unused in pst
/* TODO: implement these EE bits (plus PST:EE merged both worlds, bleargh)
#define AF_NOFATALITY    16 // Player1 death does not end the game
#define AF_NOREST        32 // Resting not allowed
#define AF_NOTRAVEL      64 // Travel not allowed
*/

enum MapEnv : ieWord {
	AT_UNINITIALIZED	= 0,
	AT_OUTDOOR        	= 1,
	AT_DAYNIGHT       	= 2,
	AT_WEATHER        	= 4,
	AT_CITY           	= 8,
	AT_FOREST         	= 0x10,
	AT_DUNGEON        	= 0x20,
	AT_EXTENDED_NIGHT 	= 0x40,
	AT_CAN_REST_INDOORS = 0x80,
	AT_PST_DAYNIGHT 	= 0x400
};

//area animation flags
#define A_ANI_ACTIVE          1        //if not set, animation is invisible
#define A_ANI_BLEND           2        //blend
#define A_ANI_NO_SHADOW       4        //lightmap doesn't affect it
#define A_ANI_PLAYONCE        8        //stop after endframe
#define A_ANI_SYNC            16       //synchronised draw (skip frames if needed)
#define A_ANI_RANDOM_START    32       //starts with a random frame in the start range
#define A_ANI_NO_WALL         64       //draw after walls (walls don't cover it)
#define A_ANI_NOT_IN_FOG      0x80     //not visible in fog of war
#define A_ANI_BACKGROUND      0x100    //draw before actors (actors cover it)
#define A_ANI_ALLCYCLES       0x200    //draw all cycles, not just the cycle specified
#define A_ANI_PALETTE         0x400    //has own palette set
#define A_ANI_MIRROR          0x800    //mirrored
#define A_ANI_COMBAT          0x1000   //draw in combat too
#define A_ANI_PSTBIT14        0x2000   // PST-only: unknown and rare, see #163 for area list
// TODO: BGEE extended flags:
// 0x2000: Use WBM resref
// 0x4000: Draw stenciled (can be used to stencil animations using the water overlay mask of the tileset, eg. to give water surface a more natural look)
// 0x8000: Use PVRZ resref

#define ANI_PRI_BACKGROUND	-9999

//creature area flags
#define AF_CRE_NOT_LOADED 1
#define AF_NAME_OVERRIDE  8
//same flags in IWD2
#define AF_SEEN_PARTY     2
#define AF_INVULNERABLE   4
#define AF_ENABLED        8

//direction flags (used in AreaLinks)
#define ADIRF_NORTH       0x01
#define ADIRF_EAST        0x02
#define ADIRF_SOUTH       0x04
#define ADIRF_WEST        0x08
#define ADIRF_CENTER      0x10 //not in the original engine

//getline flags
#define GL_NORMAL         0
#define GL_PASS           1
#define GL_REBOUND        2

//sparkle types
#define SPARKLE_PUFF      1
#define SPARKLE_EXPLOSION 2  //not in the original engine
#define SPARKLE_SHOWER    3

//in areas 10 is a magic number for resref counts
#define MAX_RESCOUNT 10

struct SongHeaderType {
	// used in bg1, set for a few copied areas in bg2 (but no files!)
	// everyone else uses the normal ARE ambients instead
	ResRef MainDayAmbient1;
	ResRef MainDayAmbient2; // except for one case, all Ambient2 are longer versions
	ieDword MainDayAmbientVol;
	ResRef MainNightAmbient1;
	ResRef MainNightAmbient2;
	ieDword MainNightAmbientVol;
	ieDword reverbID;
};

struct RestHeaderType {
	ieDword Strref[MAX_RESCOUNT];
	ResRef CreResRef[MAX_RESCOUNT];
	ieWord Difficulty;
	ieWord CreatureNum;
	ieWord Maximum;
	ieWord Enabled;
	ieWord DayChance;
	ieWord NightChance;
	ieDword sduration;
	ieWord rwdist, owdist;
};

struct Entrance {
	ieVariable Name;
	Point Pos;
	ieWord Face;
};

class MapNote {
	void swap(MapNote& mn) noexcept {
		if (&mn == this) return;
		std::swap(strref, mn.strref);
		std::swap(color, mn.color);
		std::swap(text, mn.text);
		std::swap(Pos, mn.Pos);
	}
public:
	// FIXME: things can get messed up by exposing these (specifically strref and text)
	ieStrRef strref;
	ieWord color;
	String text;
	Point Pos;
	bool readonly;

	MapNote& operator=( MapNote mn ) {
		// note the pass by value
		mn.swap(*this);
		return *this;
	}
	MapNote(const MapNote &rhs) = default;

	MapNote(String text, ieWord c, bool readonly)
	: strref(-1), text(std::move(text)), readonly(readonly)
	{
		color = Clamp<ieWord>(c, 0, 8);
		//update custom strref
		char* mbstring = MBCStringFromString(text);
		if (mbstring) {
			strref = core->UpdateString(-1, mbstring);
			free(mbstring);
		} else {
			strref = core->UpdateString(-1, "?");
			Log(WARNING, "Map", "Failed to update string from map note, possibly an encoding issue.");
		}
	}

	MapNote(ieStrRef ref, ieWord c, bool readonly)
	: strref(ref), readonly(readonly)
	{
		color = Clamp<ieWord>(c, 0, 8);
		String* tmp = core->GetString(ref);
		if (tmp) {
			text = std::move(*tmp);
			delete tmp;
		}
	}

	const Color& GetColor() const {
		static const Color colors[]={
		 ColorBlack,
		 ColorGray,
		 ColorViolet,
		 ColorGreen,
		 ColorOrange,
		 ColorRed,
		 ColorBlue,
		 ColorBlueDark,
		 ColorGreenDark
		};

		return colors[color];
	}
};

class Spawn {
public:
	ieVariable Name {};
	Point Pos;
	std::vector<ResRef> Creatures;
	ieWord Difficulty = 0;
	ieWord Frequency = 0;
	ieWord Method = 0;
	ieDword sduration = 0;      //spawn duration
	ieWord rwdist = 0;
	ieWord owdist = 0;  //maximum walk distances
	ieWord Maximum = 0;
	ieWord Enabled = 0;
	ieDword appearance = 0;
	ieWord DayChance = 0;
	ieWord NightChance = 0;
	ieDword NextSpawn = 0;
	// TODO: EE added several extra fields: Spawn frequency (another?), Countdown, Spawn weights for all Creatures
};

class SpawnGroup {
	std::vector<ResRef> ResRefs;
	int level;
public:
	SpawnGroup(std::vector<ResRef>&& resrefs, int level) noexcept
	: ResRefs(std::move(resrefs)), level(level)
	{}
	
	size_t Count() const noexcept {
		return ResRefs.size();
	}
	
	int Level() const noexcept {
		return level;
	}
	
	const ResRef& operator[](size_t i) const {
		return ResRefs[i];
	}
};

class GEM_EXPORT AreaAnimation {
public:
	mutable std::vector<Animation> animation; // FIXME: we need an "update" step, currently we do it in Draw() wich should remian const
	//dwords, or stuff combining to a dword
	Point Pos;
	ieDword appearance;
	ieDword Flags;
	// flags that must be touched by PST a bit only
	ieDword originalFlags;
	//these are on one dword
	ieWord sequence;
	ieWord frame;
	//these are on one dword
	ieWord transparency;
	ieWordSigned height;
	//these are on one dword
	ieWord startFrameRange;
	ieByte skipcycle;
	ieByte startchance;
	ieDword unknown48;
	//string values, not in any particular order
	ieVariable Name;
	ResRef BAM; //not only for saving back (StaticSequence depends on this)
	ResRef PaletteRef;
	// TODO: EE stores also the width/height for WBM and PVRZ resources (see Flags bit 13/15)
	PaletteHolder palette;
	AreaAnimation();
	AreaAnimation(const AreaAnimation& src);

	void InitAnimation();
	void SetPalette(const ResRef &PaletteRef);
	void BlendAnimation();
	bool Schedule(ieDword gametime) const;
	Region DrawingRegion() const;
	void Draw(const Region &screen, Color tint, BlitFlags flags) const;
	int GetHeight() const;
};

enum AnimationObjectType {AOT_AREA, AOT_SCRIPTED, AOT_ACTOR, AOT_SPARK, AOT_PROJECTILE, AOT_PILE};

//i believe we need only the active actors/visible inactive actors queues
#define QUEUE_COUNT 2

//priorities when handling actors, we really ignore the third one
#define PR_SCRIPT  0
#define PR_DISPLAY 1
#define PR_IGNORE  2

enum MAP_DEBUG_FLAGS : uint32_t {
	DEBUG_SHOW_INFOPOINTS   	= 0x01,
	DEBUG_SHOW_CONTAINERS   	= 0x02,
	DEBUG_SHOW_DOORS			= 0x04,
	DEBUG_SHOW_DOORS_SECRET		= 0x08,
	DEBUG_SHOW_DOORS_DISABLED	= 0x10,
	DEBUG_SHOW_DOORS_ALL		= (DEBUG_SHOW_DOORS|DEBUG_SHOW_DOORS_SECRET|DEBUG_SHOW_DOORS_DISABLED),
	DEBUG_SHOW_SEARCHMAP		= 0x20,
	DEBUG_SHOW_MATERIALMAP     	= 0x40,
	DEBUG_SHOW_HEIGHTMAP		= 0x80,
	DEBUG_SHOW_LIGHTMAP			= 0x0100,
	DEBUG_SHOW_WALLS			= 0x0200,
	DEBUG_SHOW_WALLS_ANIM_COVER	= 0x0400,
	DEBUG_SHOW_WALLS_ALL		= (DEBUG_SHOW_WALLS|DEBUG_SHOW_WALLS_ANIM_COVER),
	DEBUG_SHOW_FOG_UNEXPLORED	= 0x0800,
	DEBUG_SHOW_FOG_INVISIBLE	= 0x1000,
	DEBUG_SHOW_FOG_ALL			= (DEBUG_SHOW_FOG_UNEXPLORED|DEBUG_SHOW_FOG_INVISIBLE),
};

using aniIterator = std::list<AreaAnimation>::iterator;
using scaIterator = std::list<VEFObject*>::const_iterator;
using proIterator = std::list<Projectile*>::const_iterator;
using spaIterator = std::list<Particles*>::const_iterator;


class GEM_EXPORT Map : public Scriptable {
public:
	TileMap* TMap;
	Holder<Sprite2D> SmallMap;
	IniSpawn *INISpawn;
	ieDword AreaFlags;
	MapEnv AreaType = AT_UNINITIALIZED;
	ieWord Rain = 0;
	ieWord Snow = 0;
	ieWord Fog = 0;
	ieWord Lightning = 0;
	Bitmap ExploredBitmap;
	Bitmap VisibleBitmap;
	int version;
	ResRef WEDResRef;
	bool MasterArea;
	//this is set by the importer (not stored in the file)
	bool DayNight = false;
	//movies for day/night (only in ToB)
	ResRef Dream[2];
	Holder<Sprite2D> Background;
	ieDword BgDuration;
	ieDword LastGoCloser;
	MapReverb reverb;

private:
	uint32_t debugFlags = 0;
	ieStrRef trackString;
	int trackFlag;
	ieWord trackDiff;
	// tileProps contains the searchmap, the lightmap, the heightmap, and the material map
	// the assigned palette is the palette for the lightmap
	Holder<Sprite2D> tileProps;

	std::list<AreaAnimation> animations;
	std::vector< Actor*> actors;
	std::vector<WallPolygonGroup> wallGroups;
	std::list< VEFObject*> vvcCells;
	std::list< Projectile*> projectiles;
	std::list< Particles*> particles;
	std::vector< Entrance*> entrances;
	std::vector< Ambient*> ambients;
	std::vector<MapNote> mapnotes;
	std::vector< Spawn*> spawns;
	Actor** queue[QUEUE_COUNT];
	int Qcount[QUEUE_COUNT];
	unsigned int lastActorCount[QUEUE_COUNT];
	bool hostiles_visible = false;

	VideoBufferPtr wallStencil;
	Region stencilViewport;

	std::unordered_map<const void*, std::pair<VideoBufferPtr, Region>> objectStencils;

public:
	static constexpr uint32_t searchMapMask = 0xff000000;
	static constexpr uint32_t materialMapMask = 0x00ff0000;
	static constexpr uint32_t heightMapMask = 0x0000ff00;
	static constexpr uint32_t lightMapMask = 0x000000ff;

	Map(TileMap *tm, Holder<Sprite2D> tileProps, Holder<Sprite2D> sm);
	~Map(void) override;
	static void NormalizeDeltas(double &dx, double &dy, const double &factor = 1);
	static Point ConvertCoordToTile(const Point&);
	static Point ConvertCoordFromTile(const Point&);

	/** prints useful information on console */
	void dump(bool show_actors = false) const;
	TileMap *GetTileMap() const { return TMap; }
	/* gets the signal of daylight changes */
	bool ChangeMap(bool day_or_night);
	void SeeSpellCast(Scriptable *caster, ieDword spell) const;
	void SetTileMapProps(Holder<Sprite2D> props);
	void AutoLockDoors() const;
	void UpdateScripts();
	ResRef ResolveTerrainSound(const ResRef &sound, const Point &pos) const;
	void DoStepForActor(Actor *actor, ieDword time) const;
	void UpdateEffects();
	/* removes empty heaps and returns total itemcount */
	int ConsolidateContainers();
	/* transfers all piles (loose items) to another map */
	void CopyGroundPiles(Map *othermap, const Point &Pos) const;
	/* transfers all ever visible piles (loose items) to the specified position */
	void MoveVisibleGroundPiles(const Point &Pos);

	void DrawMap(const Region& viewport, uint32_t debugFlags);
	void PlayAreaSong(int SongType, bool restart = true, bool hard = false) const;
	void AddAnimation(AreaAnimation anim);
	aniIterator GetFirstAnimation() { return animations.begin(); }
	std::list<AreaAnimation>::const_iterator GetFirstAnimation() const { return animations.begin(); }
	AreaAnimation *GetNextAnimation(aniIterator &iter) const
	{
		if (iter == animations.end()) {
			return NULL;
		}
		return &*iter++;
	}
	const AreaAnimation *GetNextAnimation(std::list<AreaAnimation>::const_iterator &iter) const
	{
		if (iter == animations.end()) {
			return nullptr;
		}
		return &*iter++;
	}

	AreaAnimation *GetAnimation(const char *Name);
	size_t GetAnimationCount() const { return animations.size(); }

	void SetWallGroups(std::vector<WallPolygonGroup>&& walls)
	{
		wallGroups = std::move(walls);
	}
	bool BehindWall(const Point&, const Region&) const;
	void Shout(const Actor* actor, int shoutID, bool global) const;
	void ActorSpottedByPlayer(const Actor *actor) const;
	bool HandleAutopauseForVisible(Actor *actor, bool) const;
	void InitActors();
	void MarkVisited(const Actor *actor) const;
	void AddActor(Actor* actor, bool init);
	//counts the summons already in the area
	int CountSummons(ieDword flag, ieDword sex) const;
	//returns true if an enemy is near P (used in resting/saving)
	bool AnyEnemyNearPoint(const Point &p) const;
	
	
	int GetHeight(const Point &p) const;
	Color GetLighting(const Point &p) const;

	PathMapFlags GetBlockedInRadius(const Point&, unsigned int size, bool stopOnImpassable = true) const;
	PathMapFlags GetBlocked(const Point&) const;
	PathMapFlags GetBlocked(const Point&, int size) const;
	PathMapFlags GetBlockedNavmap(const Point &c) const;
	Scriptable *GetScriptableByGlobalID(ieDword objectID);
	Door *GetDoorByGlobalID(ieDword objectID) const;
	Container *GetContainerByGlobalID(ieDword objectID) const;
	InfoPoint *GetInfoPointByGlobalID(ieDword objectID) const;
	Actor* GetActorByGlobalID(ieDword objectID) const;
	Actor* GetActorInRadius(const Point &p, int flags, unsigned int radius) const;
	std::vector<Actor *> GetAllActorsInRadius(const Point &p, int flags, unsigned int radius, const Scriptable *see = NULL) const;
	const std::vector<Actor *> &GetAllActors() const { return actors; }
	int GetActorsInRect(Actor**& actorlist, const Region& rgn, int excludeFlags) const;
	Actor* GetActor(const char* Name, int flags) const;
	Actor* GetActor(int i, bool any) const;
	Actor* GetActor(const Point &p, int flags, const Movable *checker = NULL) const;
	Scriptable *GetScriptableByDialog(const ResRef& resref) const;
	Actor *GetItemByDialog(const ResRef& resref) const;
	Actor *GetActorByResource(const char *resref) const;
	Actor *GetActorByScriptName(const char *name) const;
	bool HasActor(const Actor *actor) const;
	bool SpawnsAlive() const;
	void RemoveActor(Actor* actor);

	int GetActorCount(bool any) const;
	//fix actors position if required
	void JumpActors(bool jump);
	//selects all selectable actors in the area
	void SelectActors() const;
	//if items == true, remove noncritical items from ground piles too
	void PurgeArea(bool items);

	ieDword SongList[MAX_RESCOUNT];
	SongHeaderType SongHeader;
	RestHeaderType RestHeader;
	int AreaDifficulty;

	//count of all projectiles that are saved
	size_t GetProjectileCount(proIterator &iter) const;
	//get the next projectile
	Projectile *GetNextProjectile(const proIterator &iter) const;
	//count of unexploded projectiles that are saved
	int GetTrapCount(proIterator &iter) const;
	//get the next saved projectile
	const Projectile *GetNextTrap(proIterator &iter) const;
	//add a projectile to the area
	void AddProjectile(Projectile *pro, const Point &source, ieDword actorID, bool fake);
	void AddProjectile(Projectile* pro, const Point &source, const Point &dest);

	//returns the duration of a VVC cell set in the area (point may be set to empty)
	ieDword HasVVCCell(const ResRef &resource, const Point &p) const;
	void AddVVCell(VEFObject* vvc);
	bool CanFree();
	int GetCursor(const Point &p) const;
	//adds a sparkle puff of colour to a point in the area
	//FragAnimID is an optional avatar animation ID (see avatars.2da) for
	//fragment animation
	void Sparkle(ieDword duration, ieDword color, ieDword type, const Point &pos, unsigned int FragAnimID = 0, int Zpos = 0);
	//removes or fades the sparkle puff at a point
	void FadeSparkle(const Point &pos, bool forced) const;

	//entrances
	void AddEntrance(const char* Name, const Point &, short Face);
	Entrance *GetEntrance(const char *Name) const;
	Entrance *GetEntrance(int i) const { return entrances[i]; }
	int GetEntranceCount() const { return (int) entrances.size(); }

	//containers
	/* this function returns/creates a pile container at position */
	Container* AddContainer(const char* Name, unsigned short Type,
							const std::shared_ptr<Gem_Polygon>& outline);
	Container *GetPile(Point position);
	void AddItemToLocation(const Point &position, CREItem *item);

	Size GetSize() const;
	void FillExplored(bool explored);
	/* set one fog tile as visible. x, y are tile coordinates */
	void ExploreTile(const Point&);
	/* explore map from given point in map coordinates */
	void ExploreMapChunk(const Point &Pos, int range, int los);
	/* block or unblock searchmap with value */
	void BlockSearchMap(const Point& Pos, unsigned int size, PathMapFlags value) const;
	void ClearSearchMapFor(const Movable *actor);
	/* update VisibleBitmap by resolving vision of all explore actors */
	void UpdateFog();
	//PathFinder
	/* Finds the nearest passable point */
	void AdjustPosition(Point &goal, int radiusx = 0, int radiusy = 0, int size = -1) const;
	void AdjustPositionNavmap(Point &goal, int radiusx = 0, int radiusy = 0) const;
	/* Finds the path which leads the farthest from d */
	PathNode* RunAway(const Point &s, const Point &d, unsigned int size, int maxPathLength, bool backAway, const Actor *caller) const;
	PathNode* RandomWalk(const Point &s, int size, int radius, const Actor *caller) const;
	/* Returns true if there is no path to d */
	bool TargetUnreachable(const Point &s, const Point &d, unsigned int size, bool actorsAreBlocking = false) const;
	/* returns true if there is enemy visible */
	bool AnyPCSeesEnemy() const;
	/* Finds straight path from s, length l and orientation o, f=1 passes wall, f=2 rebounds from wall*/
	PathNode* GetLine(const Point &start, const Point &dest, int flags) const;
	PathNode* GetLine(const Point &start, int steps, unsigned int orient) const;
	PathNode* GetLine(const Point &start, int Steps, int Orientation, int flags) const;
	PathNode* GetLine(const Point &start, const Point &dest, int speed, int Orientation, int flags) const;
	/* Finds the path which leads to near d */
	PathNode* FindPath(const Point &s, const Point &d, unsigned int size, unsigned int minDistance = 0, int flags = PF_SIGHT, const Actor *caller = NULL) const;

	bool IsVisible(const Point &p) const;
	bool IsExplored(const Point &p) const;
	bool IsVisibleLOS(const Point &s, const Point &d, const Actor *caller = NULL) const;
	bool IsWalkableTo(const Point &s, const Point &d, bool actorsAreBlocking, const Actor *caller) const;

	/* returns edge direction of map boundary, only worldmap regions */
	WMPDirection WhichEdge(const Point &s) const;

	//ambients
	void AddAmbient(Ambient *ambient) { ambients.push_back(ambient); }
	void SetupAmbients() const;
	Ambient *GetAmbient(int i) const { return ambients[i]; }
	ieWord GetAmbientCount(bool toSave = false) const;

	//mapnotes
	void AddMapNote(const Point &point, ieWord color, const String &text, bool readonly = false);
	void AddMapNote(const Point &point, ieWord color, ieStrRef strref, bool readonly = false);
	void AddMapNote(const Point &point, const MapNote& note);
	void RemoveMapNote(const Point &point);
	const MapNote &GetMapNote(int i) const { return mapnotes[i]; }
	const MapNote *MapNoteAtPoint(const Point &point, unsigned int radius) const;
	unsigned int GetMapNoteCount() const { return (unsigned int) mapnotes.size(); }
	//restheader
	/* May spawn creature(s), returns the remaining number of (unrested) hours for interrupted rest */
	int CheckRestInterruptsAndPassTime(const Point &pos, int hours, int day);
	/* Spawns creature(s) in radius of position */
	bool SpawnCreature(const Point &pos, const char *creResRef, int radiusx = 0, int radiusy = 0, ieWord rwdist = 0, int *difficulty = NULL, unsigned int *creCount = NULL);

	//spawns
	void LoadIniSpawn();
	Spawn *AddSpawn(const char* Name, const Point &, std::vector<ResRef>&& creatures);
	Spawn *GetSpawn(int i) const { return spawns[i]; }
	//returns spawn by name
	Spawn *GetSpawn(const char *Name) const;
	//returns spawn inside circle, checks for schedule and other
	//conditions as well
	Spawn *GetSpawnRadius(const Point &point, unsigned int radius) const;
	unsigned int GetSpawnCount() const { return (unsigned int) spawns.size(); }
	void TriggerSpawn(Spawn *spawn);

	//move some or all players to a new area
	void MoveToNewArea(const ResRef &area, const char *entrance, unsigned int direction, int EveryOne, Actor *actor) const;
	bool HasWeather() const;
	int GetWeather() const;
	void ClearTrap(Actor *actor, ieDword InTrap) const;

	//tracking stuff
	void SetTrackString(ieStrRef strref, int flg, int difficulty);
	//returns true if tracking failed
	bool DisplayTrackString(const Actor *actor) const;

	unsigned int GetLightLevel(const Point &Pos) const;
	PathMapFlags GetInternalSearchMap(const Point&) const;
	void SetInternalSearchMap(const Point&, PathMapFlags value) const;
	void SetBackground(const ResRef &bgResref, ieDword duration);

private:
	const AreaAnimation *GetNextAreaAnimation(aniIterator &iter, ieDword gametime) const;
	Particles *GetNextSpark(const spaIterator &iter) const;
	VEFObject *GetNextScriptedAnimation(const scaIterator &iter) const;
	Actor *GetNextActor(int &q, int &index) const;
	Container *GetNextPile (int &index) const;
	
	void RedrawScreenStencil(const Region& vp, const WallPolygonGroup& walls);
	void DrawStencil(const VideoBufferPtr& stencilBuffer, const Region& vp, const WallPolygonGroup& walls) const;
	WallPolygonSet WallsIntersectingRegion(Region, bool includeDisabled = false, const Point* loc = nullptr) const;
	
	void SetDrawingStencilForObject(const void*, const Region&, const WallPolygonSet&, const Point& viewPortOrigin);
	BlitFlags SetDrawingStencilForScriptable(const Scriptable*, const Region& viewPort);
	BlitFlags SetDrawingStencilForAreaAnimation(const AreaAnimation*, const Region& viewPort);
	
	void DrawDebugOverlay(const Region &vp, uint32_t dFlags) const;
	void DrawPortal(const InfoPoint *ip, int enable);
	void DrawHighlightables(const Region& viewport) const;
	void DrawFogOfWar(const Bitmap* explored_mask, const Bitmap* visible_mask, const Region& viewport) const;
	
	Size PropsSize() const noexcept;
	Size FogMapSize() const;
	bool FogTileUncovered(const Point &p, const Bitmap*) const;
	Point ConvertPointToFog(const Point &p) const;
	
	void GenerateQueues();
	void SortQueues() const;
	//Actor* GetRoot(int priority, int &index);
	void DeleteActor(int i);
	//actor uses travel region
	void UseExit(Actor *pc, InfoPoint *ip);
	//separated position adjustment, so their order could be randomised
	bool AdjustPositionX(Point &goal, int radiusx, int radiusy, int size = -1) const;
	bool AdjustPositionY(Point &goal, int radiusx, int radiusy, int size = -1) const;
	
	void UpdateSpawns() const;
	PathMapFlags QuerySearchMap(const Point&) const;
	PathMapFlags GetBlockedInLine(const Point &s, const Point &d, bool stopOnImpassable, const Actor *caller = NULL) const;

};

}

#endif
