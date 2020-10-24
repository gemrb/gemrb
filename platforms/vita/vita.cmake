ADD_DEFINITIONS("-DVITA")
	
SET(CMAKE_DL_LIBS "")
SET(VITA_FLAGS "-Ofast -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard -ftree-vectorize -D_GLIBCXX_USE_C99_MATH=0 -DNDEBUG")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${VITA_FLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VITA_FLAGS}")
SET(VITA_LIBRARIES_SDL_COMMON "-lvorbis -logg -lmikmod -lSceAudio_stub -lSceCtrl_stub -lvita2d -lSceGxm_stub -lSceHid_stub -lSceDisplay_stub -lSceDisplayUser_stub -lSceSysmodule_stub -lSceAppMgr_stub -lSceCommonDialog_stub -lScePower_stub -lSceAppUtil_stub")
SET(VITA_LIBRARIES_SDL "${VITA_LIBRARIES_SDL_COMMON}")
SET(VITA_LIBRARIES_SDL2 "-lFLAC -lSceTouch_stub ${VITA_LIBRARIES_SDL_COMMON}")

SET(DEFAULT_CACHE_DIR "ux0:data/GemRB/Cache2/")
SET(DATA_DIR "ux0:data/GemRB/")

IF (SDL_BACKEND STREQUAL "SDL")
	SET(CMAKE_CXX_STANDARD_LIBRARIES "${VITA_LIBRARIES_SDL}")
ELSEIF (SDL_BACKEND STREQUAL "SDL2")
	SET(CMAKE_CXX_STANDARD_LIBRARIES "${VITA_LIBRARIES_SDL2}")
ENDIF ()
