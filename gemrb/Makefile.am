bin_PROGRAMS = gemrb
data_DATA = GemRB.cfg
gemrb_SOURCES = GemRB.cpp 

gemrb_LDADD = ./plugins/AREImporter/libAREImporter.la ./plugins/BAMImporter/libBAMImporter.la ./plugins/BG2HCAnim/libBG2HCAnim.la ./plugins/BIFImporter/libBIFImporter.la ./plugins/BMPImporter/libBMPImporter.la ./plugins/CHUImporter/libCHUImporter.la ./plugins/Core/libcore.la ./plugins/CREImporter/libCREImporter.la ./plugins/IDSImporter/libIDSImporter.la	./plugins/KEYImporter/libKEYImporter.la ./plugins/MOSImporter/libMOSImporter.la ./plugins/MVEPlayer/libMVEPlayer.la ./plugins/SDLVideo/libSDLVideo.la ./plugins/TISImporter/libTISImporter.la ./plugins/TLKImporter/libTLKImporter.la ./plugins/WEDImporter/libWEDImporter.la ./plugins/ZLibMgr/libZLibMgr.la ./plugins/GUIScript/libGUIScript.la ./plugins/ACMImporter/libACMImporter.la ./plugins/MUSImporter/libMUSImporter.la ./plugins/2DAImporter/lib2DAImporter.la ./plugins/INIImporter/libINIImporter.la ./plugins/PLTImporter/libPLTImporter.la
SUBDIRS = plugins GUIScripts override
EXTRA_DIST = GemRB.cfg GemRB.cpp 
INCLUDES= \
	$(all_includes) \
	$(PYTHON_INCLUDES)
	
gemrb_LDFLAGS = $(all_libraries) 

install-data-local:
	$(INSTALL_DATA) GemRB.cfg $(bindir)
