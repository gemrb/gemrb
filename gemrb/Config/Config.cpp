#include "windows.h"

#ifdef WIN32
HANDLE hConsole;
#endif

#include "../includes/globals.h"
#include "../includes/win32def.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>

FILE * config;
bool cont = true;

typedef struct GameStruct {
	char Name[_MAX_PATH];
	char GameType[16];
	int  Width;
	int  Height;
	int  Bpp;
	int  FullScreen;
	int  CaseSensitive;
	int  GameOnCD;
	int  ScrollBarPatch;
	int  AllStringsTagged;
	int  MidResAvatars;
	int  HasSongList;
	int  UpperButtonText;
	int  LowerLableText;
	int  HasPartyINI;
	int  ForceStereo;
	char CursorBAM[9];
	char GameDataPath[_MAX_PATH];
	char GameOverridePath[_MAX_PATH];
	char ButtonFont[9];
	char GemRBPath[_MAX_PATH];
	char CachePath[_MAX_PATH];
	char GUIScriptsPath[_MAX_PATH];
	char GamePath[_MAX_PATH];
	char INIConfig[_MAX_PATH];
	char CD1[_MAX_PATH];
	char CD2[_MAX_PATH];
	char CD3[_MAX_PATH];
	char CD4[_MAX_PATH];
	char CD5[_MAX_PATH];
	char CD6[_MAX_PATH];
} GameStruct;

std::vector<GameStruct> games;

void InitGames();
void MainMenu();
void NewConfig();
void ClearScreen()
{
#ifdef WIN32
	gotoxy(0,0);
	for(int i = 0; i < 80*25; i++)
		putchar(' ');
#else
	printf("\033[J");
#endif
}
void ClearLine()
{
#ifdef WIN32
	for(int i = 0; i < 80; i++)
		putchar(' ');
#else
	printf("\033[K");
#endif
}

int main(int argc, char **argv)
{
	InitGames();
#ifdef WIN32
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
	ClearScreen();
	gotoxy(0,0);
	textcolor(LIGHT_WHITE);
	printf("GemRB v%.1f.%d.%d Configuration Utility\n\n", GEMRB_RELEASE/1000.0, GEMRB_API_NUM, GEMRB_SDK_REV);
	printMessage("Configurer", "Opening GemRB.cfg file...", WHITE);
	config = fopen("../GemRB.cfg", "r+");
	if(!config) {
		printStatus("ERROR", LIGHT_RED);
		return -1;
	}
	printStatus("OK", LIGHT_GREEN);
	
	while(cont)
		MainMenu();

	fclose(config);
	return 0;
}

void MainMenu()
{
	int choice;
	for(int i = 2; i < 20; i++) {
		gotoxy(0, i);
		ClearLine();
	}
	textcolor(LIGHT_WHITE);
	gotoxy(0, 2);
	printf("--==[Main Menu]==--");
	textcolor(WHITE);
	gotoxy(0, 4);
	printf("1. Create a new Configuration\n");
	printf("2. Edit an existing Configuration\n");
	printf("3. Edit current Configuration\n");
	printf("4. Change Configuration\n");
	printf("5. Exit");
ask:
	gotoxy(0,11);
	ClearLine();
	gotoxy(0,11);
	printf("Choose: ");
	choice = getchar();
	if((choice <= '0') || (choice > '5')) {
		goto ask;
	}
	switch(choice) {
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
	for(int i = 2; i <= 20; i++) {
		gotoxy(0,i);
		ClearLine();
	}
	gotoxy(0,2);
	textcolor(LIGHT_WHITE);
	printf("--==[New Configuration]==--");
	textcolor(WHITE);
NewConfigSteps:
	switch(step) {
		case 0: //Select Game Type
			{
				int choice;

				gotoxy(0,4);
				printf("Select a Game Type from the following list:");
				gotoxy(0,6);
				for(int i = 0; i < games.size(); i++) {
					printf("%d. %s\n", i+1, games[i].Name);
				}

sgt_ask:
				gotoxy(0,13);
				ClearLine();
				gotoxy(0,13);
				printf("Choose: ");
				choice = getchar();
				if((choice <= '0') || (choice > (0x30+games.size()))) {
					goto sgt_ask;
				}
				strcpy(newGame.GameType, games[choice-0x31].GameType);
				step++;
				goto NewConfigSteps;
			}
		break;

		case 1: //Game Path
			{
				struct stat st;
				char tmp[_MAX_PATH];

				for(int i = 4; i <= 20; i++) {
					gotoxy(0,i);
					ClearLine();
				}
				gotoxy(0,4);
				printf("Enter the Directory where the Game is Installed:");
gp_ask:
				gotoxy(0,6);
				ClearLine();
				gotoxy(0,6);
				printf("GamePath=");
				//if(!scanf("%[^]\n", GamePath))
				//	goto gp_ask;
				gets(newGame.GamePath);
				if(newGame.GamePath[0] == 0)
					goto gp_ask;

				strcat(newGame.GamePath, SPathDelimiter);
				strcpy(tmp, newGame.GamePath);
				strcat(tmp, "chitin.key");
				
				if(stat(tmp, &st) == -1) {
					gotoxy(0, 8);
					printf("The Path Entered is not valid. Cannot find chitin.key.");
					goto gp_ask;
				}
				step++;
				goto NewConfigSteps;
			}
		break;

		case 2: //GameOnCD
			{
				char Choose[_MAX_PATH];
				for(int i = 4; i <= 20; i++) {
					gotoxy(0,i);
					ClearLine();
				}
gocd_ask:
				gotoxy(0,4);
				ClearLine();
				gotoxy(0,4);
				printf("Is the Game Fully installed on your HD [y/n] ");

				//if(!scanf("%[^]\n", GamePath))
				//	goto gp_ask;
				gets(Choose);
				if(Choose[0] == 0)
					goto gocd_ask;

				if((stricmp(Choose, "yes") == 0) || (stricmp(Choose, "y") == 0))
					newGame.GameOnCD = 0;
				else {
					if((stricmp(Choose, "no") != 0) && (stricmp(Choose, "n") != 0))
						goto gocd_ask;
					newGame.GameOnCD = 1;
				}
				step++;
				goto NewConfigSteps;
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
		0,					//MidResAvatars
		1,					//HasSongList
		1,					//UpperButtonText
		0,					//LowerLabelText		
		0,					//HasPartyINI
		1,					//ForceStereo
		"CAROT",			//CursorBAM
		"",					//GameDataPath
		"",					//GameOverridePath
		"",					//ButtonFont
		"",					//GemRBPath
		"",					//CachePath
		"",					//GUIScriptsPath
		"",					//GamePath
		"",					//INIConfig
		"",					//CD1
		"",					//CD2
		"",					//CD3
		"",					//CD4
		"",					//CD5
		""					//CD6
	};
	games.push_back(bg1);
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
		0,					//HasSongList
		1,					//UpperButtonText
		1,					//LowerLabelText		
		0,					//HasPartyINI
		0,					//ForceStereo
		"CAROT",			//CursorBAM
		"",					//GameDataPath
		"",					//GameOverridePath
		"",					//ButtonFont
		"",					//GemRBPath
		"",					//CachePath
		"",					//GUIScriptsPath
		"",					//GamePath
		"",					//INIConfig
		"",					//CD1
		"",					//CD2
		"",					//CD3
		"",					//CD4
		"",					//CD5
		""					//CD6
	};
	games.push_back(bg2);
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
		0,					//MidResAvatars
		1,					//HasSongList
		0,					//UpperButtonText
		0,					//LowerLabelText		
		1,					//HasPartyINI
		1,					//ForceStereo
		"CAROT",			//CursorBAM
		"",					//GameDataPath
		"",					//GameOverridePath
		"",					//ButtonFont
		"",					//GemRBPath
		"",					//CachePath
		"",					//GUIScriptsPath
		"",					//GamePath
		"",					//INIConfig
		"",					//CD1
		"",					//CD2
		"",					//CD3
		"",					//CD4
		"",					//CD5
		""					//CD6
	};
	games.push_back(iwd);
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
		0,					//AllStringsTagged
		0,					//MidResAvatars
		0,					//HasSongList
		0,					//UpperButtonText
		0,					//LowerLabelText		
		1,					//HasPartyINI
		0,					//ForceStereo
		"CAROT",			//CursorBAM
		"",					//GameDataPath
		"",					//GameOverridePath
		"",					//ButtonFont
		"",					//GemRBPath
		"",					//CachePath
		"",					//GUIScriptsPath
		"",					//GamePath
		"",					//INIConfig
		"",					//CD1
		"",					//CD2
		"",					//CD3
		"",					//CD4
		"",					//CD5
		""					//CD6
	};
	games.push_back(iwd2);
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
		0,					//MidResAvatars
		1,					//HasSongList
		0,					//UpperButtonText
		0,					//LowerLabelText		
		1,					//HasPartyINI
		1,					//ForceStereo
		"CAROT",			//CursorBAM
		"",					//GameDataPath
		"",					//GameOverridePath
		"",					//ButtonFont
		"",					//GemRBPath
		"",					//CachePath
		"",					//GUIScriptsPath
		"",					//GamePath
		"",					//INIConfig
		"",					//CD1
		"",					//CD2
		"",					//CD3
		"",					//CD4
		"",					//CD5
		""					//CD6
	};
	games.push_back(how);
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
		1,					//HasSongList
		0,					//UpperButtonText
		0,					//LowerLabelText		
		0,					//HasPartyINI
		1,					//ForceStereo
		"CARET",			//CursorBAM
		"",					//GameDataPath
		"",					//GameOverridePath
		"",					//ButtonFont
		"",					//GemRBPath
		"",					//CachePath
		"",					//GUIScriptsPath
		"",					//GamePath
		"",					//INIConfig
		"",					//CD1
		"",					//CD2
		"",					//CD3
		"",					//CD4
		"",					//CD5
		""					//CD6
	};
	games.push_back(pst);
}
