SET(CMAKE_CXX_VISIBILITY_PRESET "hidden")
INCLUDE(GenerateExportHeader)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/includes)

IF(USE_TRACY)
  ADD_SUBDIRECTORY(plugins/Tracy)
ENDIF()
ADD_SUBDIRECTORY( core )

# GUIScripts and override go directly in the bundle
SET(RESOURCE_LIST "GUIScripts" "override" "unhardcoded")
IF (NOT SKIP_DEMO_DATA)
	LIST(APPEND RESOURCE_LIST "../demo")
ENDIF()
IF (NOT OPENGL_BACKEND STREQUAL "None")
	LIST(APPEND RESOURCE_LIST "plugins/SDLVideo/Shaders")
ENDIF()
SET(RESOURCE_FILES "")

FOREACH(RESOURCE_REL_PATH ${RESOURCE_LIST})
	SET(RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${RESOURCE_REL_PATH}")
	IF (APPLE)
		get_filename_component(RES_DIR "${RESOURCE_REL_PATH}" NAME)
		FILE(GLOB CHILDREN RELATIVE "${RESOURCE_DIR}" ${RESOURCE_DIR}/*)
		FOREACH(CHILD ${CHILDREN})
			IF(IS_DIRECTORY "${RESOURCE_DIR}/${CHILD}")
				# be wary of .DS_Store files. glob them out by using [A-Za-z]*.*
				FILE(GLOB RESOURCE_DIR_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${RESOURCE_DIR}/${CHILD}/[A-Za-z]*.*")
				SET_SOURCE_FILES_PROPERTIES(
					${RESOURCE_DIR_FILES}
					PROPERTIES MACOSX_PACKAGE_LOCATION Resources/${RES_DIR}/${CHILD}
				)
				SET(RESOURCE_FILES ${RESOURCE_FILES} ${RESOURCE_DIR_FILES})
			ELSEIF(NOT ${CHILD} MATCHES "\\.DS_Store")
				SET_SOURCE_FILES_PROPERTIES(
					"${RESOURCE_DIR}/${CHILD}"
					PROPERTIES MACOSX_PACKAGE_LOCATION Resources/${RES_DIR}
				)
				SET(RESOURCE_FILES ${RESOURCE_FILES} "${RESOURCE_REL_PATH}/${CHILD}")
			ENDIF()
		ENDFOREACH()
	ELSE (APPLE)
		INSTALL(DIRECTORY ${RESOURCE_DIR} DESTINATION "${DATA_DIR}")
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
		../platforms/apple/defaults.plist
		PROPERTIES MACOSX_PACKAGE_LOCATION Resources
	)

	SET(MACOSX_BUNDLE_ICON_FILE gemrb)
	SET(PRODUCT_NAME "GemRB") # for info.plist
	
	ADD_EXECUTABLE(gemrb MACOSX_BUNDLE
					../platforms/apple/CocoaWrapper/mac/CocoaWrapper.mm
					../platforms/apple/AppleLogger.mm
					../platforms/apple/EmptyStringTransformer.m
					../artwork/gemrb.icns
					../platforms/apple/defaults.plist
					${RESOURCE_FILES})

	SET(BUNDLE_FRAMEWORK_PATH "${CMAKE_BINARY_DIR}/gemrb/${PROJECT_NAME}.app/Contents/Frameworks")
	
	get_filename_component(SDL_BASENAME "${SDL2_LIBDIR}" NAME)
	
	IF(NOT SDL_BASENAME MATCHES ".framework")
		SET(SDL_BASENAME "libSDL2.dylib")
		SET(SDL_LIB "${SDL2_LIBDIR}/${SDL_BASENAME}")
		get_filename_component(SDL_FULL_PATH "${SDL_LIB}" REALPATH)

		FILE(COPY ${SDL_FULL_PATH} DESTINATION ${BUNDLE_FRAMEWORK_PATH} FOLLOW_SYMLINK_CHAIN FILE_PERMISSIONS OWNER_READ OWNER_WRITE)
		
		# change the "install name" of the copied SDL
		ADD_CUSTOM_COMMAND(TARGET gemrb PRE_BUILD COMMAND ${CMAKE_INSTALL_NAME_TOOL} -id "@loader_path/../Frameworks/${SDL_BASENAME}" "${SDL_FULL_PATH}")
		IF(USE_SDLMIXER)
			FILE(COPY "${SDL_MIXER_LIBRARIES}" DESTINATION ${BUNDLE_FRAMEWORK_PATH} FOLLOW_SYMLINK_CHAIN FILE_PERMISSIONS OWNER_READ OWNER_WRITE)
		ENDIF()
	ELSE()
		SET(SDL_LIB "${SDL2_LIBDIR}")
	ENDIF()
	
	MESSAGE(STATUS "Copying SDL: ${SDL_LIB}")
	FILE(COPY ${SDL_LIB} DESTINATION ${BUNDLE_FRAMEWORK_PATH} FILE_PERMISSIONS OWNER_READ OWNER_WRITE)

	SET(BUNDLE_LOADER "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.app/Contents/MacOS/${PROJECT_NAME}") # BUNDLE_LOADER is used by plugins needing to extend the cocoa wrapper (SDLVideo).
	STRING(APPEND CMAKE_MODULE_LINKER_FLAGS " -bundle_loader \"${BUNDLE_LOADER}\"")
ELSE(APPLE)
	ADD_SUBDIRECTORY( docs )
	ADD_SUBDIRECTORY( tests )

	SET(PLATFORM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../platforms")

	IF(WIN32)
		ADD_EXECUTABLE(gemrb ${PLATFORM_DIR}/windows/GemRB.cpp ${PLATFORM_DIR}/windows/window.rc)
	ELSEIF(VITA)
		ADD_EXECUTABLE(gemrb ${PLATFORM_DIR}/vita/GemRB.cpp ${PLATFORM_DIR}/vita/VitaLogger.cpp)
	ELSEIF(ANDROID)
		ADD_EXECUTABLE(gemrb ${PLATFORM_DIR}/android/GemRB.cpp ${PLATFORM_DIR}/android/AndroidLogger.cpp)
	ELSE(WIN32)
		ADD_EXECUTABLE(gemrb GemRB.cpp )
	ENDIF()

	INSTALL( FILES "${CMAKE_CURRENT_BINARY_DIR}/GemRB.cfg.sample" GemRB.cfg.noinstall.sample DESTINATION ${EXAMPLE_CONF_DIR} )
	INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/GemRB.cfg.sample" RENAME GemRB.cfg DESTINATION ${EXAMPLE_CONF_DIR})
ENDIF(APPLE)
target_compile_definitions(gemrb PRIVATE _USE_MATH_DEFINES)

# we need to add gemrb target before processing plugins or COCOA plugins don't work right.
ADD_SUBDIRECTORY( plugins )

# create a compilable icon from a bmp
FUNCTION(create_icon file output filename)
	FILE(READ ${file} filedata HEX)
	# convert hex data for C compatibility
	STRING(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," filedata ${filedata})
	FILE(WRITE "${PROJECT_BINARY_DIR}/${output}" "const unsigned char ${filename}[] = {${filedata}};\nconst unsigned ${filename}_size = sizeof(${filename});\n")
ENDFUNCTION()
create_icon("${CMAKE_SOURCE_DIR}/artwork/gemrb.bmp" "gemrb-icon.h" "gemrb_icon")

SET_TARGET_PROPERTIES(gemrb PROPERTIES INSTALL_RPATH ${LIB_DIR})
INCLUDE_DIRECTORIES( ${SDL_INCLUDE_DIR} )
IF(APPLE)
  # custom info.plist
  SET(PRODUCT_BUNDLE_IDENTIFIER "org.gemrb.gemrb")
  SET(EXECUTABLE_NAME "${PROJECT_NAME}")
  SET_TARGET_PROPERTIES(gemrb PROPERTIES BUNDLE True MACOSX_BUNDLE_INFO_PLIST ../platforms/apple/osx/Info.plist OUTPUT_NAME ${PROJECT_NAME})

  # compile nib
  ADD_CUSTOM_COMMAND (TARGET gemrb POST_BUILD
                      COMMAND ${IBTOOL} --errors --warnings --notices --output-format human-readable-text 
                              --compile ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Resources/GemRB.nib 
                            ${CMAKE_CURRENT_SOURCE_DIR}/../platforms/apple/CocoaWrapper/mac/nibs/GemRB.xib
                      COMMENT "Compiling GemRB.xib")

  if (STATIC_LINK)
    TARGET_LINK_LIBRARIES(gemrb
        ${COCOA_LIBRARY_PATH} ${CMAKE_DL_LIBS} Threads::Threads
       gemrb_core ${plugins}  -Wl,-all_load)
  else (STATIC_LINK)
    TARGET_LINK_LIBRARIES(gemrb gemrb_core
        ${COCOA_LIBRARY_PATH} ${CMAKE_DL_LIBS} Threads::Threads)
  endif (STATIC_LINK)
ELSE(APPLE)
  if (STATIC_LINK)
    TARGET_LINK_LIBRARIES(gemrb ${CMAKE_DL_LIBS} Threads::Threads
      -Wl,--whole-archive gemrb_core ${plugins} -Wl,--no-whole-archive)
  else (STATIC_LINK)
    IF (MSVC)
      TARGET_LINK_LIBRARIES(gemrb gemrb_core)
    ELSE (MSVC)
      TARGET_LINK_LIBRARIES(gemrb gemrb_core ${CMAKE_DL_LIBS} Threads::Threads)
    ENDIF (MSVC)
  endif (STATIC_LINK)
ENDIF(APPLE)

# preconfigure the sample config with the selected paths
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/GemRB.cfg.sample.in"
  "${CMAKE_CURRENT_BINARY_DIR}/GemRB.cfg.sample"
  @ONLY
)

# copy config for complex tests to build dir
CONFIGURE_FILE(../demo/tester.cfg "${CMAKE_BINARY_DIR}")

INSTALL( TARGETS gemrb DESTINATION ${BIN_DIR} )

# optional script to help deploy dependencies when building with windows.
IF(VCPKG_AUTO_DEPLOY)
	INCLUDE(${CMAKE_SOURCE_DIR}/platforms/windows/vcpkg_deps.cmake)
ENDIF()

IF(VITA)
	INCLUDE(${CMAKE_SOURCE_DIR}/platforms/vita/vita_build_vpk.cmake)
ENDIF(VITA)

# Tests
IF (BUILD_TESTING)
  ADD_EXECUTABLE(Test_gemrb_core
    tests/core/Test_Map.cpp
    tests/core/Test_MurmurHash.cpp
    tests/core/Test_Orient.cpp
    tests/core/Test_Palette.cpp
    tests/core/Streams/Test_DataStream.cpp
    tests/core/Strings/Test_CString.cpp
    tests/core/Strings/Test_String.cpp
    tests/core/Strings/Test_StringView.cpp
    tests/core/Strings/Test_UTF8Comparison.cpp
    tests/core/System/Test_VFS.cpp
  )

  target_compile_definitions(Test_gemrb_core PRIVATE _USE_MATH_DEFINES)
  TARGET_LINK_LIBRARIES(Test_gemrb_core GTest::gtest GTest::gtest_main gemrb_core ${Iconv_LIBRARY})
  IF (WIN32)
    TARGET_LINK_LIBRARIES(Test_gemrb_core shlwapi)
  ENDIF()

  IF (MSVC)
    ADD_TEST(NAME Test_gemrb_core
      COMMAND ${CMAKE_COMMAND} -E env "PATH=${CMAKE_BINARY_DIR}/gemrb;$ENV{PATH}" Test_gemrb_core
      WORKING_DIRECTORY $<TARGET_FILE_DIR:gemrb_core>
    )
  ELSE()
    ADD_TEST(NAME Test_gemrb_core COMMAND Test_gemrb_core
      WORKING_DIRECTORY $<TARGET_FILE_DIR:gemrb_core>
    )
  ENDIF()

  ADD_CUSTOM_COMMAND(TARGET Test_gemrb_core POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:gemrb_core>/tests/resources
      && ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/gemrb/tests/resources $<TARGET_FILE_DIR:gemrb_core>/tests/resources
  )
ENDIF()
