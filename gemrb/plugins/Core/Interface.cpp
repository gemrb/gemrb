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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Interface.cpp,v 1.86 2003/12/04 02:00:12 doc_wagon Exp $
 *
 */

#define INTERFACE
#include "Interface.h"
#include "FileStream.h"
#include "AnimationMgr.h"
#include <stdlib.h>
#include <time.h>

#ifdef WIN32
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT
#endif

GEM_EXPORT Interface *core;

#ifdef WIN32
GEM_EXPORT HANDLE hConsole;
#endif

#include "../../includes/win32def.h"

Interface::Interface(void)
{
#ifdef WIN32
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
	textcolor(LIGHT_WHITE);
	printf("GemRB Core Version %d.%d Sub. %d Loading...\n", GEMRB_RELEASE, GEMRB_API_NUM, GEMRB_SDK_REV);
	video = NULL;
	key = NULL;
	strings = NULL;
	hcanims = NULL;
	guiscript = NULL;
	windowmgr = NULL;
	vars = NULL;
	tokens = NULL;
	music = NULL;
	soundmgr = NULL;
	sgiterator = NULL;
	INIparty = NULL;
	ConsolePopped = false;
	CheatFlag = false;
	GUIScriptsPath[0]=0;
	GamePath[0]=0;
	GemRBPath[0]=0;
	memcpy(ButtonFont,"STONESML",9);
	GameFeatures=0;
	printMessage("Core", "Loading Configuration File...", WHITE);
	if(!LoadConfig()) {
		printStatus("ERROR", LIGHT_RED);
		printMessage("Core", "Cannot Load Config File.\nTermination in Progress...\n", WHITE);
		exit(-1);
	}
	printStatus("OK", LIGHT_GREEN);
	printMessage("Core", "Starting Plugin Manager...\n", WHITE);
	plugin = new PluginMgr(GemRBPath);
	printMessage("Core", "Plugin Loading Complete!\n", LIGHT_GREEN);
	printMessage("Core", "Creating Object Factory...", WHITE);
	factory = new Factory();
	printStatus("OK", LIGHT_GREEN);
	time_t t;
	t = time(NULL);
	srand(t);
#ifdef _DEBUG
	FileStreamPtrCount = 0;
	CachedFileStreamPtrCount = 0;
#endif
}

#define FreeInterfaceVector(type, variable, member)   \
{  \
  std::vector<type>::iterator i;  \
  for(i = variable.begin(); i != variable.end(); ++i) { \
	if(!(*i).free) {  \
		core->FreeInterface((*i).member); \
		(*i).free = true; \
	} \
  } \
}

#define FreeResourceVector(type, variable)   \
{  \
  std::vector<type*>::iterator i=variable.begin();  \
  while(variable.size() ) { \
	if(*i) delete(*i); \
	variable.erase(i); \
	i = variable.begin(); \
	} \
} 

Interface::~Interface(void)
{
	if(INIparty)
		delete(INIparty);
	if(music)
		delete(music);
	if(soundmgr)
		delete(soundmgr);
	if(sgiterator)
		delete(sgiterator);

	FreeResourceVector(Font, fonts);
	FreeResourceVector(Window, windows);

	if(key)
		plugin->FreePlugin(key);	
	if(video)
		plugin->FreePlugin(video);
	if(strings)
		plugin->FreePlugin(strings);
	if(hcanims)
		plugin->FreePlugin(hcanims);
	if(pal256)
		plugin->FreePlugin(pal256);
	if(pal16)
		plugin->FreePlugin(pal16);
	if(windowmgr)
		delete(windowmgr);
	if(guiscript)
		delete(guiscript);
	if(vars)
		delete(vars);
	if(tokens)
		delete(tokens);
	FreeInterfaceVector(Table, tables, tm);
	FreeInterfaceVector(Symbol, symbols, sm);
	FreeResourceVector(Actor, actors);
	delete(console);
	delete(plugin);

	//TODO: Clean the Cache and leave only .bif files

}


int Interface::Init()
{
	printMessage("Core", "GemRB Core Initialization...\n", WHITE);
	printMessage("Core", "Searching for Video Driver...", WHITE);
	if(!IsAvailable(IE_VIDEO_CLASS_ID)) {
		printStatus("ERROR", LIGHT_RED);
		printf("No Video Driver Available.\nTermination in Progress...\n");
		return GEM_ERROR;
	}
	printStatus("OK", LIGHT_GREEN);
	printMessage("Core", "Initializing Video Plugin...", WHITE);
	video = (Video*)GetInterface(IE_VIDEO_CLASS_ID);
	if(video->Init() == GEM_ERROR) {
		printStatus("ERROR", LIGHT_RED);
		printf("Cannot Initialize Video Driver.\nTermination in Progress...\n");
		return GEM_ERROR;
	}
	printStatus("OK", LIGHT_GREEN);
	printMessage("Core", "Searching for KEY Importer...", WHITE);
	if(!IsAvailable(IE_KEY_CLASS_ID)) {
		printStatus("ERROR", LIGHT_RED);
		printf("No KEY Importer Available.\nTermination in Progress...\n");
		return GEM_ERROR;
	}
	printStatus("OK", LIGHT_GREEN);
	printMessage("Core", "Initializing Resource Manager...", WHITE);
	key = (ResourceMgr*)GetInterface(IE_KEY_CLASS_ID);
	char ChitinPath[_MAX_PATH];
	strcpy(ChitinPath, GamePath);
	strcat(ChitinPath, "chitin.key");
	if(!key->LoadResFile(ChitinPath)) {
		printStatus("ERROR", LIGHT_RED);
		printf("Cannot Load Chitin.key\nTermination in Progress...\n");
		return GEM_ERROR;
	}
	printStatus("OK", LIGHT_GREEN);
	printMessage("Core", "Checking for Hard Coded Animations...", WHITE);
	if(!IsAvailable(IE_HCANIMS_CLASS_ID)) {
		printStatus("ERROR", LIGHT_RED);
		printf("No Hard Coded Animations Available.\nTermination in Progress...\n");
		return GEM_ERROR;
	}
	printStatus("OK", LIGHT_GREEN);
	printMessage("Core", "Initializing Hard Coded Animations...", WHITE);
	hcanims = (HCAnimationSeq*)GetInterface(IE_HCANIMS_CLASS_ID);
	printStatus("OK", LIGHT_GREEN);
	printMessage("Core", "Checking for Dialogue Manager...", WHITE);
	if(!IsAvailable(IE_TLK_CLASS_ID)) {
		printStatus("ERROR", LIGHT_RED);
		printf("No TLK Importer Available.\nTermination in Progress...\n");
		return GEM_ERROR;
	}
	printStatus("OK", LIGHT_GREEN);
	strings = (StringMgr*)GetInterface(IE_TLK_CLASS_ID);
	printMessage("Core", "Loading Dialog.tlk file...", WHITE);
	char strpath[_MAX_PATH];
	strcpy(strpath, GamePath);
	strcat(strpath, "dialog.tlk");
	FileStream * fs = new FileStream();
	if(!fs->Open(strpath)) {
		printStatus("ERROR", LIGHT_RED);
		printf("Cannot find Dialog.tlk.\nTermination in Progress...\n");
		delete(fs);
		return GEM_ERROR;
	}
	printStatus("OK", LIGHT_GREEN);
	strings->Open(fs, true);
	printMessage("Core", "Loading Palettes...\n", WHITE);
	DataStream * bmppal256 = NULL, * bmppal16 = NULL;
	if(stricmp(core->GameType, "bg1") != 0) {
		bmppal256 = key->GetResource("MPAL256\0", IE_BMP_CLASS_ID);
		pal256 = (ImageMgr*)this->GetInterface(IE_BMP_CLASS_ID);
		pal256->Open(bmppal256, true);
	}
	else
		pal256 = NULL;
	bmppal16 = key->GetResource("MPALETTE", IE_BMP_CLASS_ID);
	pal16  = (ImageMgr*)this->GetInterface(IE_BMP_CLASS_ID);
	pal16->Open(bmppal16, true);
	printMessage("Core", "Palettes Loaded\n", LIGHT_GREEN);
	printMessage("Core", "Loading Fonts...\n", WHITE);
	AnimationMgr * anim = (AnimationMgr*)GetInterface(IE_BAM_CLASS_ID);
	int table = LoadTable("fonts");
	if(table == -1) {
		printStatus("ERROR", LIGHT_RED);
		printf("Cannot find fonts.2da.\nTermination in Progress...\n");
		return GEM_ERROR;
	}
	else {
		TableMgr * tab = GetTable(table);
		int count = tab->GetRowCount();
		for(int i = 0; i < count; i++) {
			char * ResRef = tab->QueryField(i, 0);
			int needpalette = atoi(tab->QueryField(i, 1));
			DataStream * fstr = key->GetResource(ResRef, IE_BAM_CLASS_ID);
			if(!anim->Open(fstr, true)) {
					printStatus("ERROR", LIGHT_RED);
					delete(fstr);
					continue;
			}
			Font * fnt = anim->GetFont();
			strncpy(fnt->ResRef, ResRef, 8);
			if(needpalette) {
				Color fore = {0xff, 0xff, 0xff, 0x00}, back = {0x00, 0x00, 0x00, 0x00};
				Color * pal = core->GetVideoDriver()->CreatePalette(fore, back);
		        memcpy(fnt->GetPalette(), pal, 256*sizeof(Color));
				free(pal);
			}
			fonts.push_back(fnt);
		}
		DelTable(table);
	}
	FreeInterface(anim);
	printMessage("Core", "Fonts Loaded\n", LIGHT_GREEN);
	printMessage("Core", "Initializing the Event Manager...", WHITE);
	evntmgr = new EventMgr();
	printStatus("OK", LIGHT_GREEN);
	printMessage("Core", "BroadCasting Event Manager...", WHITE);
	video->SetEventMgr(evntmgr);
	printStatus("OK", LIGHT_GREEN);
	printMessage("Core", "Initializing Window Manager...", YELLOW);
	windowmgr = (WindowMgr*)GetInterface(IE_CHU_CLASS_ID);
	if(windowmgr == NULL) {
		printStatus("ERROR", LIGHT_RED);
		return GEM_ERROR;
	}
	printStatus("OK", LIGHT_GREEN);
	printMessage("Core", "Initializing GUI Script Engine...", YELLOW);
	guiscript = (ScriptEngine*)GetInterface(IE_GUI_SCRIPT_CLASS_ID);
	if(guiscript == NULL) {
		printStatus("ERROR", LIGHT_RED);
		return GEM_ERROR;
	}
	if(!guiscript->Init()) {
		
		printStatus("ERROR", LIGHT_RED);
		return GEM_ERROR;
	}
	printStatus("OK", LIGHT_GREEN);
	strcpy(NextScript, "Start");
	printMessage("Core", "Setting up the Console...", WHITE);
	ChangeScript = true;
	console = new Console();
	console->XPos = 0;
	console->YPos = core->Height-25;
	console->Width = core->Width;
	console->Height = 25;
	console->SetFont(fonts[0]);
	AnimationFactory *af=(AnimationFactory*) GetResourceMgr()->GetFactoryResource("CAROT",IE_BAM_CLASS_ID);
	if(af)
		console->SetCursor(af->GetFrame(0));
	printStatus("OK", LIGHT_GREEN);
	printMessage("Core", "Starting up the Sound Manager...", WHITE);
	soundmgr = (SoundMgr*)GetInterface(IE_WAV_CLASS_ID);
	if(soundmgr == NULL) {
		printStatus("ERROR", LIGHT_RED);
		return GEM_ERROR;
	}
	if(!soundmgr->Init()) {
		printStatus("ERROR", LIGHT_RED);
		return GEM_ERROR;
	}
	printStatus("OK", LIGHT_GREEN);

	printMessage("Core", "Allocating SaveGameIterator...", WHITE);
	sgiterator=new SaveGameIterator();
	if(sgiterator==NULL) {
		printStatus("ERROR", LIGHT_RED);
		return GEM_ERROR;
	}
	printStatus("OK", LIGHT_GREEN);

	printMessage("Core", "Initializing Variables Dictionary...", WHITE);
	vars = new Variables();
	if(!vars) {
		printStatus("ERROR", LIGHT_RED);
		return GEM_ERROR;
	}
	vars->SetType(GEM_VARIABLES_INT);
	{
	char ini_path[_MAX_PATH];
	strcpy(ini_path,GamePath);
	strcat(ini_path,INIConfig);
	LoadINI(ini_path);
	}
	printStatus("OK", LIGHT_GREEN);

	printMessage("Core", "Initializing Token Dictionary...", WHITE);
	tokens = new Variables();
	if(!tokens) {
		printStatus("ERROR", LIGHT_RED);
		return GEM_ERROR;
	}
	tokens->SetType(GEM_VARIABLES_STRING);
	printStatus("OK", LIGHT_GREEN);

	printMessage("Core", "Initializing Music Manager...", WHITE);
	music = (MusicMgr*)GetInterface(IE_MUS_CLASS_ID);
	if(!music) {
		printStatus("ERROR", LIGHT_RED);
		return GEM_ERROR;
	}
	printStatus("OK", LIGHT_GREEN);
	if(stricmp(GameType, "iwd2") == 0) {
		printMessage("Core", "Loading IceWind Dale 2 Extension Files...", YELLOW);
		INIparty = (DataFileMgr*)GetInterface(IE_INI_CLASS_ID);
		FileStream * fs = new FileStream();
		char tINIparty[_MAX_PATH];
		strcpy(tINIparty, GamePath);
		strcat(tINIparty, "Party.ini");
		fs->Open(tINIparty, true);
		if(!INIparty->Open(fs, true)) {
			printStatus("ERROR", LIGHT_RED);
		}
		else {
			printStatus("OK", LIGHT_GREEN);
		}
	}
	game = new Game();
	printMessage("Core", "Loading Cursors...", WHITE);
	anim = (AnimationMgr*)GetInterface(IE_BAM_CLASS_ID);
	DataStream * str = GetResourceMgr()->GetResource("CURSORS", IE_BAM_CLASS_ID);
	anim->Open(str, true);
	for(int i = 0; i < 48; i++) {
		Cursors[i] = anim->GetAnimation(i, 0, 0);
	}
	FreeInterface(anim);
	video->SetCursor(Cursors[0]->GetFrame(0), Cursors[1]->GetFrame(0));
	printMessage("Core", "Initializing A* PathFinder...\n", WHITE);
	pathfinder = new PathFinder();
	printStatus("OK", LIGHT_GREEN);
	printMessage("Core", "Core Initialization Complete!\n", LIGHT_GREEN);
	return GEM_OK;
}

bool Interface::IsAvailable(SClass_ID filetype)
{
	return plugin->IsAvailable(filetype);
}

void * Interface::GetInterface(SClass_ID filetype)
{
	return plugin->GetPlugin(filetype);
}

Video * Interface::GetVideoDriver()
{
	return video;
}

ResourceMgr * Interface::GetResourceMgr()
{
	return key;
}

const char * Interface::TypeExt(SClass_ID type)
{
	switch(type) {
		case IE_2DA_CLASS_ID:
			return ".2DA";

		case IE_ACM_CLASS_ID:
			return ".ACM";

		case IE_ARE_CLASS_ID:
			return ".ARE";

		case IE_BAM_CLASS_ID:
			return ".BAM";

		case IE_BCS_CLASS_ID:
			return ".BCS";

		case IE_BIF_CLASS_ID:
			return ".BIF";

		case IE_BMP_CLASS_ID:
			return ".BMP";

		case IE_CHR_CLASS_ID:
			return ".CHR";

		case IE_CHU_CLASS_ID:
			return ".CHU";

		case IE_CRE_CLASS_ID:
			return ".CRE";

		case IE_DLG_CLASS_ID:
			return ".DLG";

		case IE_EFF_CLASS_ID:
			return ".EFF";

		case IE_GAM_CLASS_ID:
			return ".GAM";

		case IE_IDS_CLASS_ID:
			return ".IDS";

		case IE_INI_CLASS_ID:
			return ".INI";

		case IE_ITM_CLASS_ID:
			return ".ITM";

		case IE_KEY_CLASS_ID:
			return ".KEY";

		case IE_MOS_CLASS_ID:
			return ".MOS";

		case IE_MUS_CLASS_ID:
			return ".MUS";

		case IE_MVE_CLASS_ID:
			return ".MVE";

		case IE_PLT_CLASS_ID:
			return ".PLT";

		case IE_PRO_CLASS_ID:
			return ".PRO";

		case IE_SAV_CLASS_ID:
			return ".SAV";

		case IE_SPL_CLASS_ID:
			return ".SPL";

		case IE_SRC_CLASS_ID:
			return ".SRC";

		case IE_STO_CLASS_ID:
			return ".STO";

		case IE_TIS_CLASS_ID:
			return ".TIS";

		case IE_TLK_CLASS_ID:
			return ".TLK";

		case IE_TOH_CLASS_ID:
			return ".TOH";

		case IE_TOT_CLASS_ID:
			return ".TOT";

		case IE_VAR_CLASS_ID:
			return ".VAR";

		case IE_VVC_CLASS_ID:
			return ".VVC";

		case IE_WAV_CLASS_ID:
			return ".WAV";

		case IE_WED_CLASS_ID:
			return ".WED";

		case IE_WFX_CLASS_ID:
			return ".WFX";

		case IE_WMP_CLASS_ID:
			return ".WMP";
	}
	return NULL;
}

char * Interface::GetString(unsigned long strref)
{
	unsigned long flags=0;

	vars->Lookup("Strref On",flags);
	return strings->GetString(strref, flags);
}

void Interface::GetHCAnim(Actor * act)
{
	hcanims->GetCharAnimations(act);
}

void Interface::FreeInterface(void * ptr)
{
	plugin->FreePlugin(ptr);
}
Factory * Interface::GetFactory(void)
{
	return factory;
}

int Interface::SetFeature(int flag, int position)
{
	if(flag) GameFeatures|=1<<position;
	else GameFeatures&=~(1<<position);
	return GameFeatures;
}
int Interface::HasFeature(int position)
{
	return GameFeatures&(1<<position);
}

bool Interface::LoadConfig(void)
{
	FILE * config;
	config = fopen("GemRB.cfg", "rb");
	if(config == NULL)
		return false;
	char name[65], value[_MAX_PATH+3];
	while(!feof(config)) {
		char rem;
		fread(&rem, 1, 1, config);
		if(rem == '#') {
			fscanf(config, "%*[^\r\n]%*[\r\n]");
			continue;
		}
		fseek(config, -1, SEEK_CUR);
		fscanf(config, "%[^=]=%[^\r\n]%*[\r\n]", name, value);
		if(stricmp(name, "Width") == 0) {
			Width = atoi(value);
		}
		else if(stricmp(name, "Height") == 0) {
			Height = atoi(value);
		}
		else if(stricmp(name, "Bpp") == 0) {
			Bpp = atoi(value);
		}
		else if(stricmp(name, "FullScreen") == 0) {
			FullScreen = (atoi(value) == 0) ? false : true;
		}
		else if(stricmp(name, "CaseSensitive") == 0) {
			CaseSensitive = (atoi(value) == 0) ? false : true;
		}
		else if(stricmp(name, "ScrollBarPatch") == 0) {
			SetFeature(atoi(value),GF_SCROLLBAR_PATCH);
		}
		else if(stricmp(name, "AllStringsTagged") == 0) {
			SetFeature(atoi(value),GF_ALL_STRINGS_TAGGED);
		}
		else if(stricmp(name, "MidResAvatars") == 0) {
			SetFeature(atoi(value),GF_MID_RES_AVATARS);
		}
		else if(stricmp(name, "HasSonglist") == 0) {
			SetFeature(atoi(value),GF_HAS_SONGLIST);
		}
		else if(stricmp(name, "UpperButtonText") == 0) {
			SetFeature(atoi(value),GF_UPPER_BUTTON_TEXT);
		}
		else if(stricmp(name, "LowerLabelText") == 0) {
			SetFeature(atoi(value),GF_LOWER_LABEL_TEXT);
		}
		else if(stricmp(name, "ForceStereo") == 0) {
			ForceStereo = atoi(value);
		}
		else if(stricmp(name, "ButtonFont") ==0) {
			strncpy(ButtonFont, value, 8);
		}
		else if(stricmp(name, "GameType") == 0) {
			strcpy(GameType, value);
		}
		else if(stricmp(name, "GemRBPath") == 0) {
			strcpy(GemRBPath, value);
		}
		else if(stricmp(name, "CachePath") == 0) {
			strcpy(CachePath, value);
			struct stat mystat;
			stat(CachePath, &mystat);
			if(!(mystat.st_mode&S_IFDIR) ) {
				printf("%s folder doesn't exist!",CachePath);
				fclose(config);
				return false;
			}
		}
		else if(stricmp(name, "GUIScriptsPath") == 0) {
			strcpy(GUIScriptsPath, value);
		}
		else if(stricmp(name, "GamePath") == 0) {
			strcpy(GamePath, value);
		}
		else if(stricmp(name, "INIConfig") == 0) {
			strcpy(INIConfig, value);
		}
		else if(stricmp(name, "CD1") == 0) {
			strcpy(CD1, value);
		}
		else if(stricmp(name, "CD2") == 0) {
			strcpy(CD2, value);
		}
		else if(stricmp(name, "CD3") == 0) {
			strcpy(CD3, value);
		}
		else if(stricmp(name, "CD4") == 0) {
			strcpy(CD4, value);
		}
		else if(stricmp(name, "CD5") == 0) {
			strcpy(CD5, value);
		}
		else if(stricmp(name, "CD6") == 0) {
			strcpy(CD6, value);
		}
	}
	fclose(config);
	if(!GUIScriptsPath[0])
		memcpy(GUIScriptsPath,GemRBPath,sizeof(GUIScriptsPath));
	return true;
}
/** No descriptions */
Color * Interface::GetPalette(int index, int colors){
	Color * pal = NULL;
	if(colors <= 16) {
		pal = (Color*)malloc(colors*sizeof(Color));
		pal16->GetPalette(index, colors, pal);
	}
	else if(colors <= 256) {
		pal = (Color*)malloc(colors*sizeof(Color));
		pal256->GetPalette(index, colors, pal);
	}
	return pal;
}
/** Returns a preloaded Font */
Font * Interface::GetFont(char * ResRef)
{
	printf("Searching Font %.8s...", ResRef);
	for(unsigned int i = 0; i < fonts.size(); i++) {
		if(strncmp(fonts[i]->ResRef, ResRef, 8) == 0) {
			printf("[FOUND]\n");
			return fonts[i];
		}
	}
	printf("[NOT FOUND]\n");
	return NULL;
}

Font * Interface::GetFont(unsigned int index)
{
	if(index >= fonts.size())
		return NULL;
	return fonts[index];
}

Font * Interface::GetButtonFont()
{
	return GetFont(ButtonFont);
}

/** Returns the Event Manager */
EventMgr * Interface::GetEventMgr()
{
	return evntmgr;
}

/** Returns the Window Manager */
WindowMgr * Interface::GetWindowMgr()
{
	return windowmgr;
}

/** Get GUI Script Manager */
ScriptEngine * Interface::GetGUIScriptEngine()
{
	return guiscript;
}

int Interface::UnloadCreature(unsigned int Slot)
{
	if(Slot>=actors.size())
		return 0;
	if(!actors[Slot])
		return 0;
	delete actors[Slot];
	actors[Slot]=NULL;
	return 1;
}

Actor *Interface::GetActor(unsigned int Slot)
{
	if(Slot>=actors.size())
		return 0;
	if(!actors[Slot])
		return 0;
	return actors[Slot];
}

int Interface::LoadCreature(char *ResRef, int InParty)
{
	ActorMgr *actormgr=(ActorMgr *) GetInterface(IE_CRE_CLASS_ID);
	DataStream *stream=GetResourceMgr()->GetResource(ResRef,IE_CRE_CLASS_ID);
	if(!actormgr->Open(stream, true))
	{
		FreeInterface(actormgr);
		return 0;
	}
	Actor *actor=actormgr->GetActor();
	FreeInterface(actormgr);
	actor->InParty=InParty;
	size_t index;
	for(index=0;index<actors.size(); index++) {
		if(!actors[index]) break;
        }
	if(index==actors.size() )
		actors.push_back(actor);
	return index;
}

int Interface::FindPlayer(int PartySlotCount)
{
	int index=actors.size();
	while(index--) {
		if(!actors[index])
			continue;
		if(actors[index]->InParty ) {
			break;
		}
	}
	return index;
}

int Interface::GetCreatureStat(unsigned int Slot, unsigned int StatID, int Mod)
{
	if(Slot>=actors.size())
		return 0xdadadada;
	if(!actors[Slot])
		return 0xdadadada;
	if(Mod)
		return actors[Slot]->GetStat(StatID);
	return actors[Slot]->GetBase(StatID);
}

int Interface::SetCreatureStat(unsigned int Slot, unsigned int StatID, int StatValue, int Mod)
{
	if(Slot>=actors.size())
		return 0;
	if(!actors[Slot])
		return 0;
	if(Mod)
		actors[Slot]->SetStat(StatID, StatValue);
	else
		actors[Slot]->SetBase(StatID, StatValue);
	return 1;
}

void Interface::RedrawAll()
{
	for(unsigned int i = 0; i < windows.size(); i++) {
		if(windows[i]!=NULL)
			windows[i]->Invalidate();
	}
}

/** Loads a Window in the Window Manager */
int Interface::LoadWindow(unsigned short WindowID)
{
	for(unsigned int i = 0; i < windows.size(); i++) {
		if(windows[i]==NULL)
			continue;
		if(windows[i]->WindowID == WindowID) {
			SetOnTop(i);
			windows[i]->Invalidate();
			return i;
		}
	}
	Window * win = windowmgr->GetWindow(WindowID);
	if(win == NULL)
		return -1;
	int slot = -1;
	for(unsigned int i = 0; i < windows.size(); i++) {
		if(windows[i]==NULL) {
			slot = i;
			break;
		}
	}
	if(slot == -1) {
		windows.push_back(win);
		slot=windows.size()-1;
	}
	else
		windows[slot] = win;
	win->Invalidate();
	return slot;
}

/** Get a Control on a Window */
int Interface::GetControl(unsigned short WindowIndex, unsigned long ControlID)
{
	if(WindowIndex >= windows.size())
		return -1;
	Window * win = windows[WindowIndex];
	if(win==NULL)
		return -1;
	int i = 0;
	while(true) {
		Control * ctrl = win->GetControl(i);
		if(ctrl == NULL)
			return -1;
		if(ctrl->ControlID == ControlID)
			return i;
		i++;
	}
}
/** Set the Text of a Control */
int Interface::SetText(unsigned short WindowIndex, unsigned short ControlIndex, const char * string)
{
	if(WindowIndex >= windows.size())
		return -1;
	Window * win = windows[WindowIndex];
	if(win == NULL)
		return -1;
	Control * ctrl = win->GetControl(ControlIndex);
	if(ctrl == NULL)
		return -1;
	return ctrl->SetText(string);
}

/** Set a Window Visible Flag */
int Interface::SetVisible(unsigned short WindowIndex, int visible)
{
	if(WindowIndex >= windows.size())
		return -1;
	Window * win = windows[WindowIndex];
	if(win==NULL)
		return -1;
	switch(visible) {
		case 0:
		case 2:
			{
			win->Visible = false;
			Region r(win->XPos, win->YPos, win->Width, win->Height);
			Color black = {0, 0, 0, 128};
			evntmgr->DelWindow(win->WindowID);
			if(visible == 2)
				video->DrawRect(r, black);
			}
		break;

		case 1:
			{
			win->Visible = true;
			evntmgr->AddWindow(win);
			win->Invalidate();
			SetOnTop(WindowIndex);
			}
		break;

		default:
			win->Visible = (visible != 0);
		break;
	}
	return 0;
}

/** Set an Event of a Control */
int Interface::SetEvent(unsigned short WindowIndex, unsigned short ControlIndex, unsigned long EventID, char * funcName)
{
	if(WindowIndex >= windows.size())
	{
		printMessage("Core","Window not found",LIGHT_RED);
		return -1;
	}
	Window * win = windows[WindowIndex];
	if(win==NULL)
	{
		printMessage("Core","Window already freed", LIGHT_RED);
		return -1;
	}
	Control * ctrl = win->GetControl(ControlIndex);
	if(ctrl == NULL)
	{
		printMessage("Core","Control not found", LIGHT_RED);
		return -1;
	}
	if(ctrl->ControlType!=(EventID>>24) )
	{
		printf("Expected control: %0x, but got: %0x",EventID>>24,ctrl->ControlType);
		printStatus("ERROR", LIGHT_RED);
		return -1;
	}
	switch(ctrl->ControlType) {
		case IE_GUI_BUTTON: //Button
			{
			Button * btn = (Button*)ctrl;
			btn->SetEvent(funcName);
			return 0;
			}
		break;

		case IE_GUI_EDIT: //Slider
			{
			TextEdit * te = (TextEdit*)ctrl;
			strncpy(te->EditOnChange, funcName,64);
			return 0;
			}
		break;

		case IE_GUI_SLIDER: //Slider
			{
			Slider * sld = (Slider*)ctrl;
			strncpy(sld->SliderOnChange, funcName,64);
			return 0;
			}
		break;

		case IE_GUI_SCROLLBAR: //ScrollBar
			{
			ScrollBar * sb = (ScrollBar*)ctrl;
			strncpy(sb->ScrollBarOnChange, funcName,64);
			return 0;
			}
		break;
	}
	printf("Control has no event implemented: %0x", ctrl->ControlID);
	printStatus("ERROR", LIGHT_RED);
	return -1;
}

/** Set the Status of a Control in a Window */
int Interface::SetControlStatus(unsigned short WindowIndex, unsigned short ControlIndex, unsigned long Status)
{
	if(WindowIndex >= windows.size())
		return -1;
	Window * win = windows[WindowIndex];
	if(win==NULL)
		return -1;
	Control * ctrl = win->GetControl(ControlIndex);
	if(ctrl == NULL)
		return -1;
	switch((Status & 0xff000000) >> 24) {
		case 0: //Button
		{
			if(ctrl->ControlType != 0)
				return -1;
			Button * btn = (Button*)ctrl;
			btn->SetState(Status & 0xff);
			return 0;
		}
		break;
	}
	return -1;
}

/** Show a Window in Modal Mode */
int Interface::ShowModal(unsigned short WindowIndex)
{
	if(WindowIndex >= windows.size())
	{
		printMessage("Core","Window not found", LIGHT_RED);
		return -1;
	}
	Window * win = windows[WindowIndex];
	if(win==NULL)
	{
		printMessage("Core","Window already freed", LIGHT_RED);
		return -1;
	}
	win->Visible = true;
	evntmgr->Clear();
	SetOnTop(WindowIndex);
	evntmgr->AddWindow(win);
	win->Invalidate();
	return 0;
}

void Interface::DrawWindows(void)
{
	std::vector<int>::reverse_iterator t;
	for(t = topwin.rbegin(); t != topwin.rend(); ++t) {
		if(windows[(*t)]!=NULL && windows[(*t)]->Visible)
			windows[(*t)]->DrawWindow();
	}
	//for(unsigned int i = 0; i < windows.size(); i++) {
	//	if(windows[i]!=NULL && windows[i]->Visible)
	//		windows[i]->DrawWindow();
	//}
}

Window * Interface::GetWindow(unsigned short WindowIndex)
{
	if(WindowIndex < windows.size())
		return windows[WindowIndex];
	return NULL;
}

int Interface::DelWindow(unsigned short WindowIndex) 
{
	if(WindowIndex >= windows.size())
		return -1;
	Window * win = windows[WindowIndex];
	if(win==NULL)
	{
		printMessage("Core","Window deleted again",LIGHT_RED);
		return -1;
	}
	evntmgr->DelWindow(win->WindowID);
	delete(win);
	windows[WindowIndex]=NULL;
	std::vector<int>::iterator t;
	for(t = topwin.begin(); t != topwin.end(); ++t) {
		if((*t) == WindowIndex) {
			topwin.erase(t);
			break;
		}
	}
	return 0;
}

/** Popup the Console */
void Interface::PopupConsole()
{
	ConsolePopped = !ConsolePopped;
	RedrawAll();
	console->Changed = true;
}

/** Draws the Console */
void Interface::DrawConsole()
{
	console->Draw(0,0);
}

/** Get the Sound Manager */
SoundMgr * Interface::GetSoundMgr()
{
	return soundmgr;
}
/** Get the Sound Manager */
SaveGameIterator * Interface::GetSaveGameIterator()
{
	return sgiterator;
}
/** Sends a termination signal to the Video Driver */
bool Interface::Quit(void)
{
	return video->Quit();
}
/** Returns the variables dictionary */
Variables * Interface::GetDictionary()
{
	return vars;
}
/** Returns the token dictionary */
Variables * Interface::GetTokenDictionary()
{
	return tokens;
}
/** Get the Music Manager */
MusicMgr * Interface::GetMusicMgr()
{
	return music;
}
/** Loads a 2DA Table, returns -1 on error or the Table Index on success */
int Interface::LoadTable(const char * ResRef)
{
	int ind = GetIndex(ResRef);
	if(ind != -1)
		return ind;
	printf("(%s) Table not found... Loading from file\n", ResRef);
	DataStream * str = key->GetResource(ResRef, IE_2DA_CLASS_ID);
	if(!str)
		return -1;
	TableMgr * tm = (TableMgr*)GetInterface(IE_2DA_CLASS_ID);
	if(!tm) {
		delete(str);
		return -1;
	}
	if(!tm->Open(str, true)) {
		core->FreeInterface(tm);
		return -1;
	}
	Table t;
	t.free = false;
	strncpy(t.ResRef, ResRef, 8);
	t.tm = tm;
	ind = -1;
	for(size_t i = 0; i < tables.size(); i++) {
		if(tables[i].free) {
			ind = i;
			break;
		}
	}
	if(ind != -1) {
		tables[ind] = t;
		return ind;
	}
	tables.push_back(t);
	return tables.size()-1;
}
/** Gets the index of a loaded table, returns -1 on error */
int Interface::GetIndex(const char * ResRef)
{
	for(size_t i = 0; i < tables.size(); i++) {
		if(tables[i].free)
			continue;
		if(strnicmp(tables[i].ResRef, ResRef, 8) == 0)
			return i;
	}
	return -1;
}
/** Gets a Loaded Table by its index, returns NULL on error */
TableMgr * Interface::GetTable(unsigned int index)
{
	if(index >= tables.size())
		return NULL;
	if(tables[index].free)
		return NULL;
	return tables[index].tm;
}
/** Frees a Loaded Table, returns false on error, true on success */
bool Interface::DelTable(unsigned int index)
{
	if(index >= tables.size())
		return false;
	if(tables[index].free)
		return false;
	core->FreeInterface(tables[index].tm);
	tables[index].free = true;
	return true;
}
/** Loads an IDS Table, returns -1 on error or the Symbol Table Index on success */
int Interface::LoadSymbol(const char * ResRef)
{
	int ind = GetSymbolIndex(ResRef);
	if(ind != -1)
		return ind;
	DataStream * str = key->GetResource(ResRef, IE_IDS_CLASS_ID);
	if(!str)
		return -1;
	SymbolMgr * sm = (SymbolMgr*)GetInterface(IE_IDS_CLASS_ID);
	if(!sm) {
		delete(str);
		return -1;
	}
	if(!sm->Open(str, true)) {
		core->FreeInterface(sm);
		return -1;
	}
	Symbol s;
	s.free = false;
	strncpy(s.ResRef, ResRef, 8);
	s.sm = sm;
	ind = -1;
	for(size_t i = 0; i < symbols.size(); i++) {
		if(symbols[i].free) {
			ind = i;
			break;
		}
	}
	if(ind != -1) {
		symbols[ind] = s;
		return ind;
	}
	symbols.push_back(s);
	return symbols.size()-1;	
}
/** Gets the index of a loaded Symbol Table, returns -1 on error */
int Interface::GetSymbolIndex(const char * ResRef)
{
	for(size_t i = 0; i < symbols.size(); i++) {
		if(symbols[i].free)
			continue;
		if(strnicmp(symbols[i].ResRef, ResRef, 8) == 0)
			return i;
	}
	return -1;
}
/** Gets a Loaded Symbol Table by its index, returns NULL on error */
SymbolMgr * Interface::GetSymbol(unsigned int index)
{
	if(index >= symbols.size())
		return NULL;
	if(symbols[index].free)
		return NULL;
	return symbols[index].sm;
}
/** Frees a Loaded Symbol Table, returns false on error, true on success */
bool Interface::DelSymbol(unsigned int index)
{
	if(index >= symbols.size())
		return false;
	if(symbols[index].free)
		return false;
	core->FreeInterface(symbols[index].sm);
	symbols[index].free = true;
	return true;
}
/** Plays a Movie */
int Interface::PlayMovie(char * ResRef)
{
	MoviePlayer * mp = (MoviePlayer*)GetInterface(IE_MVE_CLASS_ID);
	if(!mp)
		return 0;
	DataStream * str = core->GetResourceMgr()->GetResource(ResRef, IE_MVE_CLASS_ID);
	if(!str) {
		core->FreeInterface(mp);
		return -1;
	}
	if(!mp->Open(str, true)) {
		core->FreeInterface(mp);
		delete(str);
		return -1;
	}
	mp->Play();
	core->FreeInterface(mp);
	return 0;
}

int Interface::Roll(int dice, int size, int add)
{
	if(dice<1)
		return 0;
	if(size<1)
		return 0;
	if(dice>100)
		return add+dice*size/2;
	for(int i=0;i<dice;i++) {
		add+=rand()%size+1;
	}
	return add;
}

bool Interface::LoadINI(const char * filename)
{
	FILE * config;
	config = fopen(filename, "rb");
	if(config == NULL)
		return false;
	char name[65], value[_MAX_PATH+3];
	while(!feof(config)) {
		name[0] = 0;
		value[0] = 0;
		char rem;
		fread(&rem, 1, 1, config);
		if((rem == '#') || (rem == '[') || (rem == '\r') || (rem == '\n') || (rem == ';')) {
			if(rem == '\r') {
				fgetc(config);
				continue;
			}
			else if(rem == '\n')
				continue;
			fscanf(config, "%*[^\r\n]%*[\r\n]");
			continue;
		}
		fseek(config, -1, SEEK_CUR);
		fscanf(config, "%[^=]=%[^\r\n]%*[\r\n]", name, value);
		if((value[0] >= '0') && (value[0] <= '9')) {
	        vars->SetAt(name, atoi(value));
		}
	}
	fclose(config);
	return true;
}
