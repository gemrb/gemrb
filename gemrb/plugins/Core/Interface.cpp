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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Interface.cpp,v 1.177 2004/07/25 11:38:35 edheldil Exp $
 *
 */

#include <config.h>

#ifndef INTERFACE
#define INTERFACE
#endif

#include "Interface.h"
#include "FileStream.h"
#include "AnimationMgr.h"
#include <stdlib.h>
#include <time.h>

#ifdef WIN32
#include <direct.h>
#include <io.h>
#else
#include <dirent.h>
#endif

#ifdef WIN32
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT
#endif

GEM_EXPORT Interface* core;

#ifdef WIN32
GEM_EXPORT HANDLE hConsole;
#endif

#include "../../includes/win32def.h"
#include "../../includes/globals.h"

//use DialogF.tlk if the protagonist is female, that's why we leave space
static char dialogtlk[] = "dialog.tlk\0";

Interface::Interface(int iargc, char** iargv)
{
	argc = iargc;
	argv = iargv;
#ifdef WIN32
	hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
#endif
	textcolor( LIGHT_WHITE );
	printf( "GemRB Core Version v%.1f.%d.%d Loading...\n",
		GEMRB_RELEASE / 1000.0, GEMRB_API_NUM, GEMRB_SDK_REV );
	video = NULL;
	key = NULL;
	strings = NULL;
	guiscript = NULL;
	windowmgr = NULL;
	vars = NULL;
	tokens = NULL;
	music = NULL;
	soundmgr = NULL;
	sgiterator = NULL;
	INIparty = NULL;
	INIbeasts = NULL;
	INIquests = NULL;
	game = NULL;
	timer = NULL;
	console = NULL;
	slotmatrix = NULL;
	slottypes = NULL;

	tooltip_x = 0;
	tooltip_y = 0;
	tooltip_text = NULL;
	
	pal16 = NULL;
	pal32 = NULL;
	pal256 = NULL;

	CursorCount = 0;
	Cursors = NULL;

	ConsolePopped = false;
	CheatFlag = false;
#ifndef WIN32
	CaseSensitive = true;  //this is the default value, so CD1/CD2 will be resolved
#else
	CaseSensitive = false;
#endif
	GameOnCD = false;
	SkipIntroVideos = false;
	DrawFPS = false;
	TooltipDelay = 100;
	GUIScriptsPath[0] = 0;
	GamePath[0] = 0;
	SavePath[0] = 0;
	GemRBPath[0] = 0;
	PluginsPath[0] = 0;
	GameName[0] = 0;
	memcpy( GameOverride, "override", 9 );
	memcpy( GameData, "data\0\0\0\0", 9 );
	strcpy( INIConfig, "baldur.ini" );
	memcpy( ButtonFont, "STONESML", 9 );
	memcpy( TooltipFont, "STONESML", 9 );
	memcpy( CursorBam, "CAROT\0\0\0", 9 );
	memcpy( GlobalScript, "BALDUR\0\0", 9 );
	memcpy( GlobalMap, "WORLDMAP", 9 );
	strcpy( Palette16, "MPALETTE" );
	strcpy( Palette32, "PAL32" );
	strcpy( Palette256, "MPAL256" );
	GameFeatures = 0;
}

#define FreeInterfaceVector(type, variable, member)   \
{  \
  std::vector<type>::iterator i;  \
  for(i = variable.begin(); i != variable.end(); ++i) { \
	if(!(*i).free) {  \
		FreeInterface((*i).member); \
		(*i).free = true; \
	} \
  } \
}

#define FreeResourceVector(type, variable)   \
{  \
	unsigned int i=variable.size(); \
	while(i--) { \
		if(variable[i]) { \
			delete variable[i]; \
		} \
	} \
	variable.clear(); \
}

Interface::~Interface(void)
{
	if (slotmatrix) {
		free( slotmatrix );
	}
	if (game) {
		delete( game );
	}
	if (music) {
		FreeInterface( music );
	}
	if (soundmgr) {
		FreeInterface( soundmgr );
	}
	if (sgiterator) {
		delete( sgiterator );
	}
	if (factory) {
		delete( factory );
	}
	if (Cursors) {
		for (int i = 0; i < CursorCount; i++) {
			if (Cursors[i])
				delete( Cursors[i] );
		}
		delete[] Cursors;
	}

	FreeResourceVector( Font, fonts );
	FreeResourceVector( Window, windows );
	if (console) {
		delete( console );
	}

	if (key) {
		FreeInterface( key );
	}	
	if (strings) {
		FreeInterface( strings );
	}
	if (pal256) {
		FreeInterface( pal256 );
	}
	if (pal32) {
		FreeInterface( pal32 );
	}
	if (pal16) {
		FreeInterface( pal16 );
	}

	if (windowmgr) {
		FreeInterface( windowmgr );
	}

	if (video) {
		FreeInterface( video );
	}

	if (timer) {
		delete( timer );
	}

	if (evntmgr) {
		delete( evntmgr );
	}
	if (guiscript) {
		delete( guiscript );
	}
	if (vars) {
		delete( vars );
	}
	if (tokens) {
		delete( tokens );
	}
	FreeInterfaceVector( Table, tables, tm );
	FreeInterfaceVector( Symbol, symbols, sm );

	if(INIquests) {
		FreeInterface(INIquests);
	}
	if(INIbeasts) {
		FreeInterface(INIbeasts);
	}
	if(INIparty) {
		FreeInterface(INIparty);
	}
	delete( plugin );
	//TODO: Clean the Cache and leave only .bif files
}


int Interface::Init()
{
	printMessage( "Core", "Loading Configuration File...", WHITE );
	if (!LoadConfig()) {
		printStatus( "ERROR", LIGHT_RED );
		printMessage( "Core",
			"Cannot Load Config File.\nTermination in Progress...\n", WHITE );
		exit( -1 );
	}
	printStatus( "OK", LIGHT_GREEN );
	printMessage( "Core", "Starting Plugin Manager...\n", WHITE );
	plugin = new PluginMgr( PluginsPath );
	printMessage( "Core", "Plugin Loading Complete...", WHITE );
	printStatus( "OK", LIGHT_GREEN );
	printMessage( "Core", "Creating Object Factory...", WHITE );
	factory = new Factory();
	printStatus( "OK", LIGHT_GREEN );
	time_t t;
	t = time( NULL );
	srand( ( unsigned int ) t );
#ifdef _DEBUG
	FileStreamPtrCount = 0;
	CachedFileStreamPtrCount = 0;
#endif
	printMessage( "Core", "GemRB Core Initialization...\n", WHITE );
	printMessage( "Core", "Searching for Video Driver...", WHITE );
	if (!IsAvailable( IE_VIDEO_CLASS_ID )) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "No Video Driver Available.\nTermination in Progress...\n" );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );
	printMessage( "Core", "Initializing Video Plugin...", WHITE );
	video = ( Video * ) GetInterface( IE_VIDEO_CLASS_ID );
	if (video->Init() == GEM_ERROR) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "Cannot Initialize Video Driver.\nTermination in Progress...\n" );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );
	printMessage( "Core", "Searching for KEY Importer...", WHITE );
	if (!IsAvailable( IE_KEY_CLASS_ID )) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "No KEY Importer Available.\nTermination in Progress...\n" );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );
	printMessage( "Core", "Initializing Resource Manager...\n", WHITE );
	key = ( ResourceMgr * ) GetInterface( IE_KEY_CLASS_ID );
	char ChitinPath[_MAX_PATH];
	PathJoin( ChitinPath, GamePath, "chitin.key", NULL );
	ResolveFilePath( ChitinPath );
	if (!key->LoadResFile( ChitinPath )) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "Cannot Load Chitin.key\nTermination in Progress...\n" );
		return GEM_ERROR;
	}
	if(!LoadGemRBINI())
	{
		printf( "Cannot Load INI\nTermination in Progress...\n" );
		return GEM_ERROR;
	}

	printMessage( "Core", "Checking for Dialogue Manager...", WHITE );
	if (!IsAvailable( IE_TLK_CLASS_ID )) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "No TLK Importer Available.\nTermination in Progress...\n" );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );
	strings = ( StringMgr * ) GetInterface( IE_TLK_CLASS_ID );
	printMessage( "Core", "Loading Dialog.tlk file...", WHITE );
	char strpath[_MAX_PATH];
	PathJoin( strpath, GamePath, dialogtlk, NULL );
	ResolveFilePath( strpath );
	FileStream* fs = new FileStream();
	if (!fs->Open( strpath, true )) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "Cannot find Dialog.tlk.\nTermination in Progress...\n" );
		delete( fs );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );
	strings->Open( fs, true );

	printMessage( "Core", "Loading Palettes...\n", WHITE );
	DataStream* bmppal16 = NULL;
	DataStream* bmppal32 = NULL;
	DataStream* bmppal256 = NULL;
	if (!IsAvailable( IE_BMP_CLASS_ID )) {
		printf( "No BMP Importer Available.\nTermination in Progress...\n" );
		return GEM_ERROR;
	}
	bmppal16 = key->GetResource( Palette16, IE_BMP_CLASS_ID );
	if (bmppal16) {
		pal16 = ( ImageMgr * ) GetInterface( IE_BMP_CLASS_ID );
		pal16->Open( bmppal16, true );
	} else {
		pal16 = NULL;
	}
	bmppal32 = key->GetResource( Palette32, IE_BMP_CLASS_ID );
	if (bmppal32) {
		pal32 = ( ImageMgr * )this->GetInterface( IE_BMP_CLASS_ID );
		pal32->Open( bmppal32, true );
	} else {
		pal32 = NULL;
	}
	bmppal256 = key->GetResource( Palette256, IE_BMP_CLASS_ID );
	if (bmppal256) {
		pal256 = ( ImageMgr * ) GetInterface( IE_BMP_CLASS_ID );
		pal256->Open( bmppal256, true );
	} else {
		pal256 = NULL;
	}
	printMessage( "Core", "Palettes Loaded\n", WHITE );

	printMessage( "Core", "Loading Fonts...\n", WHITE );
	if (!IsAvailable( IE_BAM_CLASS_ID )) {
		printf( "No BAM Importer Available.\nTermination in Progress...\n" );
		return GEM_ERROR;
	}
	AnimationMgr* anim = ( AnimationMgr* ) GetInterface( IE_BAM_CLASS_ID );
	if (!IsAvailable( IE_2DA_CLASS_ID )) {
		printf( "No 2DA Importer Available.\nTermination in Progress...\n" );
		return GEM_ERROR;
	}
	int table = LoadTable( "fonts" );
	if (table < 0) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "Cannot find fonts.2da.\nTermination in Progress...\n" );
		return GEM_ERROR;
	} else {
		TableMgr* tab = GetTable( table );
		int count = tab->GetRowCount();
		for (int i = 0; i < count; i++) {
			char* ResRef = tab->QueryField( i, 0 );
			int needpalette = atoi( tab->QueryField( i, 1 ) );
			DataStream* fstr = key->GetResource( ResRef, IE_BAM_CLASS_ID );
			if (!anim->Open( fstr, true )) {
				delete( fstr );
				continue;
			}
			Font* fnt = anim->GetFont();
			if (!fnt) {
				continue;
			}
			strncpy( fnt->ResRef, ResRef, 8 );
			if (needpalette) {
				Color fore = {0xff, 0xff, 0xff, 0x00},
					back = {0x00, 0x00, 0x00, 0x00};
				Color* pal = video->CreatePalette( fore, back );
				memcpy( fnt->GetPalette(), pal, 256 * sizeof( Color ) );
				video->FreePalette( pal );
			}
			fonts.push_back( fnt );
		}
		DelTable( table );
	}
	FreeInterface( anim );
	printMessage( "Core", "Fonts Loaded...", WHITE );
	printStatus( "OK", LIGHT_GREEN );
	printMessage( "Core", "Initializing the Event Manager...", WHITE );
	evntmgr = new EventMgr();
	printStatus( "OK", LIGHT_GREEN );
	printMessage( "Core", "BroadCasting Event Manager...", WHITE );
	video->SetEventMgr( evntmgr );
	printStatus( "OK", LIGHT_GREEN );
	printMessage( "Core", "Initializing Window Manager...", WHITE );
	windowmgr = ( WindowMgr * ) GetInterface( IE_CHU_CLASS_ID );
	if (windowmgr == NULL) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );
	printMessage( "Core", "Initializing GUI Script Engine...", WHITE );
	guiscript = ( ScriptEngine * ) GetInterface( IE_GUI_SCRIPT_CLASS_ID );
	if (guiscript == NULL) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	if (!guiscript->Init()) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );
	strcpy( NextScript, "Start" );
	AnimationFactory* af = ( AnimationFactory* )
	GetResourceMgr()->GetFactoryResource( CursorBam, IE_BAM_CLASS_ID );
	printMessage( "Core", "Setting up the Console...", WHITE );
	ChangeScript = true;
	console = new Console();
	console->XPos = 0;
	console->YPos = Height - 25;
	console->Width = Width;
	console->Height = 25;
	console->SetFont( fonts[0] );
	if (af) {
		console->SetCursor( af->GetFrame( 0 ) );
	}
	printStatus( "OK", LIGHT_GREEN );
	printMessage( "Core", "Starting up the Sound Manager...", WHITE );
	soundmgr = ( SoundMgr * ) GetInterface( IE_WAV_CLASS_ID );
	if (soundmgr == NULL) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	if (!soundmgr->Init()) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );

	printMessage( "Core", "Allocating SaveGameIterator...", WHITE );
	sgiterator = new SaveGameIterator();
	if (sgiterator == NULL) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );

	printMessage( "Core", "Initializing Variables Dictionary...", WHITE );
	vars = new Variables();
	if (!vars) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	vars->SetType( GEM_VARIABLES_INT ); {
		char ini_path[_MAX_PATH];
		PathJoin( ini_path, GamePath, INIConfig, NULL );
		LoadINI( ini_path );
		int i;
		for (i = 0; i < 8; i++) {
			if (INIConfig[i] == '.')
				break;
			GameNameResRef[i] = INIConfig[i];
		}
		GameNameResRef[i] = 0;
	}
	//no need of strdup, variables do copy the key!
	vars->SetAt( "SkipIntroVideos", (unsigned long)SkipIntroVideos );
	printStatus( "OK", LIGHT_GREEN );

	printMessage( "Core", "Initializing Token Dictionary...", WHITE );
	tokens = new Variables();
	if (!tokens) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	tokens->SetType( GEM_VARIABLES_STRING );
	printStatus( "OK", LIGHT_GREEN );

	printMessage( "Core", "Initializing Music Manager...", WHITE );
	music = ( MusicMgr * ) GetInterface( IE_MUS_CLASS_ID );
	if (!music) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );
	if (HasFeature( GF_HAS_PARTY_INI )) {
		printMessage( "Core", "Loading IceWind Dale 2 Extension Files...",
			WHITE );
		INIparty = ( DataFileMgr * ) GetInterface( IE_INI_CLASS_ID );
		FileStream* fs = new FileStream();
		char tINIparty[_MAX_PATH];
		PathJoin( tINIparty, GamePath, "Party.ini", NULL );
		ResolveFilePath( tINIparty );
		fs->Open( tINIparty, true );
		if (!INIparty->Open( fs, true )) {
			printStatus( "ERROR", LIGHT_RED );
		} else {
			printStatus( "OK", LIGHT_GREEN );
		}
	}
	if (HasFeature( GF_HAS_BEASTS_INI )) {
		printMessage( "Core", "Loading beasts definition File...",
			WHITE );
		INIbeasts = ( DataFileMgr * ) GetInterface( IE_INI_CLASS_ID );
		FileStream* fs = new FileStream();
		char tINIbeasts[_MAX_PATH];
		PathJoin( tINIbeasts, GamePath, "beast.ini", NULL );
		ResolveFilePath( tINIbeasts );
		// FIXME: crashes if file does not open
		fs->Open( tINIbeasts, true );
		if (!INIbeasts->Open( fs, true )) {
			printStatus( "ERROR", LIGHT_RED );
		} else {
			printStatus( "OK", LIGHT_GREEN );
		}

		printMessage( "Core", "Loading quests definition File...",
			WHITE );
		INIquests = ( DataFileMgr * ) GetInterface( IE_INI_CLASS_ID );
		FileStream* fs2 = new FileStream();
		char tINIquests[_MAX_PATH];
		PathJoin( tINIquests, GamePath, "quests.ini", NULL );
		ResolveFilePath( tINIquests );
		// FIXME: crashes if file does not open
		fs2->Open( tINIquests, true );
		if (!INIquests->Open( fs2, true )) {
			printStatus( "ERROR", LIGHT_RED );
		} else {
			printStatus( "OK", LIGHT_GREEN );
		}
	}
	game = NULL;//new Game();
	DataStream* str = GetResourceMgr()->GetResource( "CURSORS",
											IE_BAM_CLASS_ID );
	printMessage( "Core", "Loading Cursors...", WHITE );
	anim = ( AnimationMgr * ) GetInterface( IE_BAM_CLASS_ID );
	anim->Open( str, true );
	CursorCount = anim->GetCycleCount();
	Cursors = new Animation * [CursorCount];
	for (int i = 0; i < CursorCount; i++) {
		Cursors[i] = anim->GetAnimation( i, 0, 0 );
	}
	FreeInterface( anim );
	video->SetCursor( Cursors[0]->GetFrame( 0 ), Cursors[1]->GetFrame( 0 ) );
	printStatus( "OK", LIGHT_GREEN );
/*
	printMessage( "Core", "Initializing A* PathFinder...", WHITE );
	pathfinder = new PathFinder();
	printStatus( "OK", LIGHT_GREEN );
*/
	printMessage( "Core", "Bringing up the Global Timer...", WHITE );
	timer = new GlobalTimer();
	if (!timer) {
		printStatus( "ERROR", LIGHT_RED );
		return GEM_ERROR;
	}
	printStatus( "OK", LIGHT_GREEN );
	bool ret = InitItemTypes();
	printMessage( "Core", "Initializing Inventory Management...", WHITE );
	if(ret) {
		printStatus( "OK", LIGHT_GREEN );
	}
	else {
		printStatus( "ERROR", LIGHT_RED );
	}

	printMessage( "Core", "Core Initialization Complete!\n", WHITE );
	return GEM_OK;
}

bool Interface::IsAvailable(SClass_ID filetype)
{
	return plugin->IsAvailable( filetype );
}

void* Interface::GetInterface(SClass_ID filetype)
{
	return plugin->GetPlugin( filetype );
}

Video* Interface::GetVideoDriver()
{
	return video;
}

ResourceMgr* Interface::GetResourceMgr()
{
	return key;
}

const char* Interface::TypeExt(SClass_ID type)
{
	switch (type) {
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

char* Interface::GetString(unsigned long strref, unsigned long options)
{
	unsigned long flags = 0;

	vars->Lookup( "Strref On", flags );
	return strings->GetString( strref, flags | options );
}

void Interface::FreeInterface(void* ptr)
{
	plugin->FreePlugin( ptr );
}

Factory* Interface::GetFactory(void)
{
	return factory;
}

int Interface::SetFeature(int flag, int position)
{
	if (flag) {
		GameFeatures |= 1 << position;
	} else {
		GameFeatures &= ~( 1 << position );
	}
	return GameFeatures;
}
int Interface::HasFeature(int position)
{
	return GameFeatures & ( 1 << position );
}

/** Search directories and load a config file */
bool Interface::LoadConfig(void)
{
#ifndef WIN32
	char path[_MAX_PATH];
	char name[_MAX_PATH];

	// Find directory where user stores GemRB configurations (~/.gemrb).
	// FIXME: Create it if it does not exist
	// Use current dir if $HOME is not defined (or bomb out??)

	char* s = getenv( "HOME" );
	if (s) {
		strcpy( UserDir, s );
		strcat( UserDir, "/."PACKAGE"/" );
	} else {
		strcpy( UserDir, "./" );
	}

	// Find basename of this program. It does the same as basename (3),
	//   but that's probably missing on some archs 
	s = strrchr( argv[0], PathDelimiter );
	if (s) {
		s++;
	} else {
		s = argv[0];
	}

	strcpy( name, s );
	//if (!name[0])		// FIXME: could this happen?
	//  strcpy (name, PACKAGE);    // ugly hack


	// FIXME: temporary hack, to be deleted??
	if (LoadConfig( "GemRB.cfg" )) {
		return true;
	}

	PathJoin( path, UserDir, name, NULL );
	strcat( path, ".cfg" );

	if (LoadConfig( path )) {
		return true;
	}

#ifdef SYSCONFDIR
	PathJoin( path, SYSCONFDIR, name, NULL );
	strcat( path, ".cfg" );

	if (LoadConfig( path )) {
		return true;
	}
#endif

	// Don't try with default binary name if we have tried it already
	if (!strcmp( name, PACKAGE )) {
		return false;
	}

	PathJoin( path, UserDir, PACKAGE, NULL );
	strcat( path, ".cfg" );

	if (LoadConfig( path )) {
		return true;
	}

#ifdef SYSCONFDIR
	PathJoin( path, SYSCONFDIR, PACKAGE, NULL );
	strcat( path, ".cfg" );

	if (LoadConfig( path )) {
		return true;
	}
#endif

	return false;
#else   // WIN32
	strcpy( UserDir, ".\\" );
	return LoadConfig( "GemRB.cfg" );
#endif  // WIN32
}

bool Interface::LoadConfig(const char* filename)
{
	FILE* config;
	config = fopen( filename, "rb" );
	if (config == NULL) {
		return false;
	}
	char name[65], value[_MAX_PATH + 3];
	while (!feof( config )) {
		char rem;
		fread( &rem, 1, 1, config );
		if (rem == '#') {
			fscanf( config, "%*[^\r\n]%*[\r\n]" );
			continue;
		}
		fseek( config, -1, SEEK_CUR );
		fscanf( config, "%[^=]=%[^\r\n]%*[\r\n]", name, value );
		if (stricmp( name, "Width" ) == 0) {
			Width = atoi( value );
		} else if (stricmp( name, "Height" ) == 0) {
			Height = atoi( value );
		} else if (stricmp( name, "Bpp" ) == 0) {
			Bpp = atoi( value );
		} else if (stricmp( name, "FullScreen" ) == 0) {
			FullScreen = ( atoi( value ) == 0 ) ? false : true;
		} else if (stricmp( name, "SkipIntroVideos" ) == 0) {
			SkipIntroVideos = ( atoi( value ) == 0 ) ? false : true;
		} else if (stricmp( name, "DrawFPS" ) == 0) {
			DrawFPS = ( atoi( value ) == 0 ) ? false : true;
		} else if (stricmp( name, "CaseSensitive" ) == 0) {
			CaseSensitive = ( atoi( value ) == 0 ) ? false : true;
		} else if (stricmp( name, "GameOnCD" ) == 0) {
			GameOnCD = ( atoi( value ) == 0 ) ? false : true;
		} else if (stricmp( name, "TooltipDelay" ) == 0) {
			TooltipDelay = atoi( value );
		} else if (stricmp( name, "GameDataPath" ) == 0) {
			strncpy( GameData, value, 8 );
		} else if (stricmp( name, "GameOverridePath" ) == 0) {
			strncpy( GameOverride, value, 8 );
		} else if (stricmp( name, "GameName" ) == 0) {
			strcpy( GameName, value );
		} else if (stricmp( name, "GameType" ) == 0) {
			strcpy( GameType, value );
		} else if (stricmp( name, "GemRBPath" ) == 0) {
			strcpy( GemRBPath, value );
		} else if (stricmp( name, "CachePath" ) == 0) {
			strcpy( CachePath, value );
			if (! dir_exists( CachePath )) {
				printf( "Cache folder %s doesn't exist!", CachePath );
				fclose( config );
				return false;
			}
			strcat( CachePath, SPathDelimiter );
		} else if (stricmp( name, "GUIScriptsPath" ) == 0) {
			strcpy( GUIScriptsPath, value );
#ifndef WIN32
			ResolveFilePath( GUIScriptsPath );
#endif
		} else if (stricmp( name, "PluginsPath" ) == 0) {
			strcpy( PluginsPath, value );
#ifndef WIN32
			ResolveFilePath( PluginsPath );
#endif
		} else if (stricmp( name, "GamePath" ) == 0) {
			strcpy( GamePath, value );
#ifndef WIN32
			ResolveFilePath( GamePath );
#endif
		} else if (stricmp( name, "SavePath" ) == 0) {
			strcpy( SavePath, value );
#ifndef WIN32
			ResolveFilePath( SavePath );
#endif
		} else if (stricmp( name, "CD1" ) == 0) {
			strcpy( CD1, value );
#ifndef WIN32
			ResolveFilePath( CD1 );
#endif
		} else if (stricmp( name, "CD2" ) == 0) {
			strcpy( CD2, value );
#ifndef WIN32
			ResolveFilePath( CD2 );
#endif
		} else if (stricmp( name, "CD3" ) == 0) {
			strcpy( CD3, value );
#ifndef WIN32
			ResolveFilePath( CD3 );
#endif
		} else if (stricmp( name, "CD4" ) == 0) {
			strcpy( CD4, value );
#ifndef WIN32
			ResolveFilePath( CD4 );
#endif
		} else if (stricmp( name, "CD5" ) == 0) {
			strcpy( CD5, value );
#ifndef WIN32
			ResolveFilePath( CD5 );
#endif
		} else if (stricmp( name, "CD6" ) == 0) {
			strcpy( CD6, value );
#ifndef WIN32
			ResolveFilePath( CD6 );
#endif
		}
	}
	fclose( config );
#ifdef DATADIR
	if (!GemRBPath[0]) {
		strcpy( GemRBPath, DATADIR );
		strcat( GemRBPath, SPathDelimiter );
	}
#endif
	if (!PluginsPath[0]) {
#ifdef PLUGINDIR
		strcpy( PluginsPath, PLUGINDIR );
#else
		PathJoin( PluginsPath, GemRBPath, "plugins", NULL );
#endif
		strcat( PluginsPath, SPathDelimiter );
	}
	if (!GUIScriptsPath[0]) {
		memcpy( GUIScriptsPath, GemRBPath, sizeof( GUIScriptsPath ) );
	}
	if (!GameName[0]) {
		sprintf( GameName, "GemRB v%.1f.%d.%d", GEMRB_RELEASE / 1000.0,
			GEMRB_API_NUM, GEMRB_SDK_REV );
	}
	if (!SavePath[0]) {
		// FIXME: maybe should use UserDir instead of GamePath
		memcpy( SavePath, GamePath, sizeof( GamePath ) );
	}

	printf( "Loaded config file %s\n", filename );
	return true;
}

/** Loads gemrb.ini */
bool Interface::LoadGemRBINI()
{
	printMessage( "Core", "Loading game type-specific GemRB setup...",
		      WHITE );

	DataStream* inifile = core->GetResourceMgr()->GetResource( "gemrb", IE_INI_CLASS_ID );
	if (! inifile) {
		printStatus( "ERROR", LIGHT_RED );
		return false;
	}
	if (!core->IsAvailable( IE_INI_CLASS_ID )) {
		printStatus( "ERROR", LIGHT_RED );
		printf( "[Core]: No INI Importer Available.\n" );
		return false;
	}
	DataFileMgr* ini = ( DataFileMgr* ) GetInterface( IE_INI_CLASS_ID );
	ini->Open( inifile, true ); //autofree

	printStatus( "OK", LIGHT_GREEN );

	const char *s;

	// Resrefs are already initialized in Interface::Interface() 
	s = ini->GetKeyAsString( "resources", "CursorBAM", NULL );
	if (s)
		strcpy( CursorBam, s );

	s = ini->GetKeyAsString( "resources", "ButtonFont", NULL );
	if (s)
		strcpy( ButtonFont, s );

	s = ini->GetKeyAsString( "resources", "TooltipFont", NULL );
	if (s)
		strcpy( TooltipFont, s );

	s = ini->GetKeyAsString( "resources", "INIConfig", NULL );
	if (s)
		strcpy( INIConfig, s );
	
	s = ini->GetKeyAsString( "resources", "Palette16", NULL );
	if (s)
		strcpy( Palette16, s );

	s = ini->GetKeyAsString( "resources", "Palette32", NULL );
	if (s)
		strcpy( Palette32, s );

	s = ini->GetKeyAsString( "resources", "Palette256", NULL );
	if (s)
		strcpy( Palette256, s );


	SetFeature( ini->GetKeyAsInt( "resources", "IgnoreButtonFrames", 1 ), GF_IGNORE_BUTTON_FRAMES );
	SetFeature( ini->GetKeyAsInt( "resources", "AllStringsTagged", 1 ), GF_ALL_STRINGS_TAGGED );
	SetFeature( ini->GetKeyAsInt( "resources", "HasDPLAYER", 0 ), GF_HAS_DPLAYER );
	SetFeature( ini->GetKeyAsInt( "resources", "HasPickSound", 0 ), GF_HAS_PICK_SOUND );
	SetFeature( ini->GetKeyAsInt( "resources", "HasDescIcon", 0 ), GF_HAS_DESC_ICON );
	SetFeature( ini->GetKeyAsInt( "resources", "HasEXPTABLE", 0 ), GF_HAS_EXPTABLE );
	SetFeature( ini->GetKeyAsInt( "resources", "SoundFolders", 0 ), GF_SOUNDFOLDERS );
	SetFeature( ini->GetKeyAsInt( "resources", "HasSongList", 0 ), GF_HAS_SONGLIST );
	SetFeature( ini->GetKeyAsInt( "resources", "UpperButtonText", 0 ), GF_UPPER_BUTTON_TEXT );
	SetFeature( ini->GetKeyAsInt( "resources", "LowerLabelText", 0 ), GF_LOWER_LABEL_TEXT );
	SetFeature( ini->GetKeyAsInt( "resources", "HasPartyIni", 0 ), GF_HAS_PARTY_INI );
	SetFeature( ini->GetKeyAsInt( "resources", "HasBeastsIni", 0 ), GF_HAS_BEASTS_INI );
	ForceStereo = ini->GetKeyAsInt( "resources", "ForceStereo", 0 );

	core->FreeInterface( ini );
	return true;
}

/** No descriptions */
Color* Interface::GetPalette(int index, int colors)
{
	Color* pal = NULL;
	if (colors == 32) {
		pal = ( Color * ) malloc( colors * sizeof( Color ) );
		pal32->GetPalette( index, colors, pal );
	} else if (colors <= 32) {
		pal = ( Color * ) malloc( colors * sizeof( Color ) );
		pal16->GetPalette( index, colors, pal );
	} else if (colors == 256) {
		pal = ( Color * ) malloc( colors * sizeof( Color ) );
		pal256->GetPalette( index, colors, pal );
	}
	return pal;
}
/** Returns a preloaded Font */
Font* Interface::GetFont(char* ResRef)
{
	//printf("Searching Font %.8s...", ResRef);
	for (unsigned int i = 0; i < fonts.size(); i++) {
		if (strncmp( fonts[i]->ResRef, ResRef, 8 ) == 0) {
			return fonts[i];
		}
	}
	return NULL;
}

Font* Interface::GetFont(unsigned int index)
{
	if (index >= fonts.size()) {
		return NULL;
	}
	return fonts[index];
}

Font* Interface::GetButtonFont()
{
	return GetFont( ButtonFont );
}

/** Returns the Event Manager */
EventMgr* Interface::GetEventMgr()
{
	return evntmgr;
}

/** Returns the Window Manager */
WindowMgr* Interface::GetWindowMgr()
{
	return windowmgr;
}

/** Get GUI Script Manager */
ScriptEngine* Interface::GetGUIScriptEngine()
{
	return guiscript;
}

int Interface::LoadCreature(char* ResRef, int InParty)
{
	ActorMgr* actormgr = ( ActorMgr* ) GetInterface( IE_CRE_CLASS_ID );
	DataStream* stream = GetResourceMgr()->GetResource( ResRef,
											IE_CRE_CLASS_ID );
	if (!actormgr->Open( stream, true )) {
		FreeInterface( actormgr );
		return 0;
	}
	Actor* actor = actormgr->GetActor();
	FreeInterface( actormgr );
	actor->FromGame = true;
	actor->InParty = InParty;
	actor->AnimID = IE_ANI_AWAKE;
	//both fields are of length 9, make this sure!
	memcpy(actor->Area, GetGame()->CurrentArea, 9);
	if (actor->BaseStats[IE_STATE_ID] & STATE_DEAD) {
		actor->AnimID = IE_ANI_SLEEP;
	}
	actor->Orientation = 0;

	return game->SetPC( actor );
}

int Interface::GetCreatureStat(unsigned int Slot, unsigned int StatID, int Mod)
{
	Actor * actor = GetGame()->FindPC(Slot);
	if (!actor) {
		return 0xdadadada;
	}

	if (Mod) {
		return actor->GetStat( StatID );
	}
	return actor->GetBase( StatID );
}

int Interface::SetCreatureStat(unsigned int Slot, unsigned int StatID,
	int StatValue, int Mod)
{
	Actor * actor = GetGame()->FindPC(Slot);
	if (!actor) {
		return 0;
	}
	if (Mod) {
		actor->SetStat( StatID, StatValue );
	} else {
		actor->SetBase( StatID, StatValue );
	}
	return 1;
}

void Interface::RedrawAll()
{
	for (unsigned int i = 0; i < windows.size(); i++) {
		if (windows[i] != NULL) {
			windows[i]->Invalidate();
		}
	}
}

/** Loads a WindowPack (CHUI file) in the Window Manager */
bool Interface::LoadWindowPack(const char* name)
{
	DataStream* stream = GetResourceMgr()->GetResource( name, IE_CHU_CLASS_ID );
	if (stream == NULL) {
		printMessage( "Interface", "Error: Cannot find ", LIGHT_RED );
		printf( "%s.CHU\n", name );
		return false;
	}
	if (!GetWindowMgr()->Open( stream, true )) {
		printMessage( "Interface", "Error: Cannot Load ", LIGHT_RED );
		printf( "%s.CHU\n", name );
		return false;
	}

	strncpy( WindowPack, name, sizeof( WindowPack ) );
	WindowPack[sizeof( WindowPack ) - 1] = '\0';

	return true;
}

/** Loads a Window in the Window Manager */
int Interface::LoadWindow(unsigned short WindowID)
{
  unsigned int i;

	for (i = 0; i < windows.size(); i++) {
		if (windows[i] == NULL)
			continue;
		if (windows[i]->WindowID == WindowID && !stricmp( WindowPack,
													windows[i]->WindowPack )) {
			SetOnTop( i );
			windows[i]->Invalidate();
			return i;
		}
	}
	Window* win = windowmgr->GetWindow( WindowID );
	if (win == NULL) {
		return -1;
	}
	strcpy( win->WindowPack, WindowPack );

	int slot = -1;
	for (i = 0; i < windows.size(); i++) {
		if (windows[i] == NULL) {
			slot = i;
			break;
		}
	}
	if (slot == -1) {
		windows.push_back( win );
		slot = ( int ) windows.size() - 1;
	} else {
		windows[slot] = win;
	}
	win->Invalidate();
	return slot;
}
// FIXME: it's a clone of LoadWindow
/** Creates a Window in the Window Manager */
int Interface::CreateWindow(unsigned short WindowID, int XPos, int YPos, unsigned int Width, unsigned int Height, char* Background)
{
  unsigned int i;

	for (i = 0; i < windows.size(); i++) {
		if (windows[i] == NULL)
			continue;
		if (windows[i]->WindowID == WindowID && !stricmp( WindowPack,
													windows[i]->WindowPack )) {
			SetOnTop( i );
			windows[i]->Invalidate();
			return i;
		}
	}

	Window* win = new Window( WindowID, XPos, YPos, Width, Height );
	if (Background[0]) {
		if (IsAvailable( IE_MOS_CLASS_ID )) {
			DataStream* bkgr = GetResourceMgr()->GetResource( Background,
														IE_MOS_CLASS_ID );
			if (bkgr != NULL) {
				ImageMgr* mos = ( ImageMgr* )
					GetInterface( IE_MOS_CLASS_ID );
				mos->Open( bkgr, true );
				win->SetBackGround( mos->GetImage(), true );
				FreeInterface( mos );
			} else
				printf( "[Core]: Cannot Load BackGround, skipping\n" );
		} else
			printf( "[Core]: No MOS Importer Available, skipping background\n" );
	}

	strcpy( win->WindowPack, WindowPack );

	int slot = -1;
	for (i = 0; i < windows.size(); i++) {
		if (windows[i] == NULL) {
			slot = i;
			break;
		}
	}
	if (slot == -1) {
		windows.push_back( win );
		slot = ( int ) windows.size() - 1;
	} else {
		windows[slot] = win;
	}
	win->Invalidate();
	return slot;
}

/** Get a Control on a Window */
int Interface::GetControl(unsigned short WindowIndex, unsigned long ControlID)
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		return -1;
	}
	int i = 0;
	while (true) {
		Control* ctrl = win->GetControl( i );
		if (ctrl == NULL)
			return -1;
		if (ctrl->ControlID == ControlID)
			return i;
		i++;
	}
}
/** Set the Text of a Control */
int Interface::SetText(unsigned short WindowIndex,
	unsigned short ControlIndex, const char* string)
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		return -1;
	}
	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return -1;
	}
	return ctrl->SetText( string );
}
/** Set the Tooltip text of a Control */
int Interface::SetTooltip(unsigned short WindowIndex,
	unsigned short ControlIndex, const char* string)
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		return -1;
	}
	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return -1;
	}
	return ctrl->SetTooltip( string );
}

void Interface::DisplayTooltip(int x, int y, char* text)
{
	tooltip_x = x;
	tooltip_y = y;
	tooltip_text = text;
}

/** Set a Window Visible Flag */
int Interface::SetVisible(unsigned short WindowIndex, int visible)
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		return -1;
	}
	switch (visible) {
		case 0:
		case 2:
			 {
				win->Visible = false;
				Region r( win->XPos, win->YPos, win->Width, win->Height );
				Color black = {
					0, 0, 0, 128
				};
				evntmgr->DelWindow( win->WindowID );
				if (visible == 2)
					video->DrawRect( r, black );
			}
			break;

		case 1:
			 {
				win->Visible = true;
				evntmgr->AddWindow( win );
				win->Invalidate();
				SetOnTop( WindowIndex );
			}
			break;
		
		default:
			win->Visible = ( visible != 0 );
			break;
	}
	return 0;
}

/** Set an Event of a Control */
int Interface::SetEvent(unsigned short WindowIndex,
	unsigned short ControlIndex, unsigned long EventID, char* funcName)
{
	if (WindowIndex >= windows.size()) {
		printMessage( "Core", "Window not found", LIGHT_RED );
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		printMessage( "Core", "Window already freed", LIGHT_RED );
		return -1;
	}
	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		printMessage( "Core", "Control not found", LIGHT_RED );
		return -1;
	}
	if (ctrl->ControlType != ( EventID >> 24 )) {
		printf( "Expected control: %0x, but got: %0x", (int) (EventID >> 24),
			(int) ctrl->ControlType );
		printStatus( "ERROR", LIGHT_RED );
		return -1;
	}
	switch (ctrl->ControlType) {
		case IE_GUI_BUTTON:
			//Button
			 {
				Button* btn = ( Button* ) ctrl;
				btn->SetEvent( funcName, EventID & 255 );
				return 0;
			}
			break;

		case IE_GUI_TEXTAREA:
			//TextArea
			 {
				TextArea* te = ( TextArea* ) ctrl;
				strncpy( te->TextAreaOnChange, funcName, 64 );
				return 0;
			}
			break;

		case IE_GUI_EDIT:
			//Slider
			 {
				TextEdit* te = ( TextEdit* ) ctrl;
				strncpy( te->EditOnChange, funcName, 64 );
				return 0;
			}
			break;

		case IE_GUI_SLIDER:
			//Slider
			 {
				Slider* sld = ( Slider* ) ctrl;
				strncpy( sld->SliderOnChange, funcName, 64 );
				return 0;
			}
			break;

		case IE_GUI_LABEL:
			//Label
			 {
				Label* lab = ( Label* ) ctrl;
				strncpy( lab->LabelOnPress, funcName, 64 );
				return 0;
			}
			break;

		case IE_GUI_SCROLLBAR:
			//ScrollBar
			 {
				ScrollBar* sb = ( ScrollBar* ) ctrl;
				strncpy( sb->ScrollBarOnChange, funcName, 64 );
				return 0;
			}
			break;
	}
	printf( "Control has no event implemented: %0x", (int) ctrl->ControlID );
	printStatus( "ERROR", LIGHT_RED );
	return -1;
}

/** Set the Status of a Control in a Window */
int Interface::SetControlStatus(unsigned short WindowIndex,
	unsigned short ControlIndex, unsigned long Status)
{
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		return -1;
	}
	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return -1;
	}
	if(Status&0x80) {
			evntmgr->SetFocused( win, ctrl);
	}
	switch ((Status >> 24) & 0xff) {
		case 0:
		//Button
		 {
			if (ctrl->ControlType != 0)
				return -1;
			Button* btn = ( Button* ) ctrl;
			btn->SetState( ( unsigned char ) ( Status & 0x7f ) );
		}
		break;
	}
	return 0;
}

/** Show a Window in Modal Mode */
int Interface::ShowModal(unsigned short WindowIndex)
{
	if (WindowIndex >= windows.size()) {
		printMessage( "Core", "Window not found", LIGHT_RED );
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		printMessage( "Core", "Window already freed", LIGHT_RED );
		return -1;
	}
	win->Visible = true;
	evntmgr->Clear();
	SetOnTop( WindowIndex );
	evntmgr->AddWindow( win );
	win->Invalidate();
	return 0;
}

void Interface::DrawWindows(void)
{
	std::vector< int>::reverse_iterator t = topwin.rbegin();
	for (unsigned int i = 0; i < topwin.size(); i++) {
		if ( (unsigned int) ( *t ) >=windows.size() )
			continue;
		if (windows[( *t )] != NULL && windows[( *t )]->Visible)
			windows[( *t )]->DrawWindow();
		++t;
	}

}

void Interface::DrawTooltip ()
{	
	if (! tooltip_text) 
		return;

	Font* fnt = GetFont( TooltipFont );

	int Width = fnt->CalcStringWidth( tooltip_text );
	//int Width = 200;
	int Height = fnt->maxHeight;

	int x = tooltip_x - Width / 2;
	int y = tooltip_y - Height / 2;

	// Ensure placement within the screen
	if (x < 0) x = 0;
	else if (x + Width > core->Width) 
		x = core->Width - Width;
	if (y < 0) y = 0;
	else if (y + Height > core->Height) 
		y = core->Height - Height;

	Color fore = {0x00, 0xff, 0x00, 0x00};
	Color back = {0x00, 0x00, 0x00, 0x00};
	Color* palette = GetVideoDriver()->CreatePalette( fore, back );
	
	fnt->Print( Region( x, y, Width, Height ),
		    ( unsigned char * ) tooltip_text, palette,
		    IE_FONT_ALIGN_CENTER | IE_FONT_SINGLE_LINE, true );
}

Window* Interface::GetWindow(unsigned short WindowIndex)
{
	if (WindowIndex < windows.size()) {
		return windows[WindowIndex];
	}
	return NULL;
}

int Interface::DelWindow(unsigned short WindowIndex)
{
	if(WindowIndex == 0xffff) {
		for(unsigned int WindowIndex=0; WindowIndex<windows.size();WindowIndex++) {
			Window* win = windows[WindowIndex];
			if(win) {
				evntmgr->DelWindow( win->WindowID );
				delete( win );
			}
		}
		windows.clear();
		topwin.clear();
		return 0;
	}
	if (WindowIndex >= windows.size()) {
		return -1;
	}
	Window* win = windows[WindowIndex];
	if (win == NULL) {
		printMessage( "Core", "Window deleted again", LIGHT_RED );
		return -1;
	}
	evntmgr->DelWindow( win->WindowID );
	delete( win );
	windows[WindowIndex] = NULL;
	std::vector< int>::iterator t;
	for (t = topwin.begin(); t != topwin.end(); ++t) {
		if (( *t ) == WindowIndex) {
			topwin.erase( t );
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
	console->Draw( 0, 0 );
}

/** Get the Sound Manager */
SoundMgr* Interface::GetSoundMgr()
{
	return soundmgr;
}
/** Get the Sound Manager */
SaveGameIterator* Interface::GetSaveGameIterator()
{
	return sgiterator;
}
/** Sends a termination signal to the Video Driver */
bool Interface::Quit(void)
{
	return video->Quit();
}
/** Returns the variables dictionary */
Variables* Interface::GetDictionary()
{
	return vars;
}
/** Returns the token dictionary */
Variables* Interface::GetTokenDictionary()
{
	return tokens;
}
/** Get the Music Manager */
MusicMgr* Interface::GetMusicMgr()
{
	return music;
}
/** Loads a 2DA Table, returns -1 on error or the Table Index on success */
int Interface::LoadTable(const char* ResRef)
{
	int ind = GetIndex( ResRef );
	if (ind != -1) {
		return ind;
	}
	//printf("(%s) Table not found... Loading from file\n", ResRef);
	DataStream* str = key->GetResource( ResRef, IE_2DA_CLASS_ID );
	if (!str) {
		return -1;
	}
	TableMgr* tm = ( TableMgr* ) GetInterface( IE_2DA_CLASS_ID );
	if (!tm) {
		delete( str );
		return -1;
	}
	if (!tm->Open( str, true )) {
		FreeInterface( tm );
		return -1;
	}
	Table t;
	t.free = false;
	strncpy( t.ResRef, ResRef, 8 );
	t.tm = tm;
	ind = -1;
	for (size_t i = 0; i < tables.size(); i++) {
		if (tables[i].free) {
			ind = ( int ) i;
			break;
		}
	}
	if (ind != -1) {
		tables[ind] = t;
		return ind;
	}
	tables.push_back( t );
	return ( int ) tables.size() - 1;
}
/** Gets the index of a loaded table, returns -1 on error */
int Interface::GetIndex(const char* ResRef)
{
	for (size_t i = 0; i < tables.size(); i++) {
		if (tables[i].free)
			continue;
		if (strnicmp( tables[i].ResRef, ResRef, 8 ) == 0)
			return ( int ) i;
	}
	return -1;
}
/** Gets a Loaded Table by its index, returns NULL on error */
TableMgr* Interface::GetTable(unsigned int index)
{
	if (index >= tables.size()) {
		return NULL;
	}
	if (tables[index].free) {
		return NULL;
	}
	return tables[index].tm;
}
/** Frees a Loaded Table, returns false on error, true on success */
bool Interface::DelTable(unsigned int index)
{
	if (index >= tables.size()) {
		return false;
	}
	if (tables[index].free) {
		return false;
	}
	FreeInterface( tables[index].tm );
	tables[index].free = true;
	return true;
}
/** Loads an IDS Table, returns -1 on error or the Symbol Table Index on success */
int Interface::LoadSymbol(const char* ResRef)
{
	int ind = GetSymbolIndex( ResRef );
	if (ind != -1) {
		return ind;
	}
	DataStream* str = key->GetResource( ResRef, IE_IDS_CLASS_ID );
	if (!str) {
		return -1;
	}
	SymbolMgr* sm = ( SymbolMgr* ) GetInterface( IE_IDS_CLASS_ID );
	if (!sm) {
		delete( str );
		return -1;
	}
	if (!sm->Open( str, true )) {
		FreeInterface( sm );
		return -1;
	}
	Symbol s;
	s.free = false;
	strncpy( s.ResRef, ResRef, 8 );
	s.sm = sm;
	ind = -1;
	for (size_t i = 0; i < symbols.size(); i++) {
		if (symbols[i].free) {
			ind = ( int ) i;
			break;
		}
	}
	if (ind != -1) {
		symbols[ind] = s;
		return ind;
	}
	symbols.push_back( s );
	return ( int ) symbols.size() - 1;
}
/** Gets the index of a loaded Symbol Table, returns -1 on error */
int Interface::GetSymbolIndex(const char* ResRef)
{
	for (size_t i = 0; i < symbols.size(); i++) {
		if (symbols[i].free)
			continue;
		if (strnicmp( symbols[i].ResRef, ResRef, 8 ) == 0)
			return ( int ) i;
	}
	return -1;
}
/** Gets a Loaded Symbol Table by its index, returns NULL on error */
SymbolMgr* Interface::GetSymbol(unsigned int index)
{
	if (index >= symbols.size()) {
		return NULL;
	}
	if (symbols[index].free) {
		return NULL;
	}
	return symbols[index].sm;
}
/** Frees a Loaded Symbol Table, returns false on error, true on success */
bool Interface::DelSymbol(unsigned int index)
{
	if (index >= symbols.size()) {
		return false;
	}
	if (symbols[index].free) {
		return false;
	}
	FreeInterface( symbols[index].sm );
	symbols[index].free = true;
	return true;
}
/** Plays a Movie */
int Interface::PlayMovie(char* ResRef)
{
	MoviePlayer* mp = ( MoviePlayer* ) GetInterface( IE_MVE_CLASS_ID );
	if (!mp) {
		return 0;
	}
	DataStream* str = GetResourceMgr()->GetResource( ResRef,
												IE_MVE_CLASS_ID );
	if (!str) {
		FreeInterface( mp );
		return -1;
	}
	if (!mp->Open( str, true )) {
		FreeInterface( mp );
		delete( str );
		return -1;
	}
	mp->Play();
	FreeInterface( mp );
	return 0;
}

int Interface::Roll(int dice, int size, int add)
{
	if (dice < 1) {
		return 0;
	}
	if (size < 1) {
		return 0;
	}
	if (dice > 100) {
		return add + dice * size / 2;
	}
	for (int i = 0; i < dice; i++) {
		add += rand() % size + 1;
	}
	return add;
}

int Interface::GetCharSounds(TextArea* ta)
{
	bool hasfolders;
	int count = 0;
	char Path[_MAX_PATH];

	PathJoin( Path, GamePath, "sounds", NULL );
	hasfolders = ( HasFeature( GF_SOUNDFOLDERS ) != 0 );
	DIR* dir = opendir( Path );
	if (dir == NULL) {
		return -1;
	}
	//Lookup the first entry in the Directory
	struct dirent* de = readdir( dir );
	if (de == NULL) {
		closedir( dir );
		return -1;
	}
	printf( "Looking in %s\n", Path );
	do {
		if (de->d_name[0] == '.')
			continue;
		char dtmp[_MAX_PATH];
		PathJoin( dtmp, Path, de->d_name, NULL );
		struct stat fst;
		stat( dtmp, &fst );
		if (hasfolders == !S_ISDIR( fst.st_mode ))
			continue;
		count++;
		ta->AppendText( de->d_name, -1 );
	} while (( de = readdir( dir ) ) != NULL);
	closedir( dir );
	return count;
}

bool Interface::LoadINI(const char* filename)
{
	FILE* config;
	config = fopen( filename, "rb" );
	if (config == NULL) {
		return false;
	}
	char name[65], value[_MAX_PATH + 3];
	while (!feof( config )) {
		name[0] = 0;
		value[0] = 0;
		char rem;
		fread( &rem, 1, 1, config );
		if (( rem == '#' ) ||
			( rem == '[' ) ||
			( rem == '\r' ) ||
			( rem == '\n' ) ||
			( rem == ';' )) {
			if (rem == '\r') {
				fgetc( config );
				continue;
			} else if (rem == '\n')
				continue;
			fscanf( config, "%*[^\r\n]%*[\r\n]" );
			continue;
		}
		fseek( config, -1, SEEK_CUR );
		fscanf( config, "%[^=]=%[^\r\n]%*[\r\n]", name, value );
		if (( value[0] >= '0' ) && ( value[0] <= '9' )) {
			vars->SetAt( name, atoi( value ) );
		}
	}
	fclose( config );
	return true;
}

void Interface::SetGameVariable(const char* VarName, const char* Context,
	int value)
{
	//script->SetVariable(VarName, Context, value);
}

/** Enables/Disables the Cut Scene Mode */
void Interface::SetCutSceneMode(bool active)
{
	GameControl *gc=GetGameControl();
	if(gc) {
		gc->SetCutSceneMode( active );
	}
}

bool Interface::InCutSceneMode()
{
       return video->DisableMouse;
}

void Interface::LoadGame(int index)
{
	DataStream* ds;

	if (index == -1) {
		//Load the Default Game
		ds = GetResourceMgr()->GetResource( GameNameResRef, IE_GAM_CLASS_ID );
	} else {
		SaveGame* sg = GetSaveGameIterator()->GetSaveGame( index );
		if (!sg)
			return;
		ds = sg->GetGame();
		delete sg;
	}
	if (!ds) {
		return;
	}
	SaveGameMgr* sgm = ( SaveGameMgr* ) GetInterface( IE_GAM_CLASS_ID );
	if (!sgm) {
		delete ds;
		return;
	}
	sgm->Open( ds );

	if (game) {
		delete( game );
	}
	game = sgm->GetGame();
	FreeInterface( sgm );
}

GameControl *Interface::GetGameControl()
{
 	Window *window = GetWindow( 0 );
	Control* gc = window->GetControl(0);
	if(gc->ControlType!=IE_GUI_GAMECONTROL) {
		return NULL;
	}
	return (GameControl *) gc;
}

bool Interface::InitItemTypes()
{
	if (slotmatrix) {
		free(slotmatrix);
	}
	int ItemTypeTable = core->LoadTable( "itemtype" );
	TableMgr *it = core->GetTable(ItemTypeTable);

	ItemTypes = 0;
	if(it) {
		ItemTypes = it->GetRowCount(); //number of itemtypes
		if (ItemTypes<0) {
			ItemTypes = 0;
		}
		int InvSlotTypes = it->GetColumnCount();
		if (InvSlotTypes > 32) { //bit count limit
			InvSlotTypes = 32;
		}
		//make sure unsigned int is 32 bits
		slotmatrix = (unsigned int *) malloc(ItemTypes * sizeof(unsigned int) );
		for (int i=0;i<ItemTypes;i++) {
			unsigned int value = 0;
			unsigned int k = 1;
			for (int j=0;j<InvSlotTypes;j++) {
				if (strtol(it->QueryField(i,j),NULL,0) ) {
					value |= k;
				}
				k <<= 1;
			}
			slotmatrix[i] = value;
		}
		core->DelTable(ItemTypeTable);
	}

	//slottype describes the inventory structure
	int SlotTypeTable = core->LoadTable( "slottype" );
	TableMgr *st = core->GetTable(SlotTypeTable);
	if (slottypes) {
		free(slottypes);
	}
	SlotTypes = 0;
	if(st) {
		SlotTypes = st->GetColumnCount();
		//make sure unsigned int is 32 bits
		slottypes = (unsigned int *) malloc(SlotTypes * sizeof(unsigned int) );
		// FIXME: read in slottype info
		//
		for (int i=0;i<SlotTypes;i++) {
			slottypes[i] = strtol(it->QueryField(i),NULL,0 );
		}
		core->DelTable(SlotTypeTable);
	}
	return (it && st);
}

bool Interface::CanUseItemType(int itype, int slottype)
{
	if( !slottype ) { 
		//inventory slot, can hold any item, including invalid
		return true;
	}
	if( itype>=ItemTypes ) {
		//invalid itemtype
		return false;
	}
	//if any bit is true, we return true (int->bool conversion)
	return (bool) (slotmatrix[itype]&slottype);
}

