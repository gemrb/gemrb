#ifndef GLOBALTIMER_H
#define GLOBALTIMER_H

#include <vector>
#include "Region.h"

class ControlAnimation;

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

struct AnimationRef
{
	ControlAnimation *ctlanim;
	unsigned long  time;
};


class GEM_EXPORT GlobalTimer {
private:
	unsigned long startTime;
	unsigned long interval;

	//GameScript* CutScene;

	unsigned long fadeToCounter, fadeToMax;
	unsigned long fadeFromCounter, fadeFromMax;
	unsigned long waitCounter;
	unsigned long shakeCounter;
	unsigned long shakeX, shakeY;
	int first_animation;
	std::vector<AnimationRef*>  animations;
public:
	GlobalTimer(void);
	~GlobalTimer(void);
public:
	void Init();
	void Freeze();
	void Update();
	void SetFadeToColor(unsigned long Count);
	void SetFadeFromColor(unsigned long Count);
	void SetWait(unsigned long Count);
	//void SetCutScene(GameScript* script);
	void SetScreenShake(unsigned long shakeX, unsigned long shakeY,
		unsigned long Count);
	void AddAnimation(ControlAnimation* ctlanim, unsigned long time);
	void RemoveAnimation(ControlAnimation* ctlanim);
	void ClearAnimations();
	void UpdateAnimations();
public:
	//bool CutSceneMode;
	Region shakeStartVP;
};

#endif
