####### kdevelop will overwrite this part!!! (begin)##########
bin_PROGRAMS = gemrb

## INCLUDES were found outside kdevelop specific part

gemrb_SOURCES = GemRB.cpp 
gemrb_LDADD   = ./plugins/AREImporter/libAREImporter.la ./plugins/BAMImporter/libBAMImporter.la ./plugins/BG2HCAnim/libBG2HCAnim.la ./plugins/BIFImporter/libBIFImporter.la ./plugins/BMPImporter/libBMPImporter.la ./plugins/CHUImporter/libCHUImporter.la ./plugins/Core/libcore.la ./plugins/CREImporter/libCREImporter.la ./plugins/IDSImporter/libIDSImporter.la ./plugins/KEYImporter/libKEYImporter.la ./plugins/MOSImporter/libMOSImporter.la ./plugins/MVEPlayer/libMVEPlayer.la ./plugins/SDLVideo/libSDLVideo.la ./plugins/TISImporter/libTISImporter.la ./plugins/TLKImporter/libTLKImporter.la ./plugins/WEDImporter/libWEDImporter.la ./plugins/ZLibMgr/libZLibMgr.la 

SUBDIRS = plugins includes zlib 

EXTRA_DIST = GemRB.cfg GemRB.cpp plugins-prepare.sh 

####### kdevelop will overwrite this part!!! (end)############
# set the include path found by configure
INCLUDES= $(all_includes)
 
# the library search path.
gemrb_LDFLAGS = $(all_libraries) 
                                                                                
