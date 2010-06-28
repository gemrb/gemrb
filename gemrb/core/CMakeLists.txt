ADD_DEFINITIONS(-DGEM_BUILD_DLL)

FILE(GLOB gemrb_core_LIB_SRCS "*.cpp")

ADD_LIBRARY(gemrb_core SHARED ${gemrb_core_LIB_SRCS}
	GameScript/Actions.cpp
	GameScript/GSUtils.cpp
	GameScript/GameScript.cpp
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
	System/CachedFileStream.cpp
	System/DataStream.cpp
	System/FileStream.cpp
	System/MemoryStream.cpp
	System/VFS.cpp
	System/snprintf.cpp
	)

IF(WIN32)
  INSTALL(TARGETS gemrb_core RUNTIME DESTINATION ${LIB_DIR})
ELSE(WIN32)
  INSTALL(TARGETS gemrb_core LIBRARY DESTINATION ${LIB_DIR})
ENDIF(WIN32)
