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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id$
 *
 */

/**
 * @file Projectile.h
 * Declares Projectile, class for supporting functionality of spell/item projectiles
 * @author The GemRB Project
 */


#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "../../includes/ie_types.h"
#include "PathFinder.h"
#include "CharAnimations.h"  //contains MAX_ORIENT
#include "Map.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

//projectile phases
#define P_UNINITED  -1
#define P_TRAVEL    0       //projectile moves to target
#define P_TRIGGER   1       //projectile hovers over target, waits for trigger
#define P_EXPLODING 2       //projectile explosion spreads
#define P_EXPLODED  3       //projectile spread over area
#define P_EXPIRED   99      //projectile scheduled for removal (existing parts are still drawn)

//projectile spark flags
#define PSF_SPARKS  1
#define PSF_FLYING  2  
#define PSF_LOOPING 4  //looping sound
#define PSF_IGNORE_CENTER 16

typedef struct ProjectileExtension
{
	ieDword AFlags;
	ieWord TriggerRadius;
	ieWord ExplosionRadius;
	ieResRef SoundRes;
	ieWord Delay;
	ieWord FragAnimID;
	ieWord FragProjIdx;
	ieByte ExplosionCount;
	ieByte ExplType;
	ieWord ExplColor;
	ieWord ExplProjIdx;
	ieResRef VVCRes;
	ieWord ConeWidth;
	//there could be some more unused bytes we don't load
} ProjectileExtension;

class GEM_EXPORT Projectile  
{
public:
	Projectile();
	~Projectile();
	void InitExtension();

	ieWord Type;
	ieWord Speed;
	ieDword SFlags;
	ieResRef SoundRes1;
	ieResRef SoundRes2;
	ieResRef SoundRes3;
	ieDword SparkColor;
	////// gap
	ieDword TFlags;
	ieResRef BAMRes1;
	ieResRef BAMRes2;
	ieByte Seq1, Seq2;
	ieWord LightX;
	ieWord LightY;
	ieWord LightZ;
	ieResRef Palette;
	ieByte Gradients[7];
	ieByte SmokeSpeed;
	ieByte SmokeGrad[7];
	ieByte SmokeAim;
	ieWord SmokeAnimID;
	ieResRef TrailBAM[3];
	ieWord TrailSpeed[3];
	//
	ProjectileExtension *Extension;
	//internals
protected:
	//attributes from moveable object
	unsigned char Orientation, NewOrientation;
	PathNode* path; //whole path
	PathNode* step; //actual step
	//similar to normal actors
	Map *area;
	Point Pos;
	Point Destination;
	ieDword Target; //the globalID of target actor
	ieDword timeStartStep;
	int phase;

	//special (not using char animations)
	Animation* travel[MAX_ORIENT];
	Animation* shadow[MAX_ORIENT];
	Sprite2D* light;//this is just a round/halftrans sprite, has no animation
	Animation** fragments;
public:
	PathNode *GetNextStep(int x);
	int GetPathLength();
//inliners to protect data consistency
	inline PathNode * GetNextStep() {
		if (!step) {
			DoStep((unsigned int) ~0);
		}
		return step;
	}

	inline unsigned char GetOrientation() const {
		return Orientation;
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
	void ChangePhase();
	void DoStep(unsigned int walk_speed);
	void AddWayPoint(Point &Des);
	void RunAwayFrom(Point &Des, int PathLength, int flags);
	void MoveLine(int steps, int Pass, ieDword Orient);
	void WalkTo(Point &Des, int MinDistance = 0);
	void MoveTo(Point &Des);
	void ClearPath();
	void Draw(Region &screen);
private:
	void CheckTrigger(unsigned int radius);
	void DrawTravel(Region &screen);
	void DrawExplosion(Region &screen);
	void DrawExploded(Region &screen);
};

#endif // PROJECTILE_H

