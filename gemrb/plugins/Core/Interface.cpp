#include "win32def.h"
#include "Interface.h"
#include "FileStream.h"
#include "AnimationMgr.h"

#ifdef WIN32
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT
#endif

GEM_EXPORT Interface *core;

Interface::Interface(void)
{
	video = NULL;
	key = NULL;
	strings = NULL;
	hcanims = NULL;
	if(!LoadConfig()) {
		printf("Cannot Load Config File.\nTermination in Progress...\n");
		exit(-1);
	}
	plugin = new PluginMgr(GemRBPath);
	factory = new Factory();
}

Interface::~Interface(void)
{
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
	std::vector<Font*>::iterator m = fonts.begin();
	for(int i = 0; fonts.size() != 0; ) {
		delete(*m);
		fonts.erase(m);
		m = fonts.begin();
	}
	delete(plugin);
}

int Interface::Init()
{
  printf("Searching for Video Driver...");
	if(!IsAvailable(IE_VIDEO_CLASS_ID)) {
		printf("[ERROR]\nNo Video Driver Available.\nTermination in Progress...\n");
		return GEM_ERROR;
	}
	printf("[OK]\nGetting Video Plugin Interface...");
	video = (Video*)GetInterface(IE_VIDEO_CLASS_ID);
	video->Init();
	printf("Searching for KEY Importer...");
	if(!IsAvailable(IE_KEY_CLASS_ID)) {
		printf("[ERROR]\nNo KEY Importer Available.\nTermination in Progress...\n");
		return GEM_ERROR;
	}
	printf("[OK]\n");
	key = (ResourceMgr*)GetInterface(IE_KEY_CLASS_ID);
	char ChitinPath[_MAX_PATH];
	strcpy(ChitinPath, GamePath);
	strcat(ChitinPath, "chitin.key");
	if(!key->LoadResFile(ChitinPath)) {
		printf("Error Loading Chitin.key\nTermination in Progress...\n");
		return GEM_ERROR;
	}
	if(!IsAvailable(IE_HCANIMS_CLASS_ID)) {
		printf("No Hard Coded Animations Available.\nTermination in Progress...\n");
		return GEM_ERROR;
	}
	hcanims = (HCAnimationSeq*)GetInterface(IE_HCANIMS_CLASS_ID);
	if(!IsAvailable(IE_TLK_CLASS_ID)) {
		printf("No TLK Importer Available.\nTermination in Progress...\n");
		return GEM_ERROR;
	}
	strings = (StringMgr*)GetInterface(IE_TLK_CLASS_ID);
	char strpath[_MAX_PATH];
	strcpy(strpath, GamePath);
	strcat(strpath, "dialog.tlk");
	FileStream * fs = new FileStream();
	if(!fs->Open(strpath)) {
		printf("Cannot find Dialog.tlk.\nTermination in Progress...\n");
		delete(fs);
		return GEM_ERROR;
	}
	strings->Open(fs, true);
	printf("Loading Palettes...\n");
	DataStream * bmppal256 = key->GetResource("MPAL256\0", IE_BMP_CLASS_ID);
	DataStream * bmppal16 = key->GetResource("MPALETTE", IE_BMP_CLASS_ID);
	pal256 = (ImageMgr*)this->GetInterface(IE_BMP_CLASS_ID);
	pal16  = (ImageMgr*)this->GetInterface(IE_BMP_CLASS_ID);
	pal256->Open(bmppal256, true);
	pal16->Open(bmppal16, true);
	printf("Palettes Loaded\n");
	printf("Loading Fonts...\n");
	AnimationMgr * anim = (AnimationMgr*)GetInterface(IE_BAM_CLASS_ID);
	for(int i = 0; i < 5; i++) {
		char ResRef[9];
		switch(i) {
			case 0:
				strcpy(ResRef, "REALMS\0\0\0");
			break;

			case 1:
				strcpy(ResRef, "STONEBIG\0");
			break;

			case 2:
				strcpy(ResRef, "STONESML\0");
			break;

			case 3:
				strcpy(ResRef, "NORMAL\0\0\0");
			break;

			case 4:
				strcpy(ResRef, "TOOLFONT");
			break;
		}
		DataStream * fstr = key->GetResource(ResRef, IE_BAM_CLASS_ID);
		anim->Open(fstr, true);
		Font * fnt = anim->GetFont();
		strncpy(fnt->ResRef, ResRef, 8);
		fonts.push_back(fnt);
	}
	FreeInterface(anim);
	printf("Fonts Loaded\n");
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

char * Interface::TypeExt(SClass_ID type)
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
	return strings->GetString(strref);
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

#ifndef WIN32
#define stricmp(x,y) strcasecmp(x,y)
#endif

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
		else if(stricmp(name, "GemRBPath") == 0) {
			strcpy(GemRBPath, value);
		}
		else if(stricmp(name, "CachePath") == 0) {
			strcpy(CachePath, value);
		}
		else if(stricmp(name, "GamePath") == 0) {
			strcpy(GamePath, value);
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
	}
	return true;
}
/** No descriptions */
Color * Interface::GetPalette(int index, int colors){
	Color * pal = NULL;
	if(colors == 16) {
		pal = (Color*)malloc(colors*sizeof(Color));
		pal16->GetPalette(index, colors, pal);
	}
	else if(colors == 256) {
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
