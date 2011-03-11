ADD_DEFINITIONS(-DGEM_BUILD_DLL)

FILE(GLOB gemrb_core_LIB_SRCS "*.cpp"
	GameScript/Actions.cpp
	GameScript/GSUtils.cpp
	GameScript/GameScript.cpp
	GameScript/Matching.cpp
	GameScript/Objects.cpp
	GameScript/Triggers.cpp
	GUI/Button.cpp
	GUI/Console.cpp
	GUI/Control.cpp
	GUI/EventMgr.cpp
	GUI/GameControl.cpp
	GUI/Label.cpp
	GUI/MapControl.cpp
	GUI/Progressbar.cpp
	GUI/ScrollBar.cpp
	GUI/Slider.cpp
	GUI/TextArea.cpp
	GUI/TextEdit.cpp
	GUI/Window.cpp
	GUI/WorldMapControl.cpp
	Scriptable/Actor.cpp
	Scriptable/ActorBlock.cpp
	Scriptable/PCStatStruct.cpp
	System/DataStream.cpp
	System/FileStream.cpp
	System/SlicedStream.cpp
	System/MemoryStream.cpp
	System/VFS.cpp
	System/snprintf.cpp
	)

if (STATIC_LINK)
	ADD_LIBRARY(gemrb_core STATIC ${gemrb_core_LIB_SRCS})
else (STATIC_LINK)
	ADD_LIBRARY(gemrb_core SHARED ${gemrb_core_LIB_SRCS})
	IF(WIN32)
	  INSTALL(TARGETS gemrb_core RUNTIME DESTINATION ${LIB_DIR})
	ELSE(WIN32)
	  INSTALL(TARGETS gemrb_core LIBRARY DESTINATION ${LIB_DIR})
	ENDIF(WIN32)
endif (STATIC_LINK)

SET_TARGET_PROPERTIES(gemrb_core PROPERTIES
	COMPILE_DEFINITIONS
	"PLUGINDIR=\"${PLUGIN_DIR}\";DATADIR=\"${DATA_DIR}\";SYSCONFDIR=\"${SYSCONF_DIR}\""
	)
