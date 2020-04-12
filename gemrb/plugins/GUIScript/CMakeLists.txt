INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
ADD_GEMRB_PLUGIN (GUIScript GUIScript.cpp PythonHelpers.cpp)
IF(MINGW AND CMAKE_SIZEOF_VOID_P EQUAL 8)
	ADD_DEFINITIONS(-DMS_WIN64)
ENDIF()
# remove this once we move to python3
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")
TARGET_LINK_LIBRARIES(GUIScript ${PYTHON_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})

add_executable (run_script run_script.cpp GUIScript.cpp PythonHelpers.cpp)
set_target_properties (run_script PROPERTIES COMPILE_FLAGS
	"${CMAKE_CXX_FLAGS} -fno-strict-aliasing -Wno-deprecated-declarations")
target_link_libraries (run_script
	${PYTHON_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} gemrb_core)
