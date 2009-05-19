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
 * $Id$
 *
 */

// Config.cpp : Simple utility to create (and maybe modify) GemRB.cfg

#ifdef WIN32
#include "windows.h"
HANDLE hConsole;
#endif

#include "../includes/globals.h"
#include "../includes/win32def.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>
#ifndef WIN32
#include <unistd.h>
#else
#include <direct.h>
#endif

FILE* config;
bool cont = true;
char CurrentDir[_MAX_PATH];

typedef struct GameStruct {
	char Name[_MAX_PATH];
	char GameType[16];
	int Width;
	int Height;
	int Bpp;
	int FullScreen;
	int CaseSensitive;
	int GameOnCD;
	int ScrollBarPatch;
	int AllStringsTagged;
	int MidResAvatars;
	int HasSongList;
	int UpperButtonText;
	int LowerLabelText;
	int HasPartyINI;
	int ForceStereo;
	int IgnoreButtonFrames;
	char CursorBAM[9];
	char GameDataPath[_MAX_PATH];
	char GameOverridePath[_MAX_PATH];
	char ButtonFont[9];
	char GemRBPath[_MAX_PATH];
	char CachePath[_MAX_PATH];
	char GUIScriptsPath[_MAX_PATH];
	char GamePath[_MAX_PATH];
	char INIConfig[_MAX_PATH];
	int CDCount;
	char CDs[6][_MAX_PATH];
} GameStruct;

std::vector< GameStruct> games;

void InitGames();
void MainMenu();
void NewConfig();
void AskCDPath(GameStruct* game, int disk);
int WriteConfig(GameStruct* game, char* file);
void ClearScreen()
{
#ifdef WIN32
	gotoxy( 0, 0 );
	for (int i = 0; i < 80 * 25; i++)
		putchar( ' ' );
#else
	printf( "\033[J" );
#endif
}
void ClearLine()
{
#ifdef WIN32
	for (int i = 0; i < 80; i++)
		putchar( ' ' );
#else
	printf( "\033[K" );
#endif
}

int main(int , char** )
{
	getcwd( CurrentDir, _MAX_PATH );
	strcat( CurrentDir, SPathDelimiter );
	InitGames();
#ifdef WIN32
	hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
#endif
	ClearScreen();
	gotoxy( 0, 0 );
	textcolor( LIGHT_WHITE );
	printf( "GemRB v%s Configuration Utility\n\n", VERSION_GEMRB);
	printMessage( "Configurer", "Opening GemRB.cfg file...", WHITE );
	config = fopen( "GemRB.cfg", "r+" );
	if (!config) {
		printStatus( "ERROR", LIGHT_RED );
		return -1;
	}
	printStatus( "OK", LIGHT_GREEN );

	while (cont)
		MainMenu();

	fclose( config );
	return 0;
}

void MainMenu()
{
	unsigned int choice;
	for (int i = 2; i < 20; i++) {
		gotoxy( 0, i );
		ClearLine();
	}
	textcolor( LIGHT_WHITE );
	gotoxy( 0, 2 );
	printf( "--==[Main Menu]==--" );
	textcolor( WHITE );
	gotoxy( 0, 4 );
	printf( "1. Create a new Configuration\n" );
	printf( "2. Edit an existing Configuration\n" );
	printf( "3. Edit current Configuration\n" );
	printf( "4. Change Configuration\n" );
	printf( "5. Exit" );
	ask:
	gotoxy( 0, 11 );
	ClearLine();
	gotoxy( 0, 11 );
	printf( "Choose: " );
	choice = getchar();
	if (( choice <= '0' ) || ( choice > '5' )) {
		goto ask;
	}
	switch (choice) {
		case '1':
			NewConfig();
			break;

		case '2':
			break;

		case '3':
			break;

		case '4':
			break;

		case '5':
			cont = false;
			break;
	}
}

void NewConfig()
{
	GameStruct newGame;
	int step = 0;
	for (int i = 2; i <= 20; i++) {
		gotoxy( 0, i );
		ClearLine();
	}
	gotoxy( 0, 2 );
	textcolor( LIGHT_WHITE );
	printf( "--==[New Configuration]==--" );
	textcolor( WHITE );
	NewConfigSteps:
	switch (step) {
		case 0:
			//Select Game Type
			 {
				unsigned int choice;

				gotoxy( 0, 4 );
				printf( "Select a Game Type from the following list:" );
				gotoxy( 0, 6 );
				for (unsigned int i = 0; i < games.size(); i++) {
					printf( "%d. %s\n", i + 1, games[i].Name );
				}

				sgt_ask : gotoxy( 0, 13 );
				ClearLine();
				gotoxy( 0, 13 );
				printf( "Choose: " );
				choice = getchar();
				if (( choice <= 0x30 ) ||
				    ( choice > ( 0x30 + games.size() ) )) {
					goto sgt_ask;
				}
				newGame = games[choice - 0x31];
				step++;
				goto NewConfigSteps;
			}
			break;

		case 1:
			//Game Path
			 {
				struct stat st;
				char tmp[_MAX_PATH];

				for (int i = 4; i <= 20; i++) {
					gotoxy( 0, i );
					ClearLine();
				}
				gotoxy( 0, 4 );
				printf( "Enter the Directory where the Game is Installed:" );
				gp_ask : gotoxy( 0, 6 );
				ClearLine();
				gotoxy( 0, 6 );
				printf( "GamePath=" );
				//if(!scanf("%[^]\n", GamePath))
				//	goto gp_ask;
				fgets( newGame.GamePath, _MAX_PATH, stdin );
				if (newGame.GamePath[0] == 0)
					goto gp_ask;

				strcat( newGame.GamePath, SPathDelimiter );
				strcpy( tmp, newGame.GamePath );
				strcat( tmp, "chitin.key" );

				if (stat( tmp, &st ) == -1) {
					gotoxy( 0, 8 );
					printf( "The Path Entered is not valid. Cannot find chitin.key." );
					goto gp_ask;
				}
				step++;
				goto NewConfigSteps;
			}
			break;

		case 2:
			//GameOnCD
			 {
				char Choose[_MAX_PATH];
				for (int i = 4; i <= 20; i++) {
					gotoxy( 0, i );
					ClearLine();
				}
				gocd_ask : gotoxy( 0, 4 );
				ClearLine();
				gotoxy( 0, 4 );
				printf( "Is the Game Fully installed on your HD [y/n] " );

				//if(!scanf("%[^]\n", GamePath))
				//	goto gp_ask;
				fgets( Choose, _MAX_PATH, stdin );
				if (Choose[0] == 0)
					goto gocd_ask;

				if (( stricmp( Choose, "yes" ) == 0 ) ||
					( stricmp( Choose, "y" ) == 0 )) {
					newGame.GameOnCD = 0;
				} else {
					if (( stricmp( Choose, "no" ) != 0 ) &&
						( stricmp( Choose, "n" ) != 0 ))
						goto gocd_ask;
					newGame.GameOnCD = 1;
				}
				for (int i = 0; i < newGame.CDCount; i++) {
					AskCDPath( &newGame, i + 1 );
				}
				step++;
				goto NewConfigSteps;
			}
			break;

		case 3:
			//Save
			 {
				WriteConfig( &newGame, ".\\GemRBcfg.cfg" );
			}
			break;
	}
}

void InitGames()
{
	GameStruct bg1 = {
		"Baldur's Gate",	//Name
		"bg1",				//GameType
		640,				//Width
		480,				//Height
		32,					//Bpp
		0,					//FullScreen
		0,					//CaseSensitive
		0,					//GameOnCD
		0,					//ScrollBarPatch
		1,					//AllStringsTagged
		1,					//MidResAvatars
		1,					//HasSongList
		1,					//UpperButtonText
		1,					//LowerLabelText		
		0,					//HasPartyINI
		1,					//ForceStereo
		0,  									//IgnoreButtonFrames
		"CAROT",			//CursorBAM
		"",					//GameDataPath
		"",					//GameOverridePath
		"",					//ButtonFont
		"",					//GemRBPath
		"",					//CachePath
		"",					//GUIScriptsPath
		"",					//GamePath
		"baldur.ini",		//INIConfig
		4,					//CDCount
		{"",					//CD1
		"",					//CD2
		"",					//CD3
		"",					//CD4
		"",					//CD5
		""}					//CD6

	};
	games.push_back( bg1 );
	GameStruct bg2 = {
		"Baldur's Gate 2",	//Name
		"bg2",				//GameType
		640,				//Width
		480,				//Height
		32,					//Bpp
		0,					//FullScreen
		0,					//CaseSensitive
		0,					//GameOnCD
		0,					//ScrollBarPatch
		0,					//AllStringsTagged
		0,					//MidResAvatars
		1,					//HasSongList
		1,					//UpperButtonText
		1,					//LowerLabelText		
		0,					//HasPartyINI
		0,					//ForceStereo
		0,  									//IgnoreButtonFrames
		"CAROT",			//CursorBAM
		"",					//GameDataPath
		"",					//GameOverridePath
		"",					//ButtonFont
		"",					//GemRBPath
		"",					//CachePath
		"",					//GUIScriptsPath
		"",					//GamePath
		"baldur.ini",		//INIConfig
		4,					//CDCount
		{"",					//CD1
		"",					//CD2
		"",					//CD3
		"",					//CD4
		"",					//CD5
		""}					//CD6

	};
	games.push_back( bg2 );
	GameStruct tob = {
		"Throne of Bhaal",	//Name
		"tob",				//GameType
		640,				//Width
		480,				//Height
		32,					//Bpp
		0,					//FullScreen
		0,					//CaseSensitive
		0,					//GameOnCD
		0,					//ScrollBarPatch
		0,					//AllStringsTagged
		0,					//MidResAvatars
		1,					//HasSongList
		1,					//UpperButtonText
		1,					//LowerLabelText		
		0,					//HasPartyINI
		0,					//ForceStereo
		0,  									//IgnoreButtonFrames
		"CAROT",			//CursorBAM
		"",					//GameDataPath
		"",					//GameOverridePath
		"",					//ButtonFont
		"",					//GemRBPath
		"",					//CachePath
		"",					//GUIScriptsPath
		"",					//GamePath
		"baldur.ini",		//INIConfig
		4,					//CDCount
		{"",					//CD1
		"",					//CD2
		"",					//CD3
		"",					//CD4
		"",					//CD5
		""}					//CD6

	};
	games.push_back( tob );
	GameStruct iwd = {
		"IceWind Dale",		//Name
		"iwd",				//GameType
		640,				//Width
		480,				//Height
		32,					//Bpp
		0,					//FullScreen
		0,					//CaseSensitive
		0,					//GameOnCD
		0,					//ScrollBarPatch
		1,					//AllStringsTagged
		1,					//MidResAvatars
		0,					//HasSongList
		0,					//UpperButtonText
		0,					//LowerLabelText		
		0,					//HasPartyINI
		1,					//ForceStereo
		0,  									//IgnoreButtonFrames
		"CAROT",			//CursorBAM
		"",					//GameDataPath
		"",					//GameOverridePath
		"",					//ButtonFont
		"",					//GemRBPath
		"",					//CachePath
		"",					//GUIScriptsPath
		"",					//GamePath
		"icewind.ini",		//INIConfig
		2,					//CDCount
		{"",					//CD1
		"",					//CD2
		"",					//CD3
		"",					//CD4
		"",					//CD5
		""}					//CD6

	};
	games.push_back( iwd );
	GameStruct iwd2 = {
		"IceWind Dale 2",	//Name
		"iwd2",				//GameType
		800,				//Width
		600,				//Height
		32,					//Bpp
		0,					//FullScreen
		0,					//CaseSensitive
		0,					//GameOnCD
		0,					//ScrollBarPatch
		1,					//AllStringsTagged
		0,					//MidResAvatars
		0,					//HasSongList
		0,					//UpperButtonText
		0,					//LowerLabelText		
		1,					//HasPartyINI
		0,					//ForceStereo
		0,  									//IgnoreButtonFrames
		"CAROT",			//CursorBAM
		"",					//GameDataPath
		"",					//GameOverridePath
		"NORMAL",				//ButtonFont
		"",					//GemRBPath
		"",					//CachePath
		"",					//GUIScriptsPath
		"",					//GamePath
		"icewind2.ini",		//INIConfig
		2,					//CDCount
		{"",					//CD1
		"",					//CD2
		"",					//CD3
		"",					//CD4
		"",					//CD5
		""}					//CD6

	};
	games.push_back( iwd2 );
	GameStruct how = {
		"IceWind Dale: Heart of Winter",	//Name
		"how",				//GameType
		640,				//Width
		480,				//Height
		32,					//Bpp
		0,					//FullScreen
		0,					//CaseSensitive
		0,					//GameOnCD
		0,					//ScrollBarPatch
		1,					//AllStringsTagged
		1,					//MidResAvatars
		0,					//HasSongList
		0,					//UpperButtonText
		0,					//LowerLabelText		
		0,					//HasPartyINI
		1,					//ForceStereo
		0,  									//IgnoreButtonFrames
		"CAROT",			//CursorBAM
		"",					//GameDataPath
		"",					//GameOverridePath
		"",					//ButtonFont
		"",					//GemRBPath
		"",					//CachePath
		"",					//GUIScriptsPath
		"",					//GamePath
		"icewind.ini",		//INIConfig
		5,					//CDCount
		{"",					//CD1
		"",					//CD2
		"",					//CD3
		"",					//CD4
		"",					//CD5
		""}					//CD6

	};
	games.push_back( how );
	GameStruct pst = {
		"Planescape: Torment",	//Name
		"pst",				//GameType
		640,				//Width
		480,				//Height
		32,					//Bpp
		0,					//FullScreen
		0,					//CaseSensitive
		0,					//GameOnCD
		0,					//ScrollBarPatch
		1,					//AllStringsTagged
		0,					//MidResAvatars
		0,					//HasSongList
		0,					//UpperButtonText
		0,					//LowerLabelText		
		0,					//HasPartyINI
		1,					//ForceStereo
		1,  									//IgnoreButtonFrames
		"CARET",			//CursorBAM
		"",					//GameDataPath
		"",					//GameOverridePath
		"FONTDLG",			//ButtonFont
		"",					//GemRBPath
		"",					//CachePath
		"",					//GUIScriptsPath
		"",					//GamePath
		"torment.ini",		//INIConfig
		4,					//CDCount
		{"",					//CD1
		"",					//CD2
		"",					//CD3
		"",					//CD4
		"",					//CD5
		""}					//CD6

	};
	games.push_back( pst );
}

void AskCDPath(GameStruct* game, int disk)
{
	char CDPath[_MAX_PATH], tmp[_MAX_PATH], CDNum[_MAX_PATH];
	sprintf( CDNum, "CD%d", disk );
	for (int i = 4; i <= 20; i++) {
		gotoxy( 0, i );
		ClearLine();
	}
	gotoxy( 0, 4 );
	printf( "Enter the Path to the \"%s Disk %d\":", game->Name, disk );

	ad_ask:
	gotoxy( 0, 6 );
	ClearLine();
	gotoxy( 0, 6 );
	printf( "%s=", CDNum );
	//if(!scanf("%[^]\n", GamePath))
	//	goto gp_ask;
	fgets( CDPath, _MAX_PATH, stdin );
	if (CDPath[0] == 0) {
		goto ad_ask;
	}

	strcat( CDPath, SPathDelimiter );
	strcpy( tmp, CDPath );
	strcat( tmp, CDNum );
	strcat( tmp, SPathDelimiter );

	strcpy( game->CDs[disk - 1], tmp );
}

int WriteConfig(GameStruct* game, char* file)
{
	FILE* c = fopen( file, "w+" );
	if (!c) {
		gotoxy( 0, 18 );
		textcolor( LIGHT_RED );
		printf( "Cannot Open %s\n", file );
		textcolor( WHITE );
		return -1;
	}
	fprintf( c, "#######################################\n" );
	fprintf( c, "#      GemRB Configuration File       #\n" );
	fprintf( c, "#                                     #\n" );
	fprintf( c, "#            Generated By             #\n" );
	fprintf( c, "#     GemRB Configuration Utility     #\n" );
	fprintf( c, "#######################################\n" );
	fprintf( c, "GameName=%s\n", game->Name );
	fprintf( c, "GameType=%s\n", game->GameType );
	fprintf( c, "Width=%d\n", game->Width );
	fprintf( c, "Height=%d\n", game->Height );
	fprintf( c, "Bpp=%d\n", game->Bpp );
	fprintf( c, "FullScreen=%d\n", game->FullScreen );
	fprintf( c, "CaseSensitive=%d\n", game->CaseSensitive );
	fprintf( c, "GameOnCD=%d\n", game->GameOnCD );
	fprintf( c, "ScrollBarPatch=%d\n", game->ScrollBarPatch );
	fprintf( c, "AllStringsTagged=%d\n", game->AllStringsTagged );
	fprintf( c, "MidResAvatars=%d\n", game->MidResAvatars );
	fprintf( c, "HasSongList=%d\n", game->HasSongList );
	fprintf( c, "UpperButtonText=%d\n", game->UpperButtonText );
	fprintf( c, "LowerLabelText=%d\n", game->LowerLabelText );
	fprintf( c, "HasPartyINI=%d\n", game->HasPartyINI );
	fprintf( c, "ForceStereo=%d\n", game->ForceStereo );
	fprintf( c, "IgnoreButtonFrames=%d\n", game->IgnoreButtonFrames );
	fprintf( c, "CursorBAM=%s\n", game->CursorBAM );
	if (game->ButtonFont[0] != 0) {
		fprintf( c, "ButtonFont=%s\n", game->ButtonFont );
	}
	fprintf( c, "GemRBPath=%s\n", CurrentDir );
	fprintf( c, "CachePath=%s%s%s\n", CurrentDir, SPathDelimiter, "Cache" );
	fprintf( c, "GUIScriptsPath=%s\n", CurrentDir );
	fprintf( c, "GamePath=%s\n", game->GamePath );
	fprintf( c, "INIConfig=%s\n", game->INIConfig );
	for (int i = 0; i < game->CDCount; i++) {
		fprintf( c, "CD%d=%s\n", i + 1, game->CDs[i] );
	}
	fclose( c );
	return 0;
}
