#ifndef GLOBALS_H
#define GLOBALS_H

typedef unsigned long ulong;
typedef unsigned char Byte;
typedef unsigned long DWORD;

#define GEMRB_RELEASE 200		//GemRB Release version multiplied by 1000 (i.e. 1200 = 1.2 | 10 = 0.01 )
#define GEMRB_API_NUM 2			//GemRB API Version
#define GEMRB_SDK_REV 8			//GemRB SDK Revision

#define VERSION_GEMRB ((GEMRB_RELEASE<<16)+(GEMRB_API_NUM<<8)+GEMRB_SDK_REV)

#ifndef WIN32
#define _MAX_PATH FILENAME_MAX
#endif

#ifndef GLOBALS_ONLY_DEFS

#include "stdlib.h"
#include "stdio.h"
#include "errors.h"
#include "SClassID.h"
#include "../plugins/Core/Class_ID.h"
#include "../plugins/Core/ClassDesc.h"
#include "RGBAColor.h"
#include "../plugins/Core/Region.h"
#include "../plugins/Core/Sprite2D.h"
#include "../plugins/Core/VideoMode.h"
#include "../plugins/Core/VideoModes.h"
#include "../plugins/Core/DataStream.h"
#include "../plugins/Core/AnimStructures.h"

#endif //GLOBALS_ONLY_DEFS

//Global Variables

#ifdef WIN32
#define PathDelimiter '\\'
#define SPathDelimiter "\\"
#else
#define PathDelimiter '/'
#define SPathDelimiter "/"
#endif

#define ExtractFileFromPath(file, full_path) strcpy (file, ((strrchr (full_path, PathDelimiter)==NULL) ? ((strchr (full_path, ':')==NULL) ? full_path : (strchr(full_path, ':') +1) ) : (strrchr(full_path, PathDelimiter) +1)))

#define IE_NORMAL 0
#define IE_SHADED 1

//IDS Importer Defines
#define IDS_VALUE_NOT_LOCATED -65535 // GetValue returns this if text is not found in arrays ... this needs to be a unique number that does not exist in the value[] array
#define GEM_ENCRYPTION_KEY "\x88\xa8\x8f\xba\x8a\xd3\xb9\xf5\xed\xb1\xcf\xea\xaa\xe4\xb5\xfb\xeb\x82\xf9\x90\xca\xc9\xb5\xe7\xdc\x8e\xb7\xac\xee\xf7\xe0\xca\x8e\xea\xca\x80\xce\xc5\xad\xb7\xc4\xd0\x84\x93\xd5\xf0\xeb\xc8\xb4\x9d\xcc\xaf\xa5\x95\xba\x99\x87\xd2\x9d\xe3\x91\xba\x90\xca"

/////feature flags
//#define  GF_SCROLLBAR_PATCH		0  //bg1, pst //NOT NEEDED ANYMORE (THIS SLOT IS FREE)
#define  GF_ALL_STRINGS_TAGGED	1  //bg1, pst, iwd1
#define  GF_HAS_SONGLIST		2  //bg2
#define  GF_MID_RES_AVATARS		3  //iwd1
#define  GF_UPPER_BUTTON_TEXT		4 //bg2
#define  GF_LOWER_LABEL_TEXT		5 //bg2
#define  GF_HAS_PARTY_INI               6 //iwd2

/////AI global defines
#define AI_UPDATE_TIME	30

/////globally used functions
#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

GEM_EXPORT bool dir_exists(const char *path);
GEM_EXPORT int strlench(const char *string, char ch);

#ifdef WIN32

#else
char * FindInDir(char * Dir, char * Filename);
void ResolveFilePath(char *FilePath);
char *strupr(char *string);
char *strlwr(char *string);
#endif

#ifdef WIN32
#define GetTime(store) store = GetTickCount()
#else
#include <sys/time.h>
#define GetTime(store) \
	{ \
		struct timeval tv; \
		gettimeofday(&tv, NULL); \
		store = (tv.tv_usec/1000) + (tv.tv_sec*1000); \
	}
#endif

struct ActorBlock;

#endif //GLOBALS_H

