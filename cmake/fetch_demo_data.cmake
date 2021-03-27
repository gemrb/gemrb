# downloads big audio and other non-essential files from gemrb-assets

set(DL_FROM audio/themea.ogg
	audio/whispers-seamless.ogg
)
set(DL_TO music/theme/themea.ogg
	music/mx0100/mx0100a.ogg
)
file(MAKE_DIRECTORY music/theme music/mx0100)

# NOTE: no need to edit anything below this line

set(BASE_URL https://github.com/gemrb/gemrb-assets/raw/master/demo/)

list(LENGTH DL_FROM N1)
list(LENGTH DL_TO N2)
if(NOT ${N1} EQUAL ${N2})
	message(FATAL_ERROR "Source and destination lists not of the same size!")
endif()

foreach(I RANGE ${N1})
	if(${I} EQUAL ${N1})
		break()
	endif()
	list(GET DL_FROM ${I} FILE_IN)
	list(GET DL_TO ${I} FILE_OUT)

	message("Download ${I}: ${BASE_URL}/${FILE_IN} to ${FILE_OUT}")
	file(DOWNLOAD ${BASE_URL}/${FILE_IN} ${FILE_OUT} SHOW_PROGRESS)
endforeach()
