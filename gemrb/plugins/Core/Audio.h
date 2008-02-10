#ifndef AUDIO_H_INCLUDED
#define AUDIO_H_INCLUDED

#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include "Plugin.h"
#include "AmbientMgr.h"

#define GEM_SND_RELATIVE 1
#define GEM_SND_SPEECH   IE_STR_SPEECH // 4
#define GEM_SND_VOL_MUSIC    1
#define GEM_SND_VOL_AMBIENTS 2

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Audio : public Plugin {
public:
    Audio(void) ;
    virtual ~Audio() ;
    virtual bool Init(void) = 0 ;
    virtual unsigned int Play(const char* ResRef, int XPos = 0, int YPos = 0, unsigned int flags = GEM_SND_RELATIVE) = 0 ;
    virtual bool IsSpeaking() = 0 ;
    virtual AmbientMgr* GetAmbientMgr() { return ambim ; } ;
    virtual void UpdateVolume(unsigned int flags = GEM_SND_VOL_MUSIC | GEM_SND_VOL_AMBIENTS) = 0;
    virtual bool CanPlay() = 0 ;
    virtual void ResetMusics() = 0 ;
    virtual bool Play() = 0 ;
    virtual bool Stop() = 0 ;
    virtual int StreamFile(const char* fileName ) = 0 ;
    virtual void UpdateListenerPos(int XPos, int YPos ) = 0;
    virtual void GetListenerPos(int &XPos, int &YPos ) = 0 ;
    virtual bool ReleaseAmbientStream(int stream, bool HardStop=false ) = 0 ;
    virtual int SetupAmbientStream( ieWord x, ieWord y, ieWord z,
                    ieWord gain, bool point ) = 0 ;
    virtual int QueueAmbient(int stream, const char* sound) = 0;
    virtual void SetAmbientStreamVolume(int stream, int volume) = 0 ;
protected:
    AmbientMgr* ambim ;

};

#endif // AUDIO_H_INCLUDED
