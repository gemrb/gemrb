ADD_SUBDIRECTORY( 2DAImporter )
ADD_SUBDIRECTORY( ACMReader )
ADD_SUBDIRECTORY( AREImporter )
ADD_SUBDIRECTORY( BAMImporter )
ADD_SUBDIRECTORY( BIFImporter )
ADD_SUBDIRECTORY( BIKPlayer )
ADD_SUBDIRECTORY( BMPImporter )
ADD_SUBDIRECTORY( BMPWriter )
ADD_SUBDIRECTORY( CHUImporter )
ADD_SUBDIRECTORY( CREImporter )
ADD_SUBDIRECTORY( DLGImporter )
ADD_SUBDIRECTORY( DirectoryImporter )
ADD_SUBDIRECTORY( EFFImporter )
ADD_SUBDIRECTORY( FXOpcodes )
ADD_SUBDIRECTORY( GAMImporter )
ADD_SUBDIRECTORY( GUIScript )
ADD_SUBDIRECTORY( IDSImporter )
ADD_SUBDIRECTORY( INIImporter )
ADD_SUBDIRECTORY( ITMImporter )
ADD_SUBDIRECTORY( IWDOpcodes )
ADD_SUBDIRECTORY( KEYImporter )
ADD_SUBDIRECTORY( MOSImporter )
ADD_SUBDIRECTORY( MUSImporter )
ADD_SUBDIRECTORY( MVEPlayer )
ADD_SUBDIRECTORY( NullSound )
ADD_SUBDIRECTORY( NullSource )
ADD_SUBDIRECTORY( OGGReader )
ADD_SUBDIRECTORY( OpenALAudio )
ADD_SUBDIRECTORY( PLTImporter )
ADD_SUBDIRECTORY( PNGImporter )
ADD_SUBDIRECTORY( PROImporter )
ADD_SUBDIRECTORY( PVRZImporter )
ADD_SUBDIRECTORY( PSTOpcodes )
ADD_SUBDIRECTORY( SAVImporter )
ADD_SUBDIRECTORY( SDLAudio )
ADD_SUBDIRECTORY( SDLVideo )
ADD_SUBDIRECTORY( SPLImporter )
ADD_SUBDIRECTORY( STOImporter )
ADD_SUBDIRECTORY( TISImporter )
ADD_SUBDIRECTORY( TLKImporter )
ADD_SUBDIRECTORY( TTFImporter )
ADD_SUBDIRECTORY( VLCPlayer )
ADD_SUBDIRECTORY( WAVReader )
ADD_SUBDIRECTORY( WEDImporter )
ADD_SUBDIRECTORY( WMPImporter )
ADD_SUBDIRECTORY( ZLibManager )

if (STATIC_LINK)
	SET(plugins ${plugins} PARENT_SCOPE)
endif (STATIC_LINK)
