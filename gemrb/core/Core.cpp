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
#ifdef WIN32
#include "win32def.h"
#ifdef _DEBUG
#include <stdlib.h>
#include <crtdbg.h>
#endif

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
	return (unsigned int) sqrt( ( double ) ( x* x + y* y ) );
}

/** Calculates distance squared from a point to a scriptable */
unsigned int SquaredMapDistance(Point p, Scriptable *b)
{
	long x = ( p.x/16 - b->Pos.x/16 );
	long y = ( p.y/12 - b->Pos.y/12 );
	return (unsigned int)(x*x + y*y);
}

/** Calculates distance between 2 points */
unsigned int Distance(Point p, Scriptable *b)
{
	long x = ( p.x - b->Pos.x );
	long y = ( p.y - b->Pos.y );
	return (unsigned int) sqrt( ( double ) ( x* x + y* y ) );
}

unsigned int PersonalDistance(Point p, Scriptable *b)
{
	long x = ( p.x - b->Pos.x );
	long y = ( p.y - b->Pos.y );
	int ret = (int) sqrt( ( double ) ( x* x + y* y ) );
	if (b->Type==ST_ACTOR) {
		ret-=((Actor *)b)->size*10;
	}
	if (ret<0) return (unsigned int) 0;
	return (unsigned int) ret;
}

unsigned int SquaredPersonalDistance(Point p, Scriptable *b)
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
unsigned int SquaredMapDistance(Scriptable *a, Scriptable *b)
{
	long x = (a->Pos.x/16 - b->Pos.x/16 );
	long y = (a->Pos.y/12 - b->Pos.y/12 );
	return (unsigned int)(x*x + y*y);
}

/** Calculates distance between 2 scriptables */
unsigned int Distance(Scriptable *a, Scriptable *b)
{
	long x = ( a->Pos.x - b->Pos.x );
	long y = ( a->Pos.y - b->Pos.y );
	return (unsigned int) sqrt( ( double ) ( x* x + y* y ) );
}

/** Calculates distance squared between 2 scriptables */
unsigned int SquaredDistance(Scriptable *a, Scriptable *b)
{
	long x = ( a->Pos.x - b->Pos.x );
	long y = ( a->Pos.y - b->Pos.y );
	return (unsigned int) ( x* x + y* y );
}

/** Calculates distance between 2 scriptables, including feet circle if applicable */
unsigned int PersonalDistance(Scriptable *a, Scriptable *b)
{
	long x = ( a->Pos.x - b->Pos.x );
	long y = ( a->Pos.y - b->Pos.y );
	int ret = (int) sqrt( ( double ) ( x* x + y* y ) );
	if (a->Type==ST_ACTOR) {
		ret-=((Actor *)a)->size*10;
	}
	if (b->Type==ST_ACTOR) {
		ret-=((Actor *)b)->size*10;
	}
	if (ret<0) return (unsigned int) 0;
	return (unsigned int) ret;
}

unsigned int SquaredPersonalDistance(Scriptable *a, Scriptable *b)
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

// returns EA relation between two scriptables (non actors are always enemies)
// it is used for protectile targeting/iwd ids targeting too!
int EARelation(Scriptable* Owner, Actor* target)
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
	ieDword hour = 1 << (((time/AI_UPDATE_TIME)+(300/2))%7200/300);
	return (schedule & hour) != 0;
}

// safely copies a ResRef (ie. nulls out the unused buffer size)
void CopyResRef(ieResRef d, const ieResRef s)
{
	strncpy(d, s, sizeof(ieResRef) - 1);
	d[sizeof(ieResRef) - 1] = '\0';
}

}
