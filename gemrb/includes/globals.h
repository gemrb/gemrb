#ifndef GLOBALS_H
#define GLOBALS_H

typedef unsigned long ulong;
typedef unsigned char Byte;
typedef unsigned long DWORD;

#define GEMRB_RELEASE 10		//GemRB Release version multiplied by 1000 (i.e. 1200 = 1.2 | 10 = 0.01 )
#define GEMRB_API_NUM 2			//GemRB API Version
#define GEMRB_SDK_REV 0			//GemRB SDK Revision

#define VERSION_GEMRB ((GEMRB_RELEASE<<16)+(GEMRB_API_NUM<<8)+GEMRB_SDK_REV)

#ifndef WIN32
#define _MAX_PATH FILENAME_MAX
#endif

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

#endif
