/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/MVEPlayer/MVEPlay.cpp,v 1.6 2003/11/25 13:48:00 balrog994 Exp $
 *
 */

#include "../Core/Interface.h"
#include "MVEPlay.h"

#define MVESignature "Interplay MVE File\x1A\0"
#define MVEMagicWord "\x00\x1a\x01\x00\x11\x33"
#define MVE_SIGNATURE_LEN 20

MVEPlay::MVEPlay(void)
{
	str = NULL;
	autoFree = false;
	micro_frame_delay = 0;
	timer_started = 0;
	timer_expire = 0;

	mve_audio_curbuf_curpos = 0;
	mve_audio_bufhead = 0;
	mve_audio_buftail = 0;
	mve_audio_playing = 0;
	mve_audio_canplay = 0;
	mve_audio_spec = NULL;
	mve_audio_mutex = NULL;

	g_pCurMap=NULL;
	g_nMapLength=0;

	stupefaction=0;

	stream = NULL;
}

MVEPlay::~MVEPlay(void)
{
	if(str && autoFree)
		delete(str);
	if(g_vBackBuf1)
		free(g_vBackBuf1);
	if(g_vBackBuf2)
		free(g_vBackBuf2);
}

bool MVEPlay::Open(DataStream * stream, bool autoFree)
{
	validVideo = false;
	if(stream == NULL)
		return false;
	if(str && this->autoFree)
		delete(str);
	str = stream;
	this->autoFree = autoFree;
	char Signature[MVE_SIGNATURE_LEN];
	str->Read(Signature, MVE_SIGNATURE_LEN);
	if(memcmp(Signature, MVESignature, MVE_SIGNATURE_LEN) != 0) {
		if(memcmp(Signature, "BIK", 3) == 0) {
			printf("Warning!!! This is a Bink Video File...\nUnfortunately we cannot provide a Bink Video Player\nWe are sorry!\n");
			return true;
		}
		return false;
	}
	
	str->Seek(0, GEM_STREAM_START);
	validVideo = true;
	return true;
}

int MVEPlay::Play()
{
	if(!validVideo)
		return 0;
	MVESTREAM *mve = mve_open(this->str);
    if (mve == NULL)
    {
        fprintf(stderr, "can't open MVE file\n");
        return 1;
    }

    initializeMovie(mve);
    playMovie(mve);
    shutdownMovie(mve);

    mve_close(mve);

    return 0;
}

void MVEPlay::decodeFrame(unsigned char *pFrame, unsigned char *pMap, int mapRemain, unsigned char *pData, int dataRemain)
{
	int i, j;
	int xb, yb;

	xb = g_width >> 3;
	yb = g_height >> 3;
	for (j=0; j<yb; j++)
	{
		for (i=0; i<xb/2; i++)
		{
			dispatchDecoder(&pFrame, (*pMap) & 0xf, &pData, &dataRemain, &i, &j);
			if (pFrame < g_vBackBuf1)
				fprintf(stderr, "danger!  pointing out of bounds below after dispatch decoder: %d, %d (1) [%x]\n", i, j, (*pMap) & 0xf);
			else if (pFrame >= g_vBackBuf1 + g_width*g_height)
				fprintf(stderr, "danger!  pointing out of bounds above after dispatch decoder: %d, %d (1) [%x]\n", i, j, (*pMap) & 0xf);
			dispatchDecoder(&pFrame, (*pMap) >> 4, &pData, &dataRemain, &i, &j);
			if (pFrame < g_vBackBuf1)
				fprintf(stderr, "danger!  pointing out of bounds below after dispatch decoder: %d, %d (2) [%x]\n", i, j, (*pMap) >> 4);
			else if (pFrame >= g_vBackBuf1 + g_width*g_height)
				fprintf(stderr, "danger!  pointing out of bounds above after dispatch decoder: %d, %d (2) [%x]\n", i, j, (*pMap) >> 4);

			++pMap;
			--mapRemain;
		}

		pFrame += 7*g_width;
	}
}

void MVEPlay::dispatchDecoder(unsigned char **pFrame, unsigned char codeType, unsigned char **pData, int *pDataRemain, int *curXb, int *curYb)
{
    unsigned char p[4];
    unsigned char pat[16];
    int i, j, k;
    int x, y;

    switch(codeType)
    {
        case 0x0:
                  copyFrame(*pFrame, *pFrame + (g_vBackBuf2 - g_vBackBuf1));
                  *pFrame += 8;
                  break;
        case 0x1:
                  *pFrame += 8;
                  break;

        case 0x2:
                  relFar(*(*pData)++, 1, &x, &y);
                  copyFrame(*pFrame, *pFrame + x + y*g_width);
                  *pFrame += 8;
                  --*pDataRemain;
                  break;

        case 0x3:
                  relFar(*(*pData)++, -1, &x, &y);
                  copyFrame(*pFrame, *pFrame + x + y*g_width);
                  *pFrame += 8;
                  --*pDataRemain;
                  break;

        case 0x4:
                  relClose(*(*pData)++, &x, &y);
                  copyFrame(*pFrame, *pFrame + (g_vBackBuf2 - g_vBackBuf1) + x + y*g_width);
                  *pFrame += 8;
                  --*pDataRemain;
                  break;

        case 0x5:
                  x = (char)*(*pData)++;
                  y = (char)*(*pData)++;
                  copyFrame(*pFrame, *pFrame + (g_vBackBuf2 - g_vBackBuf1) + x + y*g_width);
                  *pFrame += 8;
                  *pDataRemain -= 2;
                  break;

        case 0x6:
/* appears to be unused
                  for (i=0; i<2; i++)
                  {
                      *pFrame += 16;
                      if (++*curXb == (g_width >> 3))
                      {
                          *pFrame += 7*g_width;
                          *curXb = 0;
                          if (++*curYb == (g_height >> 3))
                              return;
                      }
                  }
*/
                  break;

        case 0x7:
                  p[0] = *(*pData)++;
                  p[1] = *(*pData)++;
                  if (p[0] <= p[1])
                  {
                      for (i=0; i<8; i++)
                      {
                          patternRow2Pixels(*pFrame, *(*pData)++, p);
                          *pFrame += g_width;
                      }
                  }
                  else
                  {
                      for (i=0; i<2; i++)
                      {
                          patternRow2Pixels2(*pFrame, *(*pData) & 0xf, p);
                          *pFrame += 2*g_width;
                          patternRow2Pixels2(*pFrame, *(*pData)++ >> 4, p);
                          *pFrame += 2*g_width;
                      }
                  }
                  *pFrame -= (8*g_width - 8);
                  break;

        case 0x8:
                  if ( (*pData)[0] <= (*pData)[1])
                  {
                      for (i=0; i<4; i++)
                      {
                          p[0] = *(*pData)++;
                          p[1] = *(*pData)++;
                          pat[0] = *(*pData)++;
                          pat[1] = *(*pData)++;
                          patternQuadrant2Pixels(*pFrame, pat[0], pat[1], p);

                          if (i & 1)
                              *pFrame -= (4*g_width - 4);
                          else
                              *pFrame += 4*g_width;
                      }
                  }
                  else if ( (*pData)[6] <= (*pData)[7])
                  {
                      for (i=0; i<4; i++)
                      {
                          if ((i & 1) == 0)
                          {
                              p[0] = *(*pData)++;
                              p[1] = *(*pData)++;
                          }
                          pat[0] = *(*pData)++;
                          pat[1] = *(*pData)++;
                          patternQuadrant2Pixels(*pFrame, pat[0], pat[1], p);

                          if (i & 1)
                              *pFrame -= (4*g_width - 4);
                          else
                              *pFrame += 4*g_width;
                      }
                  }
                  else
                  {
                      for (i=0; i<8; i++)
                      {
                          if ((i & 3) == 0)
                          {
                              p[0] = *(*pData)++;
                              p[1] = *(*pData)++;
                          }
                          patternRow2Pixels(*pFrame, *(*pData)++, p);
                          *pFrame += g_width;
                      }
                      *pFrame -= (8*g_width - 8);
                  }
                  break;

        case 0x9:
                  if ( (*pData)[0] <= (*pData)[1])
                  {
                      if ( (*pData)[2] <= (*pData)[3])
                      {
                          p[0] = *(*pData)++;
                          p[1] = *(*pData)++;
                          p[2] = *(*pData)++;
                          p[3] = *(*pData)++;

                          for (i=0; i<8; i++)
                          {
                              pat[0] = *(*pData)++;
                              pat[1] = *(*pData)++;
                              patternRow4Pixels(*pFrame, pat[0], pat[1], p);
                              *pFrame += g_width;
                          }

                          *pFrame -= (8*g_width - 8);
                      }
                      else
                      {
                          p[0] = *(*pData)++;
                          p[1] = *(*pData)++;
                          p[2] = *(*pData)++;
                          p[3] = *(*pData)++;

                          patternRow4Pixels2(*pFrame, *(*pData)++, p);
                          *pFrame += 2*g_width;
                          patternRow4Pixels2(*pFrame, *(*pData)++, p);
                          *pFrame += 2*g_width;
                          patternRow4Pixels2(*pFrame, *(*pData)++, p);
                          *pFrame += 2*g_width;
                          patternRow4Pixels2(*pFrame, *(*pData)++, p);
                          *pFrame -= (6*g_width - 8);
                      }
                  }
                  else
                  {
                      if ( (*pData)[2] <= (*pData)[3])
                      {
                          p[0] = *(*pData)++;
                          p[1] = *(*pData)++;
                          p[2] = *(*pData)++;
                          p[3] = *(*pData)++;

                          for (i=0; i<8; i++)
                          {
                              pat[0] = *(*pData)++;
                              patternRow4Pixels2x1(*pFrame, pat[0], p);
                              *pFrame += g_width;
                          }

                          *pFrame -= (8*g_width - 8);
                      }
                      else
                      {
                          p[0] = *(*pData)++;
                          p[1] = *(*pData)++;
                          p[2] = *(*pData)++;
                          p[3] = *(*pData)++;

                          for (i=0; i<4; i++)
                          {
                              pat[0] = *(*pData)++;
                              pat[1] = *(*pData)++;
                              patternRow4Pixels(*pFrame, pat[0], pat[1], p);
                              *pFrame += g_width;
                              patternRow4Pixels(*pFrame, pat[0], pat[1], p);
                              *pFrame += g_width;
                          }

                          *pFrame -= (8*g_width - 8);
                      }
                  }
                  break;

        case 0xa:
                  if ( (*pData)[0] <= (*pData)[1])
                  {
                      for (i=0; i<4; i++)
                      {
                          p[0] = *(*pData)++;
                          p[1] = *(*pData)++;
                          p[2] = *(*pData)++;
                          p[3] = *(*pData)++;
                          pat[0] = *(*pData)++;
                          pat[1] = *(*pData)++;
                          pat[2] = *(*pData)++;
                          pat[3] = *(*pData)++;

                          patternQuadrant4Pixels(*pFrame, pat[0], pat[1], pat[2], pat[3], p);

                          if (i & 1)
                              *pFrame -= (4*g_width - 4);
                          else
                              *pFrame += 4*g_width;
                      }
                  }
                  else
                  {
                      if ( (*pData)[12] <= (*pData)[13])
                      {
                          for (i=0; i<4; i++)
                          {
                              if ((i&1) == 0)
                              {
                                  p[0] = *(*pData)++;
                                  p[1] = *(*pData)++;
                                  p[2] = *(*pData)++;
                                  p[3] = *(*pData)++;
                              }

                              pat[0] = *(*pData)++;
                              pat[1] = *(*pData)++;
                              pat[2] = *(*pData)++;
                              pat[3] = *(*pData)++;

                              patternQuadrant4Pixels(*pFrame, pat[0], pat[1], pat[2], pat[3], p);

                              if (i & 1)
                                  *pFrame -= (4*g_width - 4);
                              else
                                  *pFrame += 4*g_width;
                          }
                      }
                      else
                      {
                          for (i=0; i<8; i++)
                          {
                              if ((i&3) == 0)
                              {
                                  p[0] = *(*pData)++;
                                  p[1] = *(*pData)++;
                                  p[2] = *(*pData)++;
                                  p[3] = *(*pData)++;
                              }

                              pat[0] = *(*pData)++;
                              pat[1] = *(*pData)++;
                              patternRow4Pixels(*pFrame, pat[0], pat[1], p);
                              *pFrame += g_width;
                          }

                          *pFrame -= (8*g_width - 8);
                      }
                  }
                  break;

        case 0xb:
                  for (i=0; i<8; i++)
                  {
                      memcpy(*pFrame, *pData, 8);
                      *pFrame += g_width;
                      *pData += 8;
                      *pDataRemain -= 8;
                  }
                  *pFrame -= (8*g_width - 8);
                  break;

        case 0xc:
                  for (i=0; i<4; i++)
                  {
                      for (j=0; j<2; j++)
                      {
                          for (k=0; k<4; k++)
                          {
                              (*pFrame)[j+2*k] = (*pData)[k];
                              (*pFrame)[g_width+j+2*k] = (*pData)[k];
                          }
                          *pFrame += g_width;
                      }
                      *pData += 4;
                      *pDataRemain -= 4;
                  }
                  *pFrame -= (8*g_width - 8);
                  break;

        case 0xd:
                  for (i=0; i<2; i++)
                  {
                      for (j=0; j<4; j++)
                      {
                          for (k=0; k<4; k++)
                          {
                              (*pFrame)[k*g_width+j] = (*pData)[0];
                              (*pFrame)[k*g_width+j+4] = (*pData)[1];
                          }
                      }
                      *pFrame += 4*g_width;
                      *pData += 2;
                      *pDataRemain -= 2;
                  }
                  *pFrame -= (8*g_width - 8);
                  break;

        case 0xe:
                  for (i=0; i<8; i++)
                  {
                      memset(*pFrame, **pData, 8);
                      *pFrame += g_width;
                  }
                  ++*pData;
                  --*pDataRemain;
                  *pFrame -= (8*g_width - 8);
                  break;

        case 0xf:
                  for (i=0; i<8; i++)
                  {
                      for (j=0; j<8; j++)
                      {
                          (*pFrame)[j] = (*pData)[(i+j)&1];
                      }
                      *pFrame += g_width;
                  }
                  *pData += 2;
                  *pDataRemain -= 2;
                  *pFrame -= (8*g_width - 8);
                  break;

        default:
            break;
    }
}
//MVELib Functions
/*
 * open an MVE file
 */
MVEFILE *MVEPlay::mvefile_open(DataStream * stream)//const char *filename)
{
    MVEFILE *file;

    /* create the file */
    file = _mvefile_alloc();
    if (! _mvefile_open(file, stream))
    {
        _mvefile_free(file);
        return NULL;
    }

    /* initialize the file */
    _mvefile_set_buffer_size(file, 1024);

    /* verify the file's header */
    if (! _mvefile_read_header(file))
    {
        _mvefile_free(file);
        return NULL;
    }

    /* now, prefetch the next chunk */
    _mvefile_fetch_next_chunk(file);

    return file;
}

/*
 * close a MVE file
 */
void MVEPlay::mvefile_close(MVEFILE *movie)
{
    _mvefile_free(movie);
}

/*
 * get the size of the next segment
 */
int MVEPlay::mvefile_get_next_segment_size(MVEFILE *movie)
{
    /* if nothing is cached, fail */
    if (movie->cur_chunk == NULL  ||  movie->next_segment >= movie->cur_fill)
        return -1;

    /* if we don't have enough data to get a segment, fail */
    if (movie->cur_fill - movie->next_segment < 4)
        return -1;

    /* otherwise, get the data length */
    return _mve_get_short(movie->cur_chunk + movie->next_segment);
}

/*
 * get type of next segment in chunk (0xff if no more segments in chunk)
 */
unsigned char MVEPlay::mvefile_get_next_segment_major(MVEFILE *movie)
{
    /* if nothing is cached, fail */
    if (movie->cur_chunk == NULL  ||  movie->next_segment >= movie->cur_fill)
        return 0xff;

    /* if we don't have enough data to get a segment, fail */
    if (movie->cur_fill - movie->next_segment < 4)
        return 0xff;

    /* otherwise, get the data length */
    return movie->cur_chunk[movie->next_segment + 2];
}

/*
 * get subtype (version) of next segment in chunk (0xff if no more segments in
 * chunk)
 */
unsigned char MVEPlay::mvefile_get_next_segment_minor(MVEFILE *movie)
{
    /* if nothing is cached, fail */
    if (movie->cur_chunk == NULL  ||  movie->next_segment >= movie->cur_fill)
        return 0xff;

    /* if we don't have enough data to get a segment, fail */
    if (movie->cur_fill - movie->next_segment < 4)
        return 0xff;

    /* otherwise, get the data length */
    return movie->cur_chunk[movie->next_segment + 3];
}

/*
 * see next segment (return NULL if no next segment)
 */
unsigned char *MVEPlay::mvefile_get_next_segment(MVEFILE *movie)
{
    /* if nothing is cached, fail */
    if (movie->cur_chunk == NULL  ||  movie->next_segment >= movie->cur_fill)
        return NULL;

    /* if we don't have enough data to get a segment, fail */
    if (movie->cur_fill - movie->next_segment < 4)
        return NULL;

    /* otherwise, get the data length */
    return movie->cur_chunk + movie->next_segment + 4;
}

/*
 * advance to next segment
 */
void MVEPlay::mvefile_advance_segment(MVEFILE *movie)
{
    /* if nothing is cached, fail */
    if (movie->cur_chunk == NULL  ||  movie->next_segment >= movie->cur_fill)
        return;

    /* if we don't have enough data to get a segment, fail */
    if (movie->cur_fill - movie->next_segment < 4)
        return;

    /* else, advance to next segment */
    movie->next_segment +=
        (4 + _mve_get_short(movie->cur_chunk + movie->next_segment));
}

/*
 * fetch the next chunk (return 0 if at end of stream)
 */
int MVEPlay::mvefile_fetch_next_chunk(MVEFILE *movie)
{
    return _mvefile_fetch_next_chunk(movie);
}

/************************************************************
 * public MVESTREAM functions
 ************************************************************/

/*
 * open an MVE stream
 */
//Edit By Balrog994
MVESTREAM *MVEPlay::mve_open(DataStream *stream)//const char *filename)
{
    MVESTREAM *movie;

    /* allocate */
    movie = _mvestream_alloc();

    /* open */
    if (! _mvestream_open(movie, stream))
    {
        _mvestream_free(movie);
        return NULL;
    }

    return movie;
}

/*
 * close an MVE stream
 */
void MVEPlay::mve_close(MVESTREAM *movie)
{
    _mvestream_free(movie);
}

/*
 * set segment type handler
 */
void MVEPlay::mve_set_handler(MVESTREAM *movie, unsigned char major, MVESEGMENTHANDLER handler)
{
    if (major < 32)
        movie->handlers[major] = handler;
}

/*
 * set segment handler context
 */
void MVEPlay::mve_set_handler_context(MVESTREAM *movie, void *context)
{
    movie->context = context;
}

/*
 * play next chunk
 */
int MVEPlay::mve_play_next_chunk(MVESTREAM *movie)
{
    unsigned char major, minor;
    unsigned char *data;
    int len;

    /* loop over segments */
    major = mvefile_get_next_segment_major(movie->movie);
    while (major != 0xff)
    {
        /* check whether to handle the segment */
        if (major < 32  &&  movie->handlers[major] != NULL)
        {
            minor = mvefile_get_next_segment_minor(movie->movie);
            len = mvefile_get_next_segment_size(movie->movie);
            data = mvefile_get_next_segment(movie->movie);

            if (! movie->handlers[major](major, minor, data, len, movie->context))
                return 0;
        }
	else
	{
		printf("Unhandled opcode: %d\n",major);
		return 0;
	}

        /* advance to next segment */
        mvefile_advance_segment(movie->movie);
        major = mvefile_get_next_segment_major(movie->movie);
    }

    if (! mvefile_fetch_next_chunk(movie->movie))
        return 0;

    /* return status */
    return 1;
}

/************************************************************
 * private functions
 ************************************************************/

/*
 * allocate an MVEFILE
 */
MVEFILE *MVEPlay::_mvefile_alloc(void)
{
    MVEFILE *file = (MVEFILE *)malloc(sizeof(MVEFILE));
    file->stream = NULL;
    file->cur_chunk = NULL;
    file->buf_size = 0;
    file->cur_fill = 0;
    file->next_segment = 0;

    return file;
}

/*
 * free an MVE file
 */
void MVEPlay::_mvefile_free(MVEFILE *movie)
{
    /* free the stream */
	//In GemRB no need to free the stream directly
    //if (movie->stream)
    //    fclose(movie->stream);
    movie->stream = NULL;

    /* free the buffer */
    if (movie->cur_chunk)
        free(movie->cur_chunk);
    movie->cur_chunk = NULL;

    /* not strictly necessary */
    movie->buf_size = 0;
    movie->cur_fill = 0;
    movie->next_segment = 0;

    /* free the struct */
    free(movie);
}

/*
 * open the file stream in thie object
 */
int MVEPlay::_mvefile_open(MVEFILE *file, DataStream * stream)//const char *filename)
{
	if(stream == NULL)
		return 0;
	file->stream = stream;
	return 1;
    /*if (! (file->stream = fopen(filename, "rb")))
        return 0;

    return 1;*/
}

/*
 * read and verify the header of the recently opened file
 */
int MVEPlay::_mvefile_read_header(MVEFILE *movie)
{
    unsigned char buffer[26];

    /* check the file is open */
    if (movie->stream == NULL)
        return 0;

    /* check the file is long enough */
	if (movie->stream->Read(buffer, 26) < 26)//fread(buffer, 1, 26, movie->stream) < 26)
        return 0;

    /* check the signature */
    if (memcmp(buffer, MVE_HEADER, 20))
        return 0;

    /* check the hard-coded constants */
    if (_mve_get_short(buffer+20) != MVE_HDRCONST1)
        return 0;
    if (_mve_get_short(buffer+22) != MVE_HDRCONST2)
        return 0;
    if (_mve_get_short(buffer+24) != MVE_HDRCONST3)
        return 0;

    return 1;
}

void MVEPlay::_mvefile_set_buffer_size(MVEFILE *movie, int buf_size)
{
    unsigned char *new_buffer;
    int new_len;

    /* check if this would be a redundant operation */
    if (buf_size  <=  movie->buf_size)
        return;

    /* allocate new buffer */
    new_len = 100 + buf_size;
    new_buffer = (unsigned char*)malloc(new_len);

    /* copy old data */
    if (movie->cur_chunk  &&  movie->cur_fill)
        memcpy(new_buffer, movie->cur_chunk, movie->cur_fill);

    /* free old buffer */
    if (movie->cur_chunk)
    {
        free(movie->cur_chunk);
        movie->cur_chunk = 0;
    }

    /* install new buffer */
    movie->cur_chunk = new_buffer;
    movie->buf_size = new_len;
}

int MVEPlay::_mvefile_fetch_next_chunk(MVEFILE *movie)
{
    unsigned char buffer[4];
    unsigned short length;

    /* fail if not open */
    if (movie->stream == NULL)
        return 0;

    /* fail if we can't read the next segment descriptor */
	if (movie->stream->Read(buffer, 4) < 4)//fread(buffer, 1, 4, movie->stream) < 4)
        return 0;

    /* pull out the next length */
    length = _mve_get_short(buffer);

    /* make sure we've got sufficient space */
    _mvefile_set_buffer_size(movie, length);

    /* read the chunk */
	if (movie->stream->Read(movie->cur_chunk, length) < length)//fread(movie->cur_chunk, 1, length, movie->stream) < length)
        return 0;
    movie->cur_fill = length;
    movie->next_segment = 0;

    return 1;
}

short MVEPlay::_mve_get_short(unsigned char *data)
{
    short value;
    value = data[0] | (data[1] << 8);
    return value;
}

/*
 * allocate an MVESTREAM
 */
MVESTREAM * MVEPlay::_mvestream_alloc(void)
{
    MVESTREAM *movie;

    /* allocate and zero-initialize everything */
    movie = (MVESTREAM *)malloc(sizeof(MVESTREAM));
    movie->movie = NULL;
    movie->context = 0;
    memset(movie->handlers, 0, sizeof(movie->handlers));

    return movie;
}

/*
 * free an MVESTREAM
 */
void MVEPlay::_mvestream_free(MVESTREAM *movie)
{
    /* close MVEFILE */
    if (movie->movie)
        mvefile_close(movie->movie);
    movie->movie = NULL;

    /* clear context and handlers */
    movie->context = NULL;
    memset(movie->handlers, 0, sizeof(movie->handlers));
}

/*
 * open an MVESTREAM object
 */
int MVEPlay::_mvestream_open(MVESTREAM *movie, DataStream * stream)//const char *filename)
{
    movie->movie = mvefile_open(stream);

    return (movie->movie == NULL) ? 0 : 1;
}
