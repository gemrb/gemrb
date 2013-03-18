ADD_SUBDIRECTORY( core )

# GUIScripts and override go directly in the bundle
SET(RESOURCE_LIST "GUIScripts" "override" "unhardcoded")
SET(RESOURCE_FILES "")

FOREACH(RESOURCE_DIR_NAME ${RESOURCE_LIST})
	SET(RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${RESOURCE_DIR_NAME}")
	IF (APPLE)
		FILE(GLOB CHILDREN RELATIVE "${RESOURCE_DIR}" ${RESOURCE_DIR}/*)
		FOREACH(CHILD ${CHILDREN})
			IF(IS_DIRECTORY "${RESOURCE_DIR}/${CHILD}")
				# be wary of .DS_Store files. glob them out by using [A-Za-z]*.*
				FILE(GLOB RESOURCE_DIR_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${RESOURCE_DIR}/${CHILD}/[A-Za-z]*.*")
				SET_SOURCE_FILES_PROPERTIES(
					${RESOURCE_DIR_FILES}
					PROPERTIES MACOSX_PACKAGE_LOCATION Resources/${RESOURCE_DIR_NAME}/${CHILD}
				)
				SET(RESOURCE_FILES ${RESOURCE_FILES} ${RESOURCE_DIR_FILES})
			ELSE()
				IF (NOT ${CHILD} MATCHES "\\.DS_Store|.*\\.am")
					SET_SOURCE_FILES_PROPERTIES(
						"${RESOURCE_DIR}/${CHILD}"
						PROPERTIES MACOSX_PACKAGE_LOCATION Resources/${RESOURCE_DIR_NAME}
					)
					SET(RESOURCE_FILES ${RESOURCE_FILES} "${RESOURCE_DIR_NAME}/${CHILD}")
				ENDIF()
			ENDIF()
		ENDFOREACH()
	ELSE (APPLE)
		INSTALL( DIRECTORY ${RESOURCE_DIR} DESTINATION "${DATA_DIR}" PATTERN "*.am" EXCLUDE)
	ENDIF (APPLE)
ENDFOREACH()

IF(APPLE) #application bundle generation
	# icon
	SET_SOURCE_FILES_PROPERTIES(
		../artwork/gemrb.icns
		PROPERTIES MACOSX_PACKAGE_LOCATION Resources
	)

	# default values
	SET_SOURCE_FILES_PROPERTIES(
		../apple/defaults.plist
		PROPERTIES MACOSX_PACKAGE_LOCATION Resources
	)

	SET(MACOSX_BUNDLE_ICON_FILE gemrb)
	ADD_EXECUTABLE(gemrb MACOSX_BUNDLE ../apple/CocoaWrapper/mac/CocoaWrapper.mm ../apple/AppleLogger.mm
					../artwork/gemrb.icns ../apple/defaults.plist
					${RESOURCE_FILES})
	SET (BUNDLE_LOADER "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.app/Contents/MacOS/${PROJECT_NAME}") # BUNDLE_LOADER is used by plugins needing to extend the cocoa wrapper (SDLVideo).
	SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -bundle_loader \"${BUNDLE_LOADER}\"")
ELSE(APPLE)
	ADD_SUBDIRECTORY( docs )
	ADD_SUBDIRECTORY( tests )

	ADD_EXECUTABLE(gemrb GemRB.cpp )

	INSTALL( FILES "${CMAKE_CURRENT_BINARY_DIR}/GemRB.cfg.sample" GemRB.cfg.noinstall.sample DESTINATION ${EXAMPLE_CONF_DIR} )
ENDIF(APPLE)

# we need to add gemrb target before processing plugins or COCOA plugins don't work right.
ADD_SUBDIRECTORY( plugins )

SET_TARGET_PROPERTIES(gemrb PROPERTIES INSTALL_RPATH ${LIB_DIR})
INCLUDE_DIRECTORIES( ${SDL_INCLUDE_DIR} )
IF(WIN32)
 TARGET_LINK_LIBRARIES(gemrb gemrb_core)
ELSE(WIN32)
 IF(APPLE)
	# custom info.plist
	SET_TARGET_PROPERTIES(gemrb PROPERTIES MACOSX_BUNDLE_INFO_PLIST ../../apple/osx/Info.plist)

	# compile nib
	ADD_CUSTOM_COMMAND (TARGET gemrb POST_BUILD 
                      COMMAND ${IBTOOL} --errors --warnings --notices --output-format human-readable-text 
                              --compile ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Resources/GemRB.nib 
                            ../../apple/CocoaWrapper/mac/nibs/GemRB.xib
                      COMMENT "Compiling GemRB.xib")

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
