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
	fadeFromCounter = 0;
	fadeFromMax = 1;
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
		} else if(fadeFromCounter != (fadeFromMax+1)) {
			core->GetVideoDriver()->SetFadePercent(((fadeFromMax-fadeFromCounter)*100)/fadeFromMax);
			fadeFromCounter++;
		} else if(shakeCounter) {
			core->GetVideoDriver()->SetViewport((-(shakeX>>1)) + shakeStartVP.x + (rand()%shakeX), (-(shakeY>>1)) + shakeStartVP.y + (rand()%shakeY));
			shakeCounter--;
		}
		if(MovingActor && MovingActor->path)
			return;
		if(waitCounter) {
			waitCounter--;
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
	/*if(fadeToMax == 1) {
		core->GetVideoDriver()->SetFadePercent(100);
		fadeToCounter--;
	}*/
}

void GlobalTimer::SetFadeFromColor(unsigned long Count)
{
	fadeFromCounter = 0;
	fadeFromMax = Count;
}

void GlobalTimer::SetWait(unsigned long Count)
{
	waitCounter = Count;
}

void GlobalTimer::SetMovingActor(ActorBlock * actor)
{
	MovingActor = actor;
}

void GlobalTimer::SetCutScene(GameScript * script)
{
	CutScene = script;
}

void GlobalTimer::SetScreenShake(unsigned long shakeX, unsigned long shakeY, unsigned long Count)
{
	this->shakeX = shakeX;
	this->shakeY = shakeY;
	shakeCounter = Count;
	shakeStartVP = core->GetVideoDriver()->GetViewport();
}
