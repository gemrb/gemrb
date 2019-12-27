/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

/**
 * @file Core.cpp
 * Some compatibility and utility functions
 * @author The GemRB Project
 */

#include "globals.h"
#include "exports.h"

#include "Interface.h"
#include "Scriptable/Actor.h"

#include <cmath>
#include <ctype.h>

#if defined(__sgi)
#include <iostream>
#endif

#ifdef WIN32
#include "win32def.h"
#ifdef _DEBUG
#include <stdlib.h>
#include <crtdbg.h>
#endif

BOOL WINAPI DllEntryPoint(HINSTANCE, DWORD, LPVOID);
BOOL WINAPI DllEntryPoint(HINSTANCE /*hinstDLL*/, DWORD /*fdwReason*/,
	LPVOID /*lpvReserved*/)
{
	return true;
}

#endif

namespace GemRB {

//// Globally used functions

static const unsigned char orientations[25]={
6,7,8,9,10,
5,6,8,10,11,
4,4,0,12,12,
3,2,0,14,13,
2,1,0,15,14
};

/** Calculates the orientation of a character (or projectile) facing a point */
unsigned char GetOrient(const Point &s, const Point &d)
{
	int deltaX = s.x - d.x;
	int deltaY = s.y - d.y;
	int div = Distance(s,d);
	if(!div) return 0; //default
	if(div>3) div/=2;
	int aX=deltaX/div;
	int aY=deltaY/div;
	return orientations[(aY+2)*5+aX+2];
}

/** Calculates distance between 2 points */
unsigned int Distance(Point p, Point q)
{
	long x = ( p.x - q.x );
	long y = ( p.y - q.y );
	return (unsigned int) std::sqrt( ( double ) ( x* x + y* y ) );
}

/** Calculates distance squared from a point to a scriptable */
unsigned int SquaredMapDistance(Point p, const Scriptable *b)
{
	long x = ( p.x/16 - b->Pos.x/16 );
	long y = ( p.y/12 - b->Pos.y/12 );
	return (unsigned int)(x*x + y*y);
}

/** Calculates distance between 2 points */
unsigned int Distance(Point p, const Scriptable *b)
{
	long x = ( p.x - b->Pos.x );
	long y = ( p.y - b->Pos.y );
	return (unsigned int) std::sqrt( ( double ) ( x* x + y* y ) );
}

unsigned int PersonalDistance(Point p, const Scriptable *b)
{
	long x = ( p.x - b->Pos.x );
	long y = ( p.y - b->Pos.y );
	int ret = (int) std::sqrt( ( double ) ( x* x + y* y ) );
	if (b->Type==ST_ACTOR) {
		ret-=((Actor *)b)->size*10;
	}
	if (ret<0) return (unsigned int) 0;
	return (unsigned int) ret;
}

unsigned int SquaredPersonalDistance(Point p, const Scriptable *b)
{
	long x = ( p.x - b->Pos.x );
	long y = ( p.y - b->Pos.y );
	int ret =  x*x + y*y;
	if (b->Type==ST_ACTOR) {
		ret-=((Actor *)b)->size*100;
	}
	if (ret<0) return (unsigned int) 0;
	return (unsigned int) ret;
}

/** Calculates map distance between 2 scriptables */
unsigned int SquaredMapDistance(const Scriptable *a, const Scriptable *b)
{
	long x = (a->Pos.x/16 - b->Pos.x/16 );
	long y = (a->Pos.y/12 - b->Pos.y/12 );
	return (unsigned int)(x*x + y*y);
}

/** Calculates distance between 2 scriptables */
unsigned int Distance(const Scriptable *a, const Scriptable *b)
{
	long x = ( a->Pos.x - b->Pos.x );
	long y = ( a->Pos.y - b->Pos.y );
	return (unsigned int) std::sqrt( ( double ) ( x* x + y* y ) );
}

/** Calculates distance squared between 2 scriptables */
unsigned int SquaredDistance(const Scriptable *a, const Scriptable *b)
{
	long x = ( a->Pos.x - b->Pos.x );
	long y = ( a->Pos.y - b->Pos.y );
	return (unsigned int) ( x* x + y* y );
}

/** Calculates distance between 2 scriptables, including feet circle if applicable */
unsigned int PersonalDistance(const Scriptable *a, const Scriptable *b)
{
	long x = ( a->Pos.x - b->Pos.x );
	long y = ( a->Pos.y - b->Pos.y );
	int ret = (int) std::sqrt( ( double ) ( x* x + y* y ) );
	if (a->Type==ST_ACTOR) {
		ret-=((Actor *)a)->size*10;
	}
	if (b->Type==ST_ACTOR) {
		ret-=((Actor *)b)->size*10;
	}
	if (ret<0) return (unsigned int) 0;
	return (unsigned int) ret;
}

unsigned int SquaredPersonalDistance(const Scriptable *a, const Scriptable *b)
{
	long x = ( a->Pos.x - b->Pos.x );
	long y = ( a->Pos.y - b->Pos.y );
	int ret =  x*x + y*y;
	if (a->Type==ST_ACTOR) {
		ret-=((Actor *)a)->size*100;
	}
	if (b->Type==ST_ACTOR) {
		ret-=((Actor *)b)->size*100;
	}
	if (ret<0) return (unsigned int) 0;
	return (unsigned int) ret;
}

// What's the deal with the spell and item ranges in their descriptions being in feet?
//
// Kjeron provided these notes:
// It had no standards - spell descriptions were copied straight from their source material.
// In game it ranged from 3.57 units per foot, to 28.57 units per foot, but most often around
// 8.5 units per foot, which was still wrong. The descriptions have mostly been fixed in the
// EE's, albeit with some rounding.
//
// 16 horizontal pixels = 1 foot
// 12 vertical pixels = 1 foot
//
// Projectile trap/explosion size is based on horizontal distance, 16 = 16h pixels = 1 foot radius
// 80 = 5' radius
// 256 = 16' radius
// 448 = 28' radius (max engine can handle for repeating AoE's)
// 480 = 30' radius
//
// Visual/Script Range = 448h/336v pixels (28 feet) (I don't know why it's often listed as 30')
// Spellcasting Range = 16h/12v pixels (1 foot) per unit
// Opcode 262 (Visual Range) = 32h/24v pixels (2 feet) per unit
//
// The one thing that is 30' (480h/360v pixels) is PC movement rate per round, at least in original BG1,
// BG2 bumped it up by 50% to 45' per round.

// 1 foot = 16px horizontally, 12px vertically and all in between
// we use them to construct an ellipse and through its central angle get the adjusted "radius"
// Potential optimisation through precomputing:
// rounding the angle into 5° intervals [0-90] gives these values per foot:
// 16 16 16 16 15 15 15 14 14 14 13 13 13 12 12 12 12 12 12
double Feet2Pixels(int feet, double angle)
{
	double sin2 = pow(sin(angle) / 12, 2);
	double cos2 = pow(cos(angle) / 16, 2);
	double r = sqrt(1 / (cos2 + sin2));
	return r * feet;
}

/* Audible range was confirmed to be 3x visual range in EEs, accounting for isometric scaling
 a nd more than 1x visual range in others;
 in EE, the '48' (3*16) default can be set by the 'Audible Range' option in baldur.lua

 This is a bit tricky, it has been show to not be very consistent. The game used a double value of visual
 range in several places, so we will use '3 * visual_range / 2' */
bool WithinAudibleRange(const Actor *actor, const Point &dest)
{
	int distance = (3 * actor->GetStat(IE_VISUALRANGE)) / 2;
	return WithinRange(actor, dest, distance);
}

bool WithinRange(const Actor *actor, const Point &dest, int distance)
{
	double angle = atan2(actor->Pos.y - dest.y, actor->Pos.x - dest.x);
	return Distance(dest, actor) <= Feet2Pixels(distance, angle);
}

bool WithinPersonalRange(const Actor *actor, const Scriptable *dest, int distance)
{
	double angle = atan2(actor->Pos.y - dest->Pos.y, actor->Pos.x - dest->Pos.x);
	return PersonalDistance(dest, actor) <= Feet2Pixels(distance, angle);
}

// returns EA relation between two scriptables (non actors are always enemies)
// it is used for protectile targeting/iwd ids targeting too!
int EARelation(const Scriptable* Owner, const Actor* target)
{
	ieDword eao = EA_ENEMY;

	if (Owner && Owner->Type==ST_ACTOR) {
		eao = ((Actor *) Owner)->GetStat(IE_EA);
	}

	ieDword eat = target->GetStat(IE_EA);

	if (eao<=EA_GOODCUTOFF) {
		
		if (eat<=EA_GOODCUTOFF) {
			return EAR_FRIEND;
		}
		if (eat>=EA_EVILCUTOFF) {
			return EAR_HOSTILE;
		}

		return EAR_NEUTRAL;
	}

	if (eao>=EA_EVILCUTOFF) {

		if (eat<=EA_GOODCUTOFF) {
			return EAR_HOSTILE;
		}
		if (eat>=EA_EVILCUTOFF) {
			return EAR_FRIEND;
		}

		return EAR_NEUTRAL;
	}

	return EAR_NEUTRAL;
}

// check whether the game time matches a given schedule
// hours are offset by half an hour, ie. bit 0 covers the
// time from 0030 to 0129, bit 1 from 0130 to 0229 etc.
bool Schedule(ieDword schedule, ieDword time)
{
	return (schedule & SCHEDULE_MASK(time)) != 0;
}

// safely copies a ResRef (ie. nulls out the unused buffer size)
void CopyResRef(ieResRef d, const ieResRef s)
{
	strncpy(d, s, sizeof(ieResRef) - 1);
	d[sizeof(ieResRef) - 1] = '\0';
}

}
