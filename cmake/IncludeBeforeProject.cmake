# VCPKG manifest file: platforms/windows/vcpkg.json
# must be declared before PROJECT()
IF(WIN32)
	SET(VCPKG_MANIFEST_DIR "${CMAKE_SOURCE_DIR}/platforms/windows" )
	message(STATUS "VCPKG_MANIFEST_DIR: ${VCPKG_MANIFEST_DIR}")
ENDIF(WIN32)