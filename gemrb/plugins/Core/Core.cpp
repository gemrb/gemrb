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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Core.cpp,v 1.31 2005/02/24 22:09:33 avenger_teambg Exp $
 *
 */

// Core.cpp : Defines the entry point for the DLL application.
//

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

BOOL WINAPI DllEntryPoint(HINSTANCE hinstDLL, DWORD fdwReason,
	LPVOID lpvReserved)
{
	return true;
}
#endif

#include "../../includes/globals.h"
#include "Interface.h" 

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif

//// Globally used functions

unsigned char pl_uppercase[256];
unsigned char pl_lowercase[256];

/* this function will copy a string to a zero terminated string with a maximum length */
void strnuprcpy(char* dest, const char *source, int count)
{
	while(count--) {
		*dest++ = pl_uppercase[*source];
		if(!*source++) return;
	}
	*dest=0;
}

static unsigned char orientations[25]={
6,7,8,9,10,
5,6,8,10,11,
4,4,0,12,12,
3,2,0,14,13,
2,1,0,15,14
};

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

unsigned int Distance(Point p, Point q)
{
	long x = ( p.x - q.x );
	long y = ( p.y - q.y );
	return (unsigned int) sqrt( ( double ) ( x* x + y* y ) );
}

unsigned int Distance(Point p, Scriptable *b)
{
	long x = ( p.x - b->Pos.x );
	long y = ( p.y - b->Pos.y );
	return (unsigned int) sqrt( ( double ) ( x* x + y* y ) );
}

unsigned int Distance(Scriptable *a, Scriptable *b)
{
	long x = ( a->Pos.x - b->Pos.x );
	long y = ( a->Pos.y - b->Pos.y );
	return (unsigned int) sqrt( ( double ) ( x* x + y* y ) );
}

//returns true if path is an existing directory
GEM_EXPORT bool dir_exists(const char* path)
{
	struct stat buf;

	buf.st_mode = 0;
	stat( path, &buf );
	return S_ISDIR( buf.st_mode ) != 0;
}

//returns the length of string (up to a delimiter)
GEM_EXPORT int strlench(const char* string, char ch)
{
	int i;
	for (i = 0; string[i] && string[i] != ch; i++)
		;
	return i;
}

//// Compatibility functions
#ifndef HAVE_STRNDUP
GEM_EXPORT char* strndup(const char* s, int l)
{
	int len = strlen( s );
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
char* FindInDir(char* Dir, char* Filename)
{
	char* fn = NULL;
	DIR* dir = opendir( Dir );
	if (dir == NULL) {
		return NULL;
	}

	// First test if there's a Filename with exactly same name
	//   and if yes, return it and do not search in the Dir
	char TempFilePath[_MAX_PATH];
	strcpy( TempFilePath, Dir );
	strcat( TempFilePath, SPathDelimiter );
	strcat( TempFilePath, Filename );

	if (!access( TempFilePath, F_OK )) {
		closedir( dir );
		return strdup( Filename );
	}

	// Exact match not found, so try to search for Filename
	//    with different case
	struct dirent* de = readdir( dir );
	if (de == NULL) {
		closedir( dir );
		return NULL;
	}
	do {
		if (strcasecmp( de->d_name, Filename ) == 0) {
			fn = ( char * ) malloc( strlen( de->d_name ) + 1 );
			strcpy( fn, de->d_name );
			break;
		}
	} while (( de = readdir( dir ) ) != NULL);
	closedir( dir );  //No other files in the directory, close it
	return fn;
}

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

void ResolveFilePath(char* FilePath)
{
	char TempFilePath[_MAX_PATH];
	char TempFileName[_MAX_PATH];
	int j, pos;

	if (core && !core->CaseSensitive) {
		return;
	}

	TempFilePath[0] = FilePath[0];
	for (pos = 1; FilePath[pos] && FilePath[pos] != '/'; pos++)
		TempFilePath[pos] = FilePath[pos];
	TempFilePath[pos] = 0;
	while (FilePath[pos] == '/') {
		pos++;
		for (j = 0; FilePath[pos + j] && FilePath[pos + j] != '/'; j++) {
			TempFileName[j] = FilePath[pos + j];
		}
		TempFileName[j] = 0;
		pos += j;
		char* NewName = FindInDir( TempFilePath, TempFileName );
		if (NewName) {
			strcat( TempFilePath, SPathDelimiter );
			strcat( TempFilePath, NewName );
			free( NewName );
		} else {
			if (j)
				return;
			else {
				strcat( TempFilePath, SPathDelimiter );
			}
		}
	}
	//should work (same size)
	strcpy( FilePath, TempFilePath );
}

#endif //WIN32

