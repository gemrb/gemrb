/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2006 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

/**
 * @file Projectile.h
 * Declares Projectile, class for supporting functionality of spell/item projectiles
 * @author The GemRB Project
 */


#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "exports.h"
#include "ie_types.h"

#include "Orientation.h"
#include "EffectQueue.h"
#include "Map.h"
#include "Palette.h"
#include "PathFinder.h"
#include "Audio.h"
#include "Video/Video.h"

#include <list>

namespace GemRB {

// various special heights/Zs hardcoded in the originals
enum ProHeights {
	None = 0, // pst casting glows
	Flying = 50, // this is the height of the projectile when Spark Flag Fly = 1
	Normal = 0x23,
	Dragon = 0x90,
	Background = 50 // this is supposed to move the projectile to the background
};

#define BACK_DEPTH 50

//projectile phases
#define P_UNINITED  -1
#define P_TRAVEL     0   //projectile moves to target
#define P_TRAVEL2    1   //projectile hit target
#define P_TRIGGER    2   //projectile hovers over target, waits for trigger
#define P_EXPLODING1 3   //projectile explosion spreads
#define P_EXPLODING2 4   //projectile explosion repeats
#define P_EXPLODED   5   //projectile spread over area
#define P_EXPIRED   99   //projectile scheduled for removal (existing parts are still drawn)

//projectile spark flags
#define PSF_SPARKS  1
#define PSF_FLYING  2
#define PSF_LOOPING 4         //looping sound
#define PSF_LOOPING2 8        //looping second sound
#define PSF_IGNORE_CENTER 16
#define PSF_BACKGROUND 32
//gemrb specific internal flag
#define PSF_SOUND2  0x80000000//already started sound2

//projectile travel flags
#define PTF_COLOUR  1       //fake colours
#define PTF_SMOKE   2       //has smoke
// bg2: 4 smoke is false colored
#define PTF_TINT    8       //tint projectile
// bg2: 10 height
#define PTF_SHADOW  32      //has shadow bam
#define PTF_LIGHT   64      //has light shadow / glow
#define PTF_TRANS   128     // glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE);
#define PTF_BRIGHTEN 256    //brighten alpha; CPROJECTILEBAMFILEFORMAT_FLAGS_BRIGHTEST in bg2
#define PTF_BLEND	512		// glBlendFunc(GL_DST_COLOR, GL_ONE);
#define PTF_TRANS_BLEND (PTF_TRANS | PTF_BLEND) // glBlendFunc(GL_SRC_COLOR, GL_ONE); IWD only?
// 0x100 and 0x200: FLAGS_BRIGHTESTIFFAST BRIGHTEST3DONLYOFF
#define PTF_TIMELESS 0x4000 // GemRB extension to differentiate projectiles that ignore timestop

//projectile extended travel flags (gemrb specific)
#define PEF_BOUNCE     1       //bounce from walls (lightning bolt)
#define PEF_CONTINUE   2       //continue as a travel projectile after trigger (lightning bolt)
//TODO: This can probably be replaced by an area projectile trigger (like skull trap, glyph)
#define PEF_FREEZE     4       //stay around (ice dagger)
#define PEF_NO_TRAVEL  8       //all instant projectiles (draw upon holy might, finger of death)
#define PEF_TRAIL      16      //trail bams facing value uses the same field as the travel projectile (otherwise it defaults to 9) (shout in iwd)
#define PEF_CURVE      32      //curved path (magic missile)
#define PEF_RANDOM     64      //random starting frame for animation (?)
#define PEF_PILLAR     128     //draw all cycles simultaneously on top of each other (call lightning, flamestrike)
#define PEF_HALFTRANS  256     //half-transparency (holy might)
#define PEF_TINT       512     //use palette gradient as tint
#define PEF_ITERATION  1024    //create another projectile of type-1 (magic missiles)
#define PEF_DEFSPELL   2048    //always apply the default spell on the caster
#define PEF_FALLING    4096    //projectile falls down vertically (cow)
#define PEF_INCOMING   8192    //projectile falls in on trajectory (comet)
#define PEF_LINE       16384   //solid line between source and target (agannazar's scorcher)
#define PEF_WALL       32768   //solid line in front of source, crossing target (wall of fire)
#define PEF_BACKGROUND 0x10000 //draw under target,overrides flying (dimension door)
#define PEF_POP        0x20000 //draw travel bam, then shadow, then travel bam backwards
#define PEF_UNPOP      0x40000 //draw shadow, then travel bam (this is an internal flag)
//TODO: The next flag is probably not needed, it is done by a separate area hit animation
#define PEF_FADE       0x80000 //gradually fade on spot if used with PEF_FREEZE (ice dagger) 
#define PEF_TEXT       0x100000//display text during setup
#define PEF_WANDERING  0x200000//random movement (no real path)
#define PEF_CYCLE      0x400000//random cycle
#define PEF_RGB        0x800000//rgb pulse on hit
#define PEF_TOUCH      0x1000000//successful to hit roll needed
#define PEF_NOTIDS     0x2000000//negate IDS check
#define PEF_NOTIDS2    0x4000000//negate secondary IDS check
#define PEF_BOTH       0x8000000//both IDS check must succeed
#define PEF_DELAY      0x10000000//delay payload until travel projectile cycle ends

//projectile area flags
#define PAF_VISIBLE    1      //the travel projectile is visible until explosion; CPROJECTILEAREAFILEFORMAT_FLAGS_CENTERBAM
#define PAF_INANIMATE  2      //target inanimates
#define PAF_TRIGGER    4      //explosion needs to be triggered
#define PAF_SYNC       8      //one explosion at a time
#define PAF_SECONDARY  16     //secondary projectiles at explosion
#define PAF_FRAGMENT   32     //fragments (charanimation) at explosion
#define PAF_ENEMY      64     //target party or not party
#define PAF_PARTY      128    //target party
#define PAF_TARGET     (64|128)
#define PAF_LEV_MAGE   256
#define PAF_LEV_CLERIC 512
#define PAF_VVC        1024   // played at center
#define PAF_CONE       2048   //enable cone shape
#define PAF_NO_WALL    0x1000 //pass through walls; CPROJECTILEAREAFILEFORMAT_FLAGS_IGNORELOS
#define PAF_TRIGGER_D  0x2000 //delayed trigger (only if animation is over 30); CPROJECTILEAREAFILEFORMAT_FLAGS_CENTERBAMWAIT
#define PAF_DELAY      0x4000 // CPROJECTILEAREAFILEFORMAT_FLAGS_FORCEINITIALDELAY
#define PAF_AFFECT_ONE 0x8000 // CPROJECTILEAREAFILEFORMAT_FLAGS_ONETARGETONLY

//area projectile flags (in areapro.2da)
//this functionality was hardcoded in the original engine, so the bit flags are
//completely arbitrary (i assign them as need arises)
//child projectiles need to be tinted (example: stinking cloud, counter example: fireball)
#define APF_TINT      1
//child projectiles fill the whole area (example: stinking cloud, counter example: fireball)
#define APF_FILL      2
//child projectiles start in their destination (example: icestorm, counter example: fireball)
#define APF_SCATTER   4
//the explosion vvc has gradient (example: icestorm, counter example: fireball)
#define APF_VVCPAL    8
//there is an additional added scatter after the initial spreading ring
#define APF_SPREAD    16
//the spread projectile needs gradient colouring,not tint (example:web, counter example: stinking cloud)
#define APF_PALETTE   32
//use both animations in the spread
#define APF_BOTH      64
//more child projectiles
#define APF_MORE      128
//apply spell on caster if failed to find target
#define APF_SPELLFAIL 256
//multiple directions 
#define APF_MULTIDIR  512
//target HD counting
#define APF_COUNT_HD  1024
//target flag enemy ally switched
#define APF_INVERT_TARGET 2048
//tiled AoE animation
#define APF_TILED 4096
#define APF_PLAYONCE 8192

struct ProjectileExtension
{
	ieDword AFlags;
	ieWord TriggerRadius;
	ieWord ExplosionRadius;
	ResRef SoundRes; //used for areapro.2da explosion sound
	ieWord Delay;
	ieWord FragAnimID;
	ieWord FragProjIdx;
	ieByte ExplosionCount;
	ieByte ExplType;
	ieWord ExplColor; // a byte in the original, followed by padding
	ieWord ExplProjIdx;
	ResRef VVCRes;  //used for areapro.2da second resref (center animation)
	ieWord ConeWidth;
	//these are GemRB specific (from areapro.2da)
	ieDword APFlags;    //areapro.2da flags
	ResRef Spread;    //areapro.2da first resref
	ResRef Secondary; //areapro.2da third resref
	ResRef AreaSound; //areapro.2da second sound resource
	//used for target or HD counting
	ieWord DiceCount;
	ieWord DiceSize;
	Point tileCoord;
};

class GEM_EXPORT Projectile
{
public:
	Projectile() noexcept;
#if _MSC_VER
	// GCC doesnt like it if these are defaulted
	// MSVC doesnt like it if they are not
	~Projectile() noexcept = default;

	Projectile(const Projectile&) noexcept = default;
	Projectile(Projectile&&) noexcept = default;

	Projectile& operator=(const Projectile&) noexcept = default;
	Projectile& operator=(Projectile&&) noexcept = default;
#endif

	ieWord Speed = 20; // (horizontal) pixels / tick
	ieDword SFlags = PSF_FLYING;
	ResRef FiringSound;
	ResRef ArrivalSound;
	ResRef TravelVVC;
	ieDword SparkColor = 0;
	ieDword ExtFlags = 0;
	ieStrRef StrRef = ieStrRef::INVALID;
	Color RGB;
	ieWord ColorSpeed = 0;
	ieWord Shake = 0;
	ieWord IDSType = 0;
	ieWord IDSValue = 0;
	ieWord IDSType2 = 0;
	ieWord IDSValue2 = 0;
	ResRef failureSpell;
	ResRef successSpell;
	////// gap
	ieDword TFlags = 0;
	ResRef BAMRes1;
	ResRef BAMRes2;
	ieByte Seq1 = 0;
	ieByte Seq2 = 0;
	ieWord LightX = 0;
	ieWord LightY = 0;
	ieWord LightZ = 0;
	ResRef PaletteRes;
	ieByte Gradients[7]{};
	ieByte SmokeSpeed = 0;
	ieByte SmokeGrad[7]{};
	ieByte Aim = 0; // original bg2: m_numDirections, a list of {1, 5, 9}
	ieWord SmokeAnimID = 0;
	ResRef TrailBAM[3];
	ieWord TrailSpeed[3]{};
	unsigned int Range = 0;
	//these are public but not in the .pro file
	Holder<ProjectileExtension> Extension;
	Holder<Palette> palette = nullptr;
	//internals
private:
	ResRef smokebam;
	tick_t timeStartStep = 0;
	//attributes from moveable object
	orient_t Orientation = S;
	orient_t NewOrientation = S;
	Path path; // whole path
	size_t stepIdx = 0; // actual step in path
	//similar to normal actors
	Map *area = nullptr;
	Point Pos = Point(-1, -1);
	int ZPos = 0;
	Point Destination = Pos;
	Point Origin;
	ieDword Caster = 0;    // the globalID of the caster actor
	int Level = 0;         // the caster's level
	ieDword Target = 0;    // the globalID of target actor
	ieDword FakeTarget = 0; // a globalID for target that isn't followed
	int phase = P_UNINITED;
	//saved in area
	ResRef projectileName; // used also for namesake externalized spells
	ieWord type = 0;
	//these come from the extension area
	int extensionDelay = 0;
	int extensionExplosionCount = 0;
	int extensionTargetCount = 0;
	Color tint;

	// special (not using char animations)
	// using std::vector over std::array for better movability
	// the array will always be MAX_ORIENT in size
	using AnimArray = std::vector<Animation>;
	AnimArray travel;
	AnimArray shadow;

	Holder<Sprite2D> light = nullptr; // this is just a round/halftrans sprite, has no animation
	EffectQueue effects;
	std::list<Projectile> children;
	std::vector<Point> childLocations; // for persistence across child generations
	int pathcounter = 0x7fff;
	int bend = 0;
	int drawSpark = 0;
	
	struct LoopStop {
		Holder<SoundHandle> sound;
		
		~LoopStop() noexcept {
			if (sound) {
				//allow an explosion sound to finish completely
				sound->StopLooping();
			}
		}
		
		LoopStop() noexcept = default;
		
		LoopStop(const LoopStop& ls) noexcept
		: sound(ls.sound) {}
		
		LoopStop(LoopStop&& ls) noexcept
		{
			std::swap(sound, ls.sound);
		}
		
		LoopStop& operator=(const LoopStop& ls) noexcept {
			if (this != &ls) {
				sound = ls.sound;
			}
			return *this;
		}
		
		LoopStop& operator=(LoopStop&& ls) noexcept {
			if (this != &ls) {
				std::swap(sound, ls.sound);
			}
			return *this;
		}
		
		SoundHandle* operator->() noexcept {
			return sound.get();
		}
		
		explicit operator bool() const noexcept {
			return bool(sound);
		}
	} travel_handle;
public:
	void SetCaster(ieDword t, int level);
	ieDword GetCaster() const;
	bool FailedIDS(const Actor *target) const;
	void SetTarget(ieDword t, bool fake);
	void SetTarget(const Point &p);
	bool PointInRadius(const Point &p) const;
	int GetPhase() const;
	void Cleanup();

	inline Point GetDestination() const { return Destination; }
	inline const ResRef& GetName() const { return projectileName; }
	inline ieWord GetType() const { return type; }
	//This assumes that the effect queue cannot be bigger than 65535
	//which is a sane expectation
	EffectQueue& GetEffects() {
		return effects;
	}
	
	const EffectQueue& GetEffects() const {
		return effects;
	}

	inline orient_t GetOrientation() const {
		return Orientation;
	}
	//no idea if projectiles got height, using y
	inline int GetHeight() const {
		//if projectile is drawn absolutely on the ground
		if (SFlags&PSF_BACKGROUND) {
			return 0;
		}
		//if projectile is drawn behind target (not behind everyone)
		if (ExtFlags&PEF_BACKGROUND) {
			return Pos.y - ProHeights::Background;
		}

		return Pos.y + ZPos;
	}

	void SetIdentifiers(const ResRef &name, size_t idx);

	void SetEffectsCopy(const EffectQueue& eq, const Point &source);

	//don't forget to set effects to NULL when the projectile discharges
	//unexploded projectiles are responsible to destruct their payload

	inline void SetEffects(EffectQueue&& fx) {
		effects = std::move(fx);
	}

	inline unsigned char GetNextFace() const {
		return GemRB::GetNextFace(Orientation, NewOrientation);
	}

	inline void SetOrientation(orient_t value, bool slow) {
		//MAX_ORIENT == 16, so we can do this
		NewOrientation = value;
		if (!slow) {
			Orientation = NewOrientation;
		}
	}

	void Setup();
	//sets how long a created travel projectile will hover over a spot
	//before vanishing (without the need of area extension)
	void SetDelay(int delay);
	void MoveTo(Map *map, const Point &Des);
	void ClearPath();
	//handle phases, return 0 when expired
	int Update();
	//draw object
	void Draw(const Region &screen);
	void SetGradient(int gradient, bool tinted);
	void StaticTint(const Color &newtint);
	static Point GetStartOffset(const Actor* actor);
private:
	//creates a child projectile with current_projectile_id - 1
	void CreateIteration();
	AnimArray CreateAnimations(const ResRef& bam, ieByte seq);
	//pillar type animations
	AnimArray CreateCompositeAnimation(const AnimationFactory& af, ieByte seq) const;
	//oriented animations (also simple ones)
	AnimArray CreateOrientedAnimations(const AnimationFactory& af, ieByte seq) const;
	void GetPaletteCopy(const AnimArray&, Holder<Palette> &pal) const;
	void GetSmokeAnim();
	//apply spells and effects on the target, only in single travel mode
	//area effect projectiles call a separate single travel projectile for each affected target
	void Payload();
	//if there is an extension, convert to exploding or wait for trigger
	void EndTravel();
	void ProcessEffects(EffectQueue& projQueue, Scriptable* owner, Actor* target, bool apply) const;
	//apply default spell
	void ApplyDefault() const;
	//stops the current sound
	void StopSound();
	//kickstarts the secondary sound
	void UpdateSound();
	//reached end of single travel missile, explode or expire now
	void ChangePhase();
	//drop a BAM or VVC on the trail path, return the length of the animation
	int AddTrail(const ResRef& BAM, const ieByte *pal) const;
	void DoStep();
	void LineTarget() const;      //line projectiles (walls, scorchers)
	void LineTarget(Path::const_iterator beg, Path::const_iterator end) const;
	void SecondaryTarget(); //area projectiles (circles, cones)
	void CheckTrigger(unsigned int radius);
	//calculate target and destination points for a firewall
	void SetupWall();
	void DrawLine(const Region &screen, int face, BlitFlags flag);
	void DrawTravel(const Region &screen);
	bool DrawChildren(const Region &screen);
	void DrawExplosion(const Region &screen);
	void SpawnFragment(Point &pos);
	void DrawExploded(const Region &screen);
	int GetTravelPos(int face) const;
	int GetShadowPos(int face) const;
	void SetFrames(int face, int frame1, int frame2);
	inline int GetZPos() const;

	//logic to resolve target when single projectile hit destination
	int CalculateTargetFlag() const;
	//logic to resolve the explosion count (may be based on caster level)
	int CalculateExplosionCount() const;

	Actor *GetTarget();
	void NextTarget(const Point &p);
	void SetupPalette(const AnimArray&, Holder<Palette> &pal, const ieByte *gradients) const;

	void Draw(const Holder<Sprite2D>& spr, const Point& p,
			  BlitFlags flags, Color overrideTint) const;
};

static_assert(std::is_nothrow_move_constructible<Projectile>::value, "Projectile should be noexcept MoveConstructible");

}

#endif // PROJECTILE_H
