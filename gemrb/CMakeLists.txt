ADD_SUBDIRECTORY( core )

IF(APPLE)
	SET_SOURCE_FILES_PROPERTIES(
		CocoaWrapper.h
		../apple/CocoaWrapper/mac/CocoaWrapper.m
		PROPERTIES LANGUAGE C
	)

	SET_SOURCE_FILES_PROPERTIES(
		../artwork/gemrb.icns
		PROPERTIES MACOSX_PACKAGE_LOCATION Resources
	)
	SET(MACOSX_BUNDLE_ICON_FILE gemrb)
	ADD_EXECUTABLE(gemrb MACOSX_BUNDLE ../apple/CocoaWrapper/mac/CocoaWrapper.m GemRB.cpp ../artwork/gemrb.icns)
	SET (BUNDLE_LOADER "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.app/Contents/MacOS/${PROJECT_NAME}") # BUNDLE_LOADER is used by plugins needing to extend the cocoa wrapper (SDLVideo).
	SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -bundle_loader ${BUNDLE_LOADER}")
ELSE(APPLE)
	ADD_EXECUTABLE(gemrb GemRB.cpp )
ENDIF(APPLE)
# we need to add gemrb target before processing plugins or COCOA plugins don't work right.

ADD_SUBDIRECTORY( plugins )
ADD_SUBDIRECTORY( docs )
ADD_SUBDIRECTORY( tests )
IF (APPLE)
	# GUIScripts ALWAYS get copied to the App bundle
	SET( DATA_DIR ${BIN_DIR}/${PROJECT_NAME}.app/Contents/Resources/ )
ENDIF(APPLE)
ADD_SUBDIRECTORY( override )
ADD_SUBDIRECTORY( GUIScripts )

SET_TARGET_PROPERTIES(gemrb PROPERTIES INSTALL_RPATH ${LIB_DIR})
INCLUDE_DIRECTORIES( ${SDL_INCLUDE_DIR} )
IF(WIN32)
 TARGET_LINK_LIBRARIES(gemrb gemrb_core)
ELSE(WIN32)
 IF(APPLE)
	if (STATIC_LINK)
		TARGET_LINK_LIBRARIES(gemrb
			  ${COCOA_LIBRARY_PATH} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT}
			 gemrb_core ${plugins}  -Wl,-all_load)
	else (STATIC_LINK)
		TARGET_LINK_LIBRARIES(gemrb gemrb_core
			  ${COCOA_LIBRARY_PATH} ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})
	endif (STATIC_LINK)
 ELSE(APPLE)
  if (STATIC_LINK)
	  TARGET_LINK_LIBRARIES(gemrb ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT}
		  -Wl,--whole-archive gemrb_core ${plugins} -Wl,--no-whole-archive)
  else (STATIC_LINK)
	  TARGET_LINK_LIBRARIES(gemrb gemrb_core ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})
  endif (STATIC_LINK)
 ENDIF(APPLE)
ENDIF(WIN32)

# preconfigure the sample config with the selected paths
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/GemRB.cfg.sample.in"
  "${CMAKE_CURRENT_BINARY_DIR}/GemRB.cfg.sample"
  IMMEDIATE @ONLY
)

INSTALL( TARGETS gemrb DESTINATION ${BIN_DIR} )
INSTALL( FILES "${CMAKE_CURRENT_BINARY_DIR}/GemRB.cfg.sample" GemRB.cfg.noinstall.sample DESTINATION ${SYSCONF_DIR} )
