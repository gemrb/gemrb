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

#include <cmath>
#ifndef WIN32
#include <ctype.h>
#include <sys/time.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#else
#include <windows.h>
#include <sys\stat.h>
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

#include "../../includes/globals.h"
#include "../../includes/exports.h"
#include "Interface.h"
#include "Actor.h"

//// Globally used functions

ieByte pl_uppercase[256];
ieByte pl_lowercase[256];

// these 3 functions will copy a string to a zero terminated string with a maximum length
void strnlwrcpy(char *dest, const char *source, int count)
{
	while(count--) {
		*dest++ = pl_lowercase[(ieByte) *source];
		if(!*source++) {
			while(count--) *dest++=0;
			break;
		}
	}
	*dest=0;
}

void strnuprcpy(char* dest, const char *source, int count)
{
	while(count--) {
		*dest++ = pl_uppercase[(ieByte) *source];
		if(!*source++) {
			while(count--) *dest++=0;
			break;
		}
	}
	*dest=0;
}

// this one also filters spaces
void strnspccpy(char* dest, const char *source, int count)
{
	memset(dest,0,count);
	while(count--) {
		char c = pl_lowercase[(ieByte) *source];
		if (c!=' ') {
			*dest++=c;
		}
		if(!*source++) {
			return;
		}
	}
}

static const unsigned char orientations[25]={
6,7,8,9,10,
5,6,8,10,11,
4,4,0,12,12,
3,2,0,14,13,
2,1,0,15,14
};

/** Calculates the orientation of a character (or projectile) facing a point */
unsigned char GetOrient(Point &s, Point &d)
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

/** Calculates distance between 2 scriptables */
unsigned int Distance(Scriptable *a, Scriptable *b)
{
	long x = ( a->Pos.x - b->Pos.x );
	long y = ( a->Pos.y - b->Pos.y );
	return (unsigned int) sqrt( ( double ) ( x* x + y* y ) );
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

/** Returns the length of string (up to a delimiter) */
GEM_EXPORT int strlench(const char* string, char ch)
{
	int i;
	for (i = 0; string[i] && string[i] != ch; i++)
		;
	return i;
}

//// Compatibility functions
#ifndef HAVE_STRNDUP
GEM_EXPORT char* strndup(const char* s, size_t l)
{
	size_t len = strlen( s );
	if (len < l) {
		l = len;
	}
	char* string = ( char* ) malloc( l + 1 );
	strncpy( string, s, l );
	string[l] = 0;
	return string;
}
#endif

#ifdef WIN32

#else

char* strupr(char* string)
{
	char* s;
	if (string) {
		for (s = string; *s; ++s)
			*s = toupper( *s );
	}
	return string;
}

char* strlwr(char* string)
{
	char* s;
	if (string) {
		for (s = string; *s; ++s)
			*s = tolower( *s );
	}
	return string;
}


#endif // ! WIN32

