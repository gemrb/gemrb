#define INTERFACE
#include "Interface.h"
#include "FileStream.h"
#include "AnimationMgr.h"
#include "Slider.h"

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
	ConsolePopped = false;
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
}

Interface::~Interface(void)
{
	std::vector<Font*>::iterator m = fonts.begin();
	while( fonts.size() ) {
		delete(*m);
		fonts.erase(m);
		m = fonts.begin();
	}
	std::vector<Window*>::iterator w = windows.begin();
	while( windows.size() ) {
		if((*w)) {
			delete(*w);
		}
		windows.erase(w);
		w = windows.begin();
	}
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
	delete(console);
	delete(plugin);
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
	DataStream * bmppal256 = key->GetResource("MPAL256\0", IE_BMP_CLASS_ID);
	DataStream * bmppal16 = key->GetResource("MPALETTE", IE_BMP_CLASS_ID);
	pal256 = (ImageMgr*)this->GetInterface(IE_BMP_CLASS_ID);
	pal16  = (ImageMgr*)this->GetInterface(IE_BMP_CLASS_ID);
	pal256->Open(bmppal256, true);
	pal16->Open(bmppal16, true);
	printMessage("Core", "Palettes Loaded\n", LIGHT_GREEN);
	printMessage("Core", "Loading Fonts...\n", WHITE);
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
	console->YPos = 480-25;
	console->Width = 640;
	console->Height = 25;
	console->SetFont(fonts[2]);
	console->SetCursor(fonts[2]->chars[2]);
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
	printMessage("Core", "Initializing Variables Dictionary...", WHITE);
	vars = new Variables();
	if(!vars) {
		printStatus("ERROR", LIGHT_RED);
		return GEM_ERROR;
	}
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
		else if(stricmp(name, "GUIScriptsPath") == 0) {
			strcpy(GUIScriptsPath, value);
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

void Interface::RedrawAll()
{
	for(int i = 0; i < windows.size(); i++) {
		if(windows[i]!=NULL)
			windows[i]->Invalidate();
	}
}

/** Loads a Window in the Window Manager */
int Interface::LoadWindow(unsigned short WindowID)
{
	for(int i = 0; i < windows.size(); i++) {
		if(windows[i]==NULL)
			continue;
		if(windows[i]->WindowID == WindowID) {
			windows[i]->Invalidate();
			return i;
		}
	}
	Window * win = windowmgr->GetWindow(WindowID);
	if(win == NULL)
		return -1;
	int slot = -1;
	for(int i = 0; i < windows.size(); i++) {
		if(windows[i]==NULL) {
			slot = i;
			break;
		}
	}
	if(slot == -1) {
		windows.push_back(win);
		slot=windows.size()-1;
	}
	else {
		windows[slot] = win;
	}
	win->Invalidate();
	return slot;
}

/** Get a Control on a Window */
int Interface::GetControl(unsigned short WindowIndex, unsigned short ControlID)
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
int Interface::SetVisible(unsigned short WindowIndex, bool visible)
{
	if(WindowIndex >= windows.size())
		return -1;
	Window * win = windows[WindowIndex];
	if(win==NULL)
		return -1;
	win->Visible = visible;
	if(win->Visible) {
		evntmgr->AddWindow(win);
		win->Invalidate();
	}
	else
		evntmgr->DelWindow(win->WindowID);
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
		case 0: //Button
			{
			Button * btn = (Button*)ctrl;
			btn->SetEvent(funcName);
			return 0;
			}
		break;

		case 2: //Slider
			{
			Slider * sld = (Slider*)ctrl;
			strcpy(sld->SliderOnChange, funcName);
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
	win->Invalidate();
	evntmgr->Clear();
	evntmgr->AddWindow(win);
	return 0;
}

void Interface::DrawWindows(void)
{
	for(int i = 0; i < windows.size(); i++) {
		if(windows[i]!=NULL && windows[i]->Visible)
			windows[i]->DrawWindow();
	}
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
/** Sends a termination signal to the Video Driver */
bool Interface::Quit(void)
{
	return video->Quit();
}

Variables * Interface::GetDictionary()
{
	return vars;
}
