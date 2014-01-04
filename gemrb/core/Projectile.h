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

#include "CharAnimations.h" //contains MAX_ORIENT
#include "EffectQueue.h"
#include "Map.h"
#include "Palette.h"
#include "PathFinder.h"
#include "Audio.h"

namespace GemRB {

//this is the height of the projectile when Spark Flag Fly = 1
#define FLY_HEIGHT 50
//this is supposed to move the projectile to the background
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
#define PTF_TINT    8       //tint projectile
#define PTF_SHADOW  32      //has shadow bam
#define PTF_LIGHT   64      //has light shadow
#define PTF_BLEND   128     //blend colours (use alpha)
#define PTF_BRIGHTEN 256    //brighten alpha

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
#define PAF_VISIBLE    1      //the travel projectile is visible until explosion
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
#define PAF_VVC        1024   //
#define PAF_CONE       2048   //enable cone shape
#define PAF_NO_WALL    0x1000 //pass through walls
#define PAF_TRIGGER_D  0x2000 //delayed trigger (only if animation is over 30)
#define PAF_DELAY      0x4000 //
#define PAF_AFFECT_ONE 0x8000 //

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

struct ProjectileExtension
{
	ieDword AFlags;
	ieWord TriggerRadius;
	ieWord ExplosionRadius;
	ieResRef SoundRes; //used for areapro.2da explosion sound
	ieWord Delay;
	ieWord FragAnimID;
	ieWord FragProjIdx;
	ieByte ExplosionCount;
	ieByte ExplType;
	ieWord ExplColor;
	ieWord ExplProjIdx;
	ieResRef VVCRes;  //used for areapro.2da second resref (center animation)
	ieWord ConeWidth;
	//these are GemRB specific (from areapro.2da)
	ieDword APFlags;    //areapro.2da flags
	ieResRef Spread;    //areapro.2da first resref
	ieResRef Secondary; //areapro.2da third resref
	ieResRef AreaSound; //areapro.2da second sound resource
	//used for target or HD counting
	ieWord DiceCount;
	ieWord DiceSize;
	ieWord TileX;
	ieWord TileY;
};

class GEM_EXPORT Projectile
{
public:
	Projectile();
	~Projectile();
	void InitExtension();

	ieWord Speed;
	ieDword SFlags;
	ieResRef SoundRes1;
	ieResRef SoundRes2;
	ieResRef TravelVVC;
	ieDword SparkColor;
	ieDword ExtFlags;
	ieDword StrRef;
	ieDword RGB;
	ieWord ColorSpeed;
	ieWord Shake;
	ieWord IDSType;
	ieWord IDSValue;
	ieWord IDSType2;
	ieWord IDSValue2;
	ieResRef FailSpell;
	ieResRef SuccSpell;
	////// gap
	ieDword TFlags;
	ieResRef BAMRes1;
	ieResRef BAMRes2;
	ieByte Seq1, Seq2;
	ieWord LightX;
	ieWord LightY;
	ieWord LightZ;
	ieResRef PaletteRes;
	ieByte Gradients[7];
	ieByte SmokeSpeed;
	ieByte SmokeGrad[7];
	ieByte Aim;
	ieWord SmokeAnimID;
	ieResRef TrailBAM[3];
	ieWord TrailSpeed[3];
	//these are public but not in the .pro file
	ProjectileExtension* Extension;
	bool autofree;
	Palette* palette;
	//internals
protected:
	ieResRef smokebam;
	ieDword timeStartStep;
	//attributes from moveable object
	unsigned char Orientation, NewOrientation;
	PathNode* path; //whole path
	PathNode* step; //actual step
	//similar to normal actors
	Map *area;
	Point Pos;
	int ZPos;
	Point Destination;
	Point Origin;
	ieDword Caster;    //the globalID of the caster actor
	int Level;         //the caster's level
	ieDword Target;    //the globalID of target actor
	ieDword FakeTarget; //a globalID for target that isn't followed
	int phase;
	//saved in area
	ieResRef name;
	ieWord type;
	//these come from the extension area
	int extension_delay;
	int extension_explosioncount;
	int extension_targetcount;
	Color tint;

	//special (not using char animations)
	Animation* travel[MAX_ORIENT];
	Animation* shadow[MAX_ORIENT];
	Sprite2D* light;//this is just a round/halftrans sprite, has no animation
	EffectQueue* effects;
	Projectile **children;
	int child_size;
	int pathcounter;
	int bend;
	int drawSpark;
	Holder<SoundHandle> travel_handle;
public:
	void SetCaster(ieDword t, int level);
	ieDword GetCaster() const;
	bool FailedIDS(Actor *target) const;
	void SetTarget(ieDword t, bool fake);
	void SetTarget(const Point &p);
	bool PointInRadius(const Point &p) const;
	void Cleanup();

	//inliners to protect data consistency
	inline PathNode * GetNextStep() {
		if (!step) {
			DoStep((unsigned int) ~0);
		}
		return step;
	}

	inline Point GetDestination() const { return Destination; }
	inline const char * GetName() const { return name; }
	inline ieWord GetType() const { return type; }
	//This assumes that the effect queue cannot be bigger than 65535
	//which is a sane expectation
	inline EffectQueue *GetEffects() const {
		return effects;
	}

	inline unsigned char GetOrientation() const {
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
			return Pos.y-BACK_DEPTH;
		}

		//if projectile is flying
		if (SFlags&PSF_FLYING) {
			return Pos.y+FLY_HEIGHT;
		}
		return Pos.y;
	}

	void SetIdentifiers(const char *name, ieWord type);

	void SetEffectsCopy(EffectQueue *eq);

	//don't forget to set effects to NULL when the projectile discharges
	//unexploded projectiles are responsible to destruct their payload

	inline void SetEffects(EffectQueue *fx) {
		effects = fx;
	}

	inline unsigned char GetNextFace() {
		//slow turning
		if (Orientation != NewOrientation) {
			if ( ( (NewOrientation-Orientation) & (MAX_ORIENT-1) ) <= MAX_ORIENT/2) {
				Orientation++;
			} else {
				Orientation--;
			}
			Orientation = Orientation&(MAX_ORIENT-1);
		}

		return Orientation;
	}

	inline void SetOrientation(int value, bool slow) {
		//MAX_ORIENT == 16, so we can do this
		NewOrientation = (unsigned char) (value&(MAX_ORIENT-1));
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
	void SetGradient(int gradient, bool tint);
	void StaticTint(const Color &newtint);
private:
	//creates a child projectile with current_projectile_id - 1
	void CreateIteration();
	void CreateAnimations(Animation **anims, const ieResRef bam, int Seq);
	//pillar type animations
	void CreateCompositeAnimation(Animation **anims, AnimationFactory *af, int Seq);
	//oriented animations (also simple ones)
	void CreateOrientedAnimations(Animation **anims, AnimationFactory *af, int Seq);
	void GetPaletteCopy(Animation *anim[], Palette *&pal);
	void GetSmokeAnim();
	void SetBlend(int brighten);
	//apply spells and effects on the target, only in single travel mode
	//area effect projectiles call a separate single travel projectile for each affected target
	void Payload();
	//if there is an extension, convert to exploding or wait for trigger
	void EndTravel();
	//apply default spell
	void ApplyDefault();
	//stops the current sound
	void StopSound();
	//kickstarts the secondary sound
	void UpdateSound();
	//reached end of single travel missile, explode or expire now
	void ChangePhase();
	//drop a BAM or VVC on the trail path, return the length of the animation
	int AddTrail(ieResRef BAM, const ieByte *pal);
	void DoStep(unsigned int walk_speed);
	void LineTarget();      //line projectiles (walls, scorchers)
	void SecondaryTarget(); //area projectiles (circles, cones)
	void CheckTrigger(unsigned int radius);
	//calculate target and destination points for a firewall
	void SetupWall();
	void DrawLine(const Region &screen, int face, ieDword flag);
	void DrawTravel(const Region &screen);
	bool DrawChildren(const Region &screen);
	void DrawExplosion(const Region &screen);
	void SpawnFragment(Point &pos);
	void DrawExploded(const Region &screen);
	int GetTravelPos(int face) const;
	int GetShadowPos(int face) const;
	void SetPos(int face, int frame1, int frame2);
	inline int GetZPos() const;

	//logic to resolve target when single projectile hit destination
	int CalculateTargetFlag();
	//logic to resolve the explosion count (may be based on caster level)
	int CalculateExplosionCount();

	Actor *GetTarget();
	void NextTarget(const Point &p);
	void SetupPalette(Animation *anim[], Palette *&pal, const ieByte *gradients);
};

}

#endif // PROJECTILE_H
