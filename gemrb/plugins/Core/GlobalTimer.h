#ifndef GLOBALTIMER_H
#define GLOBALTIMER_H

#include "GameScript.h"
#include "Actor.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT GlobalTimer
{
private:
	unsigned long startTime;
	unsigned long interval;

	GameScript * CutScene;
	Actor * MovingActor;

	unsigned long fadeToCounter, fadeToMax;
	unsigned long fadeFromCounter, fadeFromMax;
	unsigned long waitCounter;
	unsigned long shakeCounter;
	unsigned long shakeX, shakeY;
	Region		  shakeStartVP;
public:
	GlobalTimer(void);
	~GlobalTimer(void);
public:
	void Update();
	void SetFadeToColor(unsigned long Count);
	void SetFadeFromColor(unsigned long Count);
	void SetWait(unsigned long Count);
	void SetMovingActor(Actor * actor);
	void SetCutScene(GameScript * script);
	void SetScreenShake(unsigned long shakeX, unsigned long shakeY, unsigned long Count);
public:
	bool CutSceneMode;
};

#endif
