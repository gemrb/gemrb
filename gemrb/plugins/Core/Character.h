#ifndef CHARACTER_H
#define CHARACTER_H

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

/** USING DEFINITIONS AS DESCRIBED IN STATS.IDS */
#define MAX_STATS 164

class GEM_EXPORT Character
{
private:
	short Stats[MAX_STATS];
	short Mods[MAX_STATS];
	char  Name[33];
	char  Dialog[9];
	char  Icon[9];
public:
	Character(void);
	~Character(void);
	/** Returns a Stat value (Base Value + Mod) */
	short GetStat(unsigned char StatIndex);
	/** Returns a Stat Modifier */
	short GetMod(unsigned char StatIndex);
	/** Returns a Stat Base Value */
	short GetBase(unsigned char StatIndex);
	/** Sets a Stat Base Value */
	bool  SetBase(unsigned char StatIndex, short Value);
	/** Sets a Stat Modifier Value */
	bool  SetMod(unsigned char StatIndex, short Mod);
	/** Sets the Character Name */
	void  SetName(const char * string)
	{
		if(string == NULL)
			return;
		strncpy(Name, string, 32);
	}
	/** Sets the Dialog ResRef */
	void  SetDialog(const char * ResRef)
	{
		if(ResRef == NULL)
			return;
		strncpy(Dialog, ResRef, 8);
	}
	/** Sets the Icon ResRef */
	void  SetIcon(const char * ResRef)
	{
		if(ResRef == NULL)
			return;
		strncpy(Icon, ResRef, 8);
	}
	/** Gets the Character Name */
	char *GetName(void)
	{
		return Name;
	}
	/** Gets the Dialog ResRef */
	char *GetDialog(void)
	{
		return Dialog;
	}
	/** Gets the Icon ResRef */
	char *GetIcon(void)
	{
		return Icon;
	}
};

#endif
