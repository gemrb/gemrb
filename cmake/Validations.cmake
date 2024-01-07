IF(PREFIX)
	MESSAGE(FATAL_ERROR "CMake started using PREFIX internally, pass CMAKE_INSTALL_PREFIX instead!")
ENDIF(PREFIX)

# prevent in-source builds
IF(NOT INSOURCEBUILD AND (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR}))
	MESSAGE(FATAL_ERROR "
		CMake generation for this project is not allowed within the source directory!
		Remove the CMake cache files and try again from another folder, e.g.:
		  rm -r CMakeCache.txt CMakeFiles/
		  mkdir build
		  cd build
		  cmake ..
		If you really want an in-source build, pass -DINSOURCEBUILD=1"
	)
ENDIF()