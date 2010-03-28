/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 *
 */

#ifndef ACMIMP_H
#define ACMIMP_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "readers.h"
#include "../Core/DataStream.h"
#include "../Core/SoundMgr.h"

#ifdef HAS_VORBIS_SUPPORT
#include <vorbis/vorbisfile.h>
#endif

#define INIT_NO_ERROR_MSG 0
#define INIT_NEED_ERROR_MSG 1

// Abstract Sound Reader class
class ACMImp : public SoundMgr {
private:
	CSoundReader* SoundReader ;

public:
    ACMImp() ;
    ~ACMImp() ;
    void release()
    {
        delete this ;
    }
    bool Open(DataStream* stream, bool autofree );
    int get_length() ;
    int get_channels() ;
    int get_samplerate() ;
    int read_samples(short* memory, int cnt) ;
    void rewind(void) ;
};

#endif //ACMIMP_H

