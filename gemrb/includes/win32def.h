#ifndef WIN32DEF_H
#define WIN32DEF_H

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#if _MSC_VER >= 1000
#pragma warning( disable: 4251 521 )
#endif

#define ADV_TEXT
#include <conio.h>
#ifndef INTERFACE
#define textcolor(i) SetConsoleTextAttribute(hConsole, i)
#else
#define textcolor(i) SetConsoleTextAttribute(hConsole, i)
#endif
#define printf cprintf

#else
#include <config.h>
#include <stdio.h>
#include <stdlib.h>

#define ADV_TEXT
#define textcolor(i) i

#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

#ifndef HAVE_SNPRINTF
#ifdef WIN32
#define snprintf _snprintf
#else
#include "../plugins/Core/snprintf.h"
#endif
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

#define MYASSERT(f) \
  if(!(f))  \
  {  \
  printf("Assertion failed: %s %d",#f, __LINE__); \
				abort(); \
  }

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
#define BLACK printf("\033[0m\033[30m");
#define RED printf("\033[0m\033[31m");
#define GREEN printf("\033[0m\033[32m");
#define BROWN printf("\033[0m\033[33m");
#define BLUE printf("\033[0m\033[34m");
#define MAGENTA printf("\033[0m\033[35m");
#define CYAN printf("\033[0m\033[36m");
#define WHITE printf("\033[0m\033[37m");
#define LIGHT_RED printf("\033[1m\033[31m");
#define LIGHT_GREEN printf("\033[1m\033[32m");
#define YELLOW printf("\033[1m\033[33m");
#define LIGHT_BLUE printf("\033[1m\033[34m");
#define LIGHT_MAGENTA printf("\033[1m\033[35m");
#define LIGHT_CYAN printf("\033[1m\033[36m");
#define LIGHT_WHITE printf("\033[1m\033[37m");
#define gotoxy(x,y) printf("\033[%d;%dH", y, x)
#endif

#define printBracket(status, color) textcolor(WHITE); printf("["); textcolor(color); printf("%s", status); textcolor(WHITE); printf("]")
#define printStatus(status, color) printBracket(status, color); printf("\n")
#define printMessage(owner, message, color) printBracket(owner, LIGHT_WHITE); printf(": "); textcolor(color); printf("%s", message)

#endif

#endif
