#include "GlobalTimer.h"
#include "Interface.h"

extern Interface * core;

GlobalTimer::GlobalTimer(void)
{
	startTime = 0;
	interval = (1000/AI_UPDATE_TIME);
	CutScene = NULL;
	MovingActor = NULL;
	fadeToCounter = 0;
	fadeFromCounter = 1;
	fadeFromMax = 0;
	waitCounter = 0;
	shakeCounter = 0;
	shakeX = shakeY = 0;
}

GlobalTimer::~GlobalTimer(void)
{
}

void GlobalTimer::Update()
{
	unsigned long thisTime;
	GetTime(thisTime);
	if((thisTime-startTime) >= interval) {
		startTime = thisTime;
		if(fadeToCounter) {
			core->GetVideoDriver()->SetFadePercent(((fadeToMax-fadeToCounter)*100)/fadeToMax);
			fadeToCounter--;
			return;
		} else {
			if(fadeFromCounter != (fadeFromMax+1)) {
				core->GetVideoDriver()->SetFadePercent(((fadeFromMax-fadeFromCounter)*100)/fadeFromMax);
				fadeFromCounter++;
				return;
			} else {
				if(shakeCounter) {
					if(shakeCounter != 1)
						core->GetVideoDriver()->SetViewport((-((short)shakeX>>1)) + shakeStartVP.x + (rand()%shakeX), (-((short)shakeY>>1)) + shakeStartVP.y + (rand()%shakeY));
					else
						core->GetVideoDriver()->SetViewport(shakeStartVP.x, shakeStartVP.y);
					shakeCounter--;
				}
			}
		}
		if(MovingActor && MovingActor->path)
			return;
		if(waitCounter) {
			waitCounter--;
			return;
		}
		GameControl * gc = (GameControl*)core->GetWindow(0)->GetControl(0);
		if(gc->ControlType == IE_GUI_GAMECONTROL) {
			if(gc->Dialogue)
				return;
		}
		MovingActor = NULL;
		if(CutScene) {
			if(!CutScene->endReached) {
				printf("Running CutScene\n");
				CutScene->Update();
			}
			else {
				printf("CutScene End\n");
				delete(CutScene);
				CutScene = NULL;
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
	if(fadeToMax == 1) {
		core->GetVideoDriver()->SetFadePercent(100);
		fadeToCounter--;
	}
}

void GlobalTimer::SetFadeFromColor(unsigned long Count)
{
	fadeFromCounter = 1;
	fadeFromMax = Count;
}

void GlobalTimer::SetWait(unsigned long Count)
{
	waitCounter = Count;
}

void GlobalTimer::SetMovingActor(Actor * actor)
{
	MovingActor = actor;
}

void GlobalTimer::SetCutScene(GameScript * script)
{
	CutScene = script;
	if(CutScene) {
		CutScene->Update(); //Caches the Script
		CutScene->Update(); //Executes the Script
		return;
	}
}

void GlobalTimer::SetScreenShake(unsigned long shakeX, unsigned long shakeY, unsigned long Count)
{
	this->shakeX = shakeX;
	this->shakeY = shakeY;
	shakeCounter = Count;
	shakeStartVP = core->GetVideoDriver()->GetViewport();
}
