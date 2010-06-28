ADD_DEFINITIONS(-DGEM_BUILD_DLL)

FILE(GLOB gemrb_core_LIB_SRCS "*.cpp")

ADD_LIBRARY(gemrb_core SHARED ${gemrb_core_LIB_SRCS}
	GUI/Button.cpp
	GUI/Console.cpp
	GUI/Control.cpp
	GUI/GameControl.cpp
	GUI/Label.cpp
	GUI/MapControl.cpp
	GUI/Progressbar.cpp
	GUI/ScrollBar.cpp
	GUI/Slider.cpp
	GUI/TextArea.cpp
	GUI/TextEdit.cpp
	GUI/WorldMapControl.cpp
	Scriptable/Actor.cpp
	Scriptable/ActorBlock.cpp
	)

IF(WIN32)
  INSTALL(TARGETS gemrb_core RUNTIME DESTINATION ${LIB_DIR})
ELSE(WIN32)
  INSTALL(TARGETS gemrb_core LIBRARY DESTINATION ${LIB_DIR})
ENDIF(WIN32)
