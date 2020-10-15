include("${DOLCESDK}/share/dolce.cmake" REQUIRED)

# Display name (under bubble in LiveArea)
set(VITA_APP_NAME "GemRB")
# Unique ID must be exactly 9 characters. Recommended: XXXXYYYYY where X =
# unique string of developer and Y = a unique number for this app
set(VITA_TITLEID  "GEMRB0001")
# Optional version string to show in LiveArea's more info screen
string(REGEX REPLACE "^(.)\.(.)\.(.).*$" "0\\1.\\2\\3" VITA_VERSION ${GEMRB_VERSION})
set(DOLCE_MKSFOEX_FLAGS "${DOLCE_MKSFOEX_FLAGS} -d ATTRIBUTE2=12")

## Create Vita files
dolce_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})
# The FILE directive lets you add additional files to the VPK, the syntax is
# FILE src_path dst_path_in_vpk. In this case, we add the LiveArea paths.
dolce_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE ${CMAKE_SOURCE_DIR}/platforms/vita/sce_sys/icon0.png sce_sys/icon0.png
  FILE ${CMAKE_SOURCE_DIR}/platforms/vita/sce_sys/icon0.png sce_sys/icon0.png
  FILE ${CMAKE_SOURCE_DIR}/platforms/vita/sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
  FILE ${CMAKE_SOURCE_DIR}/platforms/vita/sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
  FILE ${CMAKE_SOURCE_DIR}/platforms/vita/sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)
