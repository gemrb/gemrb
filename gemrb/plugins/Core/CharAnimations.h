#ifndef CHARANIMATIONS_H
#define CHARANIMATIONS_H

#include "Animation.h"
#include <vector>

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT CharAnimations
{
public:
	std::vector<Animation*> a1HBattack;
	std::vector<Animation*> a2HBattack;
	std::vector<Animation*> a1HSattack;
	std::vector<Animation*> a2HSattack;
	std::vector<Animation*> a1HPattack;
	std::vector<Animation*> a2HPattack;
	std::vector<Animation*>	a2WBattack;
	std::vector<Animation*>	a2WSattack;
	std::vector<Animation*> a2WPattack;
	std::vector<Animation*>	Bowattack;
	std::vector<Animation*>	Throwattack;
	std::vector<Animation*> XBowattack;
	std::vector<Animation*>	SpellCasts;
	std::vector<Animation*> Stands;
	std::vector<Animation*> Walk;
public:
	CharAnimations(void);
	~CharAnimations(void);
};

#endif
