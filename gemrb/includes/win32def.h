/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 * @file win32def.h
 * Some global definitions, mostly for Un*x vs. MS Windows compatibility
 * @author The GemRB Project
 */


#ifndef WIN32DEF_H
#define WIN32DEF_H

#ifdef WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>

# if _MSC_VER >= 1000
// 4251 disables the annoying warning about missing dll interface in templates
#  pragma warning( disable: 4251 521 )
//disables annoying warning caused by STL:Map in msvc 6.0
#  if _MSC_VER < 7000
#    pragma warning(disable:4786)
#  endif
# endif

# define ADV_TEXT
# include <conio.h>
# define textcolor(i) SetConsoleTextAttribute(hConsole, i)

# ifndef __MINGW32__
#  define printf cprintf //broken in mingw !!
# elif not defined HAVE_SNPRINTF
#  define HAVE_SNPRINTF
# endif

#else //WIN32
# include <config.h>
# include <stdio.h>
# include <stdlib.h>

# define ADV_TEXT
# define textcolor(i) i

# define stricmp strcasecmp
# define strnicmp strncasecmp
#endif //WIN32

#ifndef HAVE_SNPRINTF
# ifdef WIN32
#  define snprintf _snprintf
# else
#  include "../plugins/Core/snprintf.h"
# endif
#endif

#ifndef TRUE
# define TRUE 1
# define FALSE 0
#endif

//we need 32+6 bytes at least, because we store 'context' in the variable
//name too
#define MAX_VARIABLE_LENGTH  40
#include "../plugins/Core/VFS.h"

// abstract iteration position
struct __POSITION {
};
typedef __POSITION* POSITION;
#define BEFORE_START_POSITION ((POSITION)-1L)

#ifdef _DEBUG
#define MYASSERT(f) \
  if(!(f))  \
  {  \
  printf("Assertion failed: %s %d",#f, __LINE__); \
				abort(); \
  }
#else
#define MYASSERT(f)
#endif

#ifdef ADV_TEXT

#ifdef WIN32
#define BLACK 0
#define RED FOREGROUND_RED
#define GREEN FOREGROUND_GREEN
#define BROWN FOREGROUND_GREEN | FOREGROUND_RED
#define BLUE FOREGROUND_BLUE
#define MAGENTA FOREGROUND_RED | FOREGROUND_BLUE
#define CYAN FOREGROUND_BLUE | FOREGROUND_GREEN
#define WHITE FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED
#define LIGHT_RED (RED | FOREGROUND_INTENSITY)
#define LIGHT_GREEN (GREEN | FOREGROUND_INTENSITY)
#define YELLOW (GREEN | RED | FOREGROUND_INTENSITY)
#define LIGHT_BLUE (BLUE | FOREGROUND_INTENSITY)
#define LIGHT_MAGENTA (MAGENTA | FOREGROUND_INTENSITY)
#define LIGHT_CYAN (CYAN | FOREGROUND_INTENSITY)
#define LIGHT_WHITE (WHITE | FOREGROUND_INTENSITY)
#define gotoxy(x,y) \
	{ \
	COORD coord = {x,y}; \
	SetConsoleCursorPosition(hConsole, coord); \
	}
#else
#define BLACK printf("\033[0m\033[30;40m");
#define RED printf("\033[0m\033[31;40m");
#define GREEN printf("\033[0m\033[32;40m");
#define BROWN printf("\033[0m\033[33;40m");
#define BLUE printf("\033[0m\033[34;40m");
#define MAGENTA printf("\033[0m\033[35;40m");
#define CYAN printf("\033[0m\033[36;40m");
#define WHITE printf("\033[0m\033[37;40m");
#define LIGHT_RED printf("\033[1m\033[31;40m");
#define LIGHT_GREEN printf("\033[1m\033[32;40m");
#define YELLOW printf("\033[1m\033[33;40m");
#define LIGHT_BLUE printf("\033[1m\033[34;40m");
#define LIGHT_MAGENTA printf("\033[1m\033[35;40m");
#define LIGHT_CYAN printf("\033[1m\033[36;40m");
#define LIGHT_WHITE printf("\033[1m\033[37;40m");
#define gotoxy(x,y) printf("\033[%d;%dH", y, x)
#endif

#ifdef WIN32
# ifndef round
#  define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
# endif
#endif

#ifndef M_PI
#define M_PI    3.14159265358979323846 // pi
#endif
#ifndef M_PI_2
#define M_PI_2  1.57079632679489661923 // pi/2
#endif


#define printBracket(status, color) textcolor(WHITE); printf("["); textcolor(color); printf("%s", status); textcolor(WHITE); printf("]")
#define printStatus(status, color) printBracket(status, color); printf("\n")
#define printMessage(owner, message, color) printBracket(owner, LIGHT_WHITE); printf(": "); textcolor(color); printf("%s", message); textcolor(WHITE)

#endif

#endif  //! WIN32DEF_H
