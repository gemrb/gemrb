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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Projectile.h,v 1.1 2006/09/16 13:30:15 avenger_teambg Exp $
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

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

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
};

#endif // PROJECTILE_H

