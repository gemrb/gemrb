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

//#define GemRBDir "H:\\Projects\\GemRB\\Source\\GemRB\\"
//#define CacheDir "H:\\Projects\\GemRB\\Source\\GemRB\\Cache\\"
//#define DataPath "H:\\Projects\\GemRB\\Source\\GemRB\\Data"
//#define DataPath "H:\\Programmi\\Black Isle\\BGII - SoA\\"
//#define ChitinPath "H:\\Programmi\\Black Isle\\BGII - SoA\\chitin.key"
//#define GemRBDir ".\\"
//#define CacheDir ".\\cache\\"
//#define DataPath ".\\"
//#define ChitinPath ".\\chitin.key"

#endif
