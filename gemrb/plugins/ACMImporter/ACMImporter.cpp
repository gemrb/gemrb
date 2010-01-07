/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2004 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Id$
 *
 */

#include "ACMImporter.h"

ACMImp::ACMImp()
{
	SoundReader = 0;
}

bool ACMImp::Open(DataStream* stream, bool autofree)
{
	delete SoundReader;
	SoundReader = 0;

	if (!stream) {
		return false;
	}
	char Signature[4];
	ieDword SignatureDword;
	stream->Read( Signature, 4 );
	stream->Seek( 0, GEM_STREAM_START );
	stream->ReadDword( &SignatureDword );
	stream->Seek( 0, GEM_STREAM_START );
#ifdef HAS_VORBIS_SUPPORT
	if(strnicmp(Signature, "oggs", 4) == 0) {
		SoundReader = CreateSoundReader(stream, SND_READER_OGG, stream->Size(), autofree) ;
	} //ogg
#endif
	if(strnicmp(Signature, "RIFF", 4) == 0) {
		SoundReader = CreateSoundReader(stream, SND_READER_WAV, stream->Size(), autofree) ;
	} //wav
	if (SignatureDword == IP_ACM_SIG) {
		SoundReader = CreateSoundReader(stream, SND_READER_ACM, stream->Size(), autofree) ;
	} //acm
	if (memcmp( Signature, "WAVC", 4 ) == 0) {
		SoundReader = CreateSoundReader(stream, SND_READER_ACM, stream->Size(), autofree) ;
	} //wavc

	if (SoundReader)
        return true ;

	return false;
}

ACMImp::~ACMImp()
{
    delete SoundReader ;
}

int ACMImp::get_channels()
{
    return SoundReader->get_channels() ;
}

int ACMImp::get_length()
{
    return SoundReader->get_length() ;
}

int ACMImp::get_samplerate()
{
    return SoundReader->get_samplerate() ;
}

int ACMImp::read_samples(short* memory, int cnt)
{
    return SoundReader->read_samples(memory, cnt) ;
}

void ACMImp::rewind()
{
    SoundReader->rewind() ;
}

#include "../../includes/plugindef.h"

GEMRB_PLUGIN(0x1AE768FE, "ACM File Importer")
PLUGIN_CLASS(IE_WAV_CLASS_ID, ACMImp)
END_PLUGIN()
