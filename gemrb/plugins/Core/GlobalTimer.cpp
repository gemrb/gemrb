#include "GlobalTimer.h"
#include "Interface.h"

extern Interface* core;

GlobalTimer::GlobalTimer(void)
{
	interval = ( 1000 / AI_UPDATE_TIME );
	CutSceneMode = false;
	Init();
}

GlobalTimer::~GlobalTimer(void)
{
}

void GlobalTimer::Init()
{
	CutScene = NULL;
	fadeToCounter = 0;
	fadeFromCounter = 1;
	fadeFromMax = 0;
	waitCounter = 0;
	shakeCounter = 0;
	startTime = 0;  //forcing an update
}

void GlobalTimer::Update()
{
	unsigned long thisTime;
	GetTime( thisTime );
	if (( thisTime - startTime ) >= interval) {
		startTime = thisTime;
		if (shakeCounter) {
			int x = shakeStartVP.x;
			int y = shakeStartVP.y;
			if (shakeCounter != 1) {
				x += rand()%shakeX - shakeX>>1;
				y += rand()%shakeY - shakeY>>1;
			}
			core->GetVideoDriver()->MoveViewportTo(x,y,false);
			shakeCounter--;
		}
		if (fadeToCounter) {
			core->GetVideoDriver()->SetFadePercent( ( ( fadeToMax - fadeToCounter ) * 100 ) / fadeToMax );
			fadeToCounter--;
			return;
		}
		if (fadeFromCounter != ( fadeFromMax + 1 )) {
			core->GetVideoDriver()->SetFadePercent( ( ( fadeFromMax - fadeFromCounter ) * 100 ) / fadeFromMax );
			fadeFromCounter++;
			return;
		}
		GameControl* gc = core->GetGameControl();
		if (gc) {
			if (gc->DialogueFlags&DF_IN_DIALOG)
				return;
		}
		Game* game = core->GetGame();
		if (core->FogOfWar && game) {
			Map* map = game->GetCurrentMap();
			if (map) map->UpdateFog();
		}
		if (CutScene) {
			if (CutScene->endReached) {
				delete( CutScene );
				CutScene = NULL;
				CutSceneMode = false;
			}
			return;
		}
		//Call Scripts Update
	}
}

void GlobalTimer::SetFadeToColor(unsigned long Count)
{
	fadeToCounter = Count;
	fadeToMax = fadeToCounter;
	if (fadeToMax == 0) {
		core->GetVideoDriver()->SetFadePercent( 100 );
	}
}

void GlobalTimer::SetFadeFromColor(unsigned long Count)
{
	fadeFromCounter = 1;
	fadeFromMax = Count;
	if (fadeFromMax == 0) {
		core->GetVideoDriver()->SetFadePercent( 0 );
	}
}

void GlobalTimer::SetWait(unsigned long Count)
{
	waitCounter = Count;
}

void GlobalTimer::SetCutScene(GameScript* script)
{
	CutScene = script;
	if (CutScene) {
		CutScene->EvaluateAllBlocks(); //Caches the Script
		CutSceneMode = true;
		return;
	}
	CutSceneMode = false;
}

void GlobalTimer::SetScreenShake(unsigned long shakeX, unsigned long shakeY,
	unsigned long Count)
{
	this->shakeX = shakeX;
	this->shakeY = shakeY;
	shakeCounter = Count;
}
