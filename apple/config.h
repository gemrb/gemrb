
#define NOCOLOR 1

#ifndef HAVE_SNPRINTF
	#define HAVE_SNPRINTF 1
#endif

#if TARGET_OS_IPHONE
	#define PACKAGE "GemRB"
	#define TOUCHSCREEN
	#define STATIC_LINK//this is in the target build settings now.
	#define SIZEOF_INT 4
	#define SIZEOF_LONG_INT 8

	#define DATADIR UserDir
#else
	#define PACKAGE "GemRB"
	#define SIZEOF_INT 4
	#define SIZEOF_LONG_INT 8

	#define MAC_GEMRB_APPSUPPORT "~/Library/Application Support/GemRB"
	#define PLUGINDIR "~/Library/Application Support/GemRB/plugins"
	#define DATADIR UserDir
#endif

#define HAVE_FORBIDDEN_OBJECT_TO_FUNCTION_CAST 1
