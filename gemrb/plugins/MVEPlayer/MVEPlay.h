#ifndef MVEPLAY_H
#define MVEPLAY_H

#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include "../Core/MoviePlayer.h"
#include "../Core/Interface.h"
#include "../../includes/sdl/SDL.h"
#include "../../includes/sdl/SDL_thread.h"
#include "../../includes/fmod/fmod.h"
//#include "mvelib.h"
#include "mve_audio.h"

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

typedef struct MVEFILE
{
    //FILE           *stream;
	DataStream     *stream;
    unsigned char  *cur_chunk;
    unsigned int    buf_size;
    unsigned int    cur_fill;
    unsigned int    next_segment;
} MVEFILE;

/*
* callback for segment type
*/
typedef int (*MVESEGMENTHANDLER)(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context);

/*
* structure for maintaining an MVE stream
*/
typedef struct MVESTREAM
{
	MVEFILE                    *movie;
	void                       *context;
	MVESEGMENTHANDLER           handlers[32];
} MVESTREAM;

static const char  MVE_HEADER[]  = "Interplay MVE File\x1A";
static const short MVE_HDRCONST1 = 0x001A;
static const short MVE_HDRCONST2 = 0x0100;
static const short MVE_HDRCONST3 = 0x1133;

static int g_spdFactorNum = 0;
static int g_spdFactorDenom = 10;
/*
* timer variables
*/
static int micro_frame_delay = 0;
static int timer_started = 0;
static unsigned long timer_expire = 0;

static short *mve_audio_buffers[64];
static int    mve_audio_buflens[64];
static int    mve_audio_curbuf_curpos = 0;
static int mve_audio_bufhead = 0;
static int mve_audio_buftail = 0;
static int mve_audio_playing = 0;
static int mve_audio_canplay = 0;
static SDL_AudioSpec *mve_audio_spec = NULL;
static SDL_mutex *mve_audio_mutex = NULL;

static SDL_Surface *g_screen;
static int g_screenWidth, g_screenHeight;
static int g_width, g_height;
static unsigned char g_palette[768];
static unsigned char *g_vBackBuf1, *g_vBackBuf2;
static unsigned char *g_pCurMap=NULL;
static int g_nMapLength=0;

static int stupefaction=0;

static FSOUND_STREAM * stream = NULL;

class MVEPlay :	public MoviePlayer
{
private: //MVELib Functions

	/*
	 * private utility functions
	 */
	static short _mve_get_short(unsigned char *data);

	/*
	 * private functions for mvefile
	 */
	static MVEFILE *_mvefile_alloc(void);
	static void _mvefile_free(MVEFILE *movie);
	static int _mvefile_open(MVEFILE *movie, DataStream * stream);
	static int  _mvefile_read_header(MVEFILE *movie);
	static void _mvefile_set_buffer_size(MVEFILE *movie, int buf_size);
	static int _mvefile_fetch_next_chunk(MVEFILE *movie);

	/*
	* private functions for mvestream
	*/
	static MVESTREAM *_mvestream_alloc(void);
	static void _mvestream_free(MVESTREAM *movie);
	static int _mvestream_open(MVESTREAM *movie, DataStream * stream);//const char *filename);

	/*
	* open a .MVE file
	*/
	static MVEFILE *mvefile_open(DataStream * stream);//const char *filename);

	/*
	* close a .MVE file
	*/
	static void mvefile_close(MVEFILE *movie);

	/*
	* get size of next segment in chunk (-1 if no more segments in chunk)
	*/
	static int mvefile_get_next_segment_size(MVEFILE *movie);

	/*
	* get type of next segment in chunk (0xff if no more segments in chunk)
	*/
	static unsigned char mvefile_get_next_segment_major(MVEFILE *movie);

	/*
	* get subtype (version) of next segment in chunk (0xff if no more segments in
	* chunk)
	*/
	static unsigned char mvefile_get_next_segment_minor(MVEFILE *movie);

	/*
	* see next segment (return NULL if no next segment)
	*/
	static unsigned char *mvefile_get_next_segment(MVEFILE *movie);

	/*
	* advance to next segment
	*/
	static void mvefile_advance_segment(MVEFILE *movie);

	/*
	* fetch the next chunk (return 0 if at end of stream)
	*/
	static int mvefile_fetch_next_chunk(MVEFILE *movie);

	/*
	* open an MVE stream
	*/
	MVESTREAM *mve_open(DataStream * stream);//const char *filename);

	/*
	* close an MVE stream
	*/
	static void mve_close(MVESTREAM *movie);

	/*
	* set segment type handler
	*/
	static void mve_set_handler(MVESTREAM *movie, unsigned char major, MVESEGMENTHANDLER handler);

	/*
	* set segment handler context
	*/
	void mve_set_handler_context(MVESTREAM *movie, void *context);

	/*
	* play next chunk
	*/
	static int mve_play_next_chunk(MVESTREAM *movie);

private: //Decoder Variables
	

private: //Decoder Functions
	static void initializeMovie(MVESTREAM *mve)
	{
		mve_audio_mutex = SDL_CreateMutex();
		mve_set_handler(mve, 0x00, end_movie_handler);
		mve_set_handler(mve, 0x01, end_chunk_handler);
		mve_set_handler(mve, 0x02, create_timer_handler);
		mve_set_handler(mve, 0x03, create_audiobuf_handler);
		mve_set_handler(mve, 0x04, play_audio_handler);
		mve_set_handler(mve, 0x05, create_videobuf_handler);
		mve_set_handler(mve, 0x07, display_video_handler);
		mve_set_handler(mve, 0x08, audio_data_handler);
		mve_set_handler(mve, 0x09, audio_data_handler);
		mve_set_handler(mve, 0x0a, init_video_handler);
		mve_set_handler(mve, 0x0c, video_palette_handler);
		mve_set_handler(mve, 0x0f, video_codemap_handler);
		mve_set_handler(mve, 0x11, video_data_handler);
	}

	static void playMovie(MVESTREAM *mve)
	{
		int init_timer=0;
		int cont=1;
		while (cont)
		{
			cont = mve_play_next_chunk(mve);
			if (micro_frame_delay  &&  !init_timer)
			{
				timer_start();
				init_timer = 1;
			}

			do_timer_wait();
		}
	}
	static void shutdownMovie(MVESTREAM *mve)
	{
		FSOUND_Stream_Stop(stream);
		FSOUND_Stream_Close(stream);
		SDL_DestroyMutex(mve_audio_mutex);
		//SDL_CloseAudio();
	}
	static short get_short(unsigned char *data)
	{
		short value;
		value = data[0] | (data[1] << 8);
		return value;
	};

	static int get_int(unsigned char *data)
	{
		int value;
		value = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
		return value;
	};

	static int default_seg_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
	{
		fprintf(stderr, "unknown chunk type %02x/%02x\n", major, minor);
		return 1;
	};

	/*************************
	* general handlers
	*************************/
	static int end_movie_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
	{
	    //printf("Movie End Handler\n");
		return 0;
	};
	/*************************
	* timer handlers
	*************************/

	static int create_timer_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
	{
#ifdef WIN32		
		unsigned __int64 temp;
#else
		long long temp;
#endif
		micro_frame_delay = get_int(data) * (int)get_short(data+4);
		if (g_spdFactorNum != 0)
		{
		    temp = micro_frame_delay;
		    temp *= g_spdFactorNum;
		    temp /= g_spdFactorDenom;
		    micro_frame_delay = (int)temp;
		}
	
		return 1;
	};

	static void timer_start(void)
	{
		timer_expire = SDL_GetTicks();
		timer_expire += (micro_frame_delay/1000)+1;
		timer_started=1;
	};

	static void do_timer_wait(void)
	{
		unsigned long ts, tv;
		if (! timer_started)
			return;

		tv = SDL_GetTicks();
		if(tv > timer_expire)
			goto end;

		ts = timer_expire - tv;
		SDL_Delay(ts);

end:
		timer_expire += (micro_frame_delay/1000)+1;
	};

	/*************************
	* audio handlers
	*************************/
	//static void mve_audio_callback(void *userdata, unsigned char *stream, int len); 

	//static void mve_audio_callback(void *userdata, unsigned char *stream, int len)
	static signed char __stdcall mve_audio_callback(FSOUND_STREAM * fsoundstream, void *buffer, int len1, int param)
	{
		unsigned char * stream = (unsigned char*)buffer;
		int len = len1;
		int total=0;
		int length;
		//printf("Audio Callback\n");

    SDL_mutexP(mve_audio_mutex);
		
		if (mve_audio_bufhead == mve_audio_buftail) {
			SDL_mutexV(mve_audio_mutex);
			return true;
		}
			
		//fprintf(stderr, "+ <%d (%d), %d, %d>\n", mve_audio_bufhead, mve_audio_curbuf_curpos, mve_audio_buftail, len);

		while (mve_audio_bufhead != mve_audio_buftail                                       /* while we have more buffers  */
				&&  len > (mve_audio_buflens[mve_audio_bufhead]-mve_audio_curbuf_curpos))   /* and while we need more data */
		{
			length = mve_audio_buflens[mve_audio_bufhead]-mve_audio_curbuf_curpos;
			memcpy(stream,																	/* cur output position */
					mve_audio_buffers[mve_audio_bufhead]+mve_audio_curbuf_curpos,           /* cur input position  */
					length);                                                                /* cur input length    */

			total += length;
			stream += length;                                                               /* advance output */
			len -= length;                                                                  /* decrement avail ospace */
			free(mve_audio_buffers[mve_audio_bufhead]);                                     /* free the buffer */
			mve_audio_buffers[mve_audio_bufhead]=NULL;                                      /* free the buffer */
			mve_audio_buflens[mve_audio_bufhead]=0;                                         /* free the buffer */

			if (++mve_audio_bufhead == 64)                                                  /* next buffer */
				mve_audio_bufhead = 0;
			mve_audio_curbuf_curpos = 0;
		}

		//fprintf(stderr, "= <%d (%d), %d, %d>: %d\n", mve_audio_bufhead, mve_audio_curbuf_curpos, mve_audio_buftail, len, total);

		if (len != 0                                                                        /* ospace remaining  */
				&&  mve_audio_bufhead != mve_audio_buftail)                                 /* buffers remaining */
		{
			memcpy(stream,																	/* dest */
					mve_audio_buffers[mve_audio_bufhead] + mve_audio_curbuf_curpos,         /* src */
					len);                                                                   /* length */

			mve_audio_curbuf_curpos += len;                                                 /* advance input */
			stream += len;                                                                  /* advance output (unnecessary) */
			len -= len;                                                                     /* advance output (unnecessary) */

			if (mve_audio_curbuf_curpos >= mve_audio_buflens[mve_audio_bufhead])            /* if this ends the current chunk */
			{
				free(mve_audio_buffers[mve_audio_bufhead]);                                 /* free buffer */
				mve_audio_buffers[mve_audio_bufhead]=NULL;
				mve_audio_buflens[mve_audio_bufhead]=0;

				if (++mve_audio_bufhead == 64)                                              /* next buffer */
					mve_audio_bufhead = 0;
				mve_audio_curbuf_curpos = 0;
			}
		}

		//fprintf(stderr, "- <%d (%d), %d, %d>\n", mve_audio_bufhead, mve_audio_curbuf_curpos, mve_audio_buftail, len);

		SDL_mutexV(mve_audio_mutex);
		return true;
	}

	static int create_audiobuf_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
	{
		int sample_rate;
		int desired_buffer;

		//fprintf(stderr, "creating audio buffers\n");
		if(stream)
			return 1;
		sample_rate = get_short(data + 4);
		desired_buffer = get_int(data + 6);
		//mve_audio_spec = (SDL_AudioSpec *)malloc(sizeof(SDL_AudioSpec));
		//mve_audio_spec->freq = sample_rate;
		//mve_audio_spec->format = AUDIO_S16LSB;
		//mve_audio_spec->channels = 2;
		//mve_audio_spec->samples = desired_buffer/4;
		//mve_audio_spec->callback = mve_audio_callback;
		//mve_audio_spec->userdata = NULL;
		stream = FSOUND_Stream_Create(mve_audio_callback, desired_buffer, FSOUND_LOOP_OFF | FSOUND_16BITS | FSOUND_STEREO | FSOUND_2D | FSOUND_STREAMABLE | FSOUND_SIGNED, sample_rate, 0);
		if(stream)
		//if (SDL_OpenAudio(mve_audio_spec, NULL) >= 0)
			{
			//fprintf(stderr, "   success\n");
			mve_audio_canplay = 1;
			}
		else
			{
			//fprintf(stderr, "   failure : %s\n", SDL_GetError());
			mve_audio_canplay = 0;
			}
		
		//mve_audio_canplay = 0;

		memset(mve_audio_buffers, 0, sizeof(mve_audio_buffers));
		memset(mve_audio_buflens, 0, sizeof(mve_audio_buflens));
		if (mve_audio_canplay  &&  !mve_audio_playing)//  &&  mve_audio_bufhead != mve_audio_buftail)
			{
			FSOUND_Stream_Play(0, stream);
			FSOUND_SetPaused(0, true);
			}
    //play_audio_handler(0, 0, NULL, 0, NULL);
			//printf("Starting Audio Playback\n");
		//	SDL_PauseAudio(0);
		//	mve_audio_playing = 1;
		//	}
		return 1;
	}

	static int play_audio_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
	{
		printf("Play Audio\n");
		if (mve_audio_canplay  &&  !mve_audio_playing)//  &&  mve_audio_bufhead != mve_audio_buftail)
			{
			//SDL_PauseAudio(0);
			//FSOUND_Stream_Play(0, stream);
			FSOUND_SetPaused(0, false);
			mve_audio_playing = 1;
			}

		return 1;
	}

	static int audio_data_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
	{
		static const int selected_chan=1;
		int chan;
		int nsamp;
		if (mve_audio_canplay)
		{
			if (mve_audio_playing)
				FSOUND_SetPaused(0, true);
				//SDL_LockAudio();

			chan = get_short(data + 2);
			nsamp = get_short(data + 4);
			if (chan & selected_chan)
				{
				SDL_mutexP(mve_audio_mutex);
				mve_audio_buflens[mve_audio_buftail] = nsamp;
				mve_audio_buffers[mve_audio_buftail] = (short *)malloc(nsamp*sizeof(short));
				printf("Allocating %d samples on position %d\n", nsamp, mve_audio_buftail);
				if (major == 8)
					mveaudio_uncompress(mve_audio_buffers[mve_audio_buftail], data, -1); /* XXX */
				else
					memset(mve_audio_buffers[mve_audio_buftail], 0, nsamp*sizeof(short)); /* XXX */

				if (++mve_audio_buftail == 64)
					mve_audio_buftail = 0;

				if (mve_audio_buftail == mve_audio_bufhead)
					fprintf(stderr, "d'oh!  buffer ring overrun (%d)\n", mve_audio_bufhead);
            
				SDL_mutexV(mve_audio_mutex);
				}

			if (mve_audio_playing)
				//SDL_UnlockAudio();
				FSOUND_SetPaused(0, false);
			}

		return 1;
	}

	/*************************
	* video handlers
	*************************/

	static int create_videobuf_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
	{
		//printf("Create Video Buffer\n");
		short w, h;
		w = get_short(data);
		h = get_short(data+2);
		g_width = w << 3;
		g_height = h << 3;
		g_vBackBuf1 = (unsigned char*)malloc(g_width * g_height * 1.5);
		g_vBackBuf2 = (unsigned char*)malloc(g_width * g_height * 1.5);
		memset(g_vBackBuf1, 0, g_width * g_height);
		memset(g_vBackBuf2, 0, g_width * g_height);
		return 1;
	}

	static int display_video_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
	{
		printf("Display Video\n");
		int i;
		unsigned char *pal = g_palette;
		unsigned char *pDest;
		unsigned char *pixels = g_vBackBuf1;
		SDL_Surface *screenSprite;
		SDL_Rect renderArea;
		int x, y;

		SDL_Surface *initSprite = SDL_CreateRGBSurface(SDL_SWSURFACE, g_width, g_height, 8, 0, 0, 0, 0);
		for(i = 0; i < 256; i++)
		{
			initSprite->format->palette->colors[i].r = (*pal++) << 2;
			initSprite->format->palette->colors[i].g = (*pal++) << 2;
			initSprite->format->palette->colors[i].b = (*pal++) << 2;
			initSprite->format->palette->colors[i].unused = 0;
		}

		pDest = (unsigned char*)initSprite->pixels;
		for (i=0; i<g_height; i++)
		{
			memcpy(pDest, pixels, g_width);
			pixels += g_width;
			pDest += initSprite->pitch;
		}

		screenSprite = SDL_DisplayFormat(initSprite);
		SDL_FreeSurface(initSprite);

		if (g_screenWidth > screenSprite->w) 
			x = (g_screenWidth - screenSprite->w) >> 1;
		else 
			x=0;
		if (g_screenHeight > screenSprite->h) 
			y = (g_screenHeight - screenSprite->h) >> 1;
		else 
			y=0;
		renderArea.x = x;
		renderArea.y = y;
		renderArea.w = MIN(g_screenWidth  - x, screenSprite->w);
		renderArea.h = MIN(g_screenHeight - y, screenSprite->h);
		SDL_BlitSurface(screenSprite, NULL, g_screen, &renderArea);
		if ( (g_screen->flags & SDL_DOUBLEBUF) == SDL_DOUBLEBUF )
			SDL_Flip(g_screen);
		else
			SDL_UpdateRects(g_screen, 1, &renderArea);
		SDL_Event event; /* Event structure */
		while(SDL_PollEvent(&event)) {  /* Loop until there are no events left on the queue */
			switch(event.type){  /* Process the appropiate event type */
			case SDL_QUIT:  /* Handle a KEYDOWN event */         
				SDL_FreeSurface(screenSprite);
				return 0;
			break;

			case SDL_KEYUP:
				if(event.key.keysym.sym == SDLK_ESCAPE)
				{
					SDL_FreeSurface(screenSprite);
					return 0;
				}
			break;
			}
		}
		//SDL_Flip(g_screen);
		SDL_FreeSurface(screenSprite);
		return 1;
	}

	static int init_video_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
	{
		//printf("Init Video\n");
		short width, height;
		width = get_short(data);
		height = get_short(data+2);
		g_screen = (SDL_Surface*)core->GetVideoDriver()->GetVideoSurface();//SDL_SetVideoMode(width, height, 16, SDL_ANYFORMAT);
		g_screenWidth = width;
		g_screenHeight = height;
		memset(g_palette, 0, 768);
		if(!SDL_WasInit(SDL_INIT_AUDIO)) {
			SDL_InitSubSystem(SDL_INIT_AUDIO);
		}
		return 1;
	}

	static int video_palette_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
	{
		short start, count;
		start = get_short(data);
		count = get_short(data+2);
		memcpy(g_palette + 3*start, data+4, 3*count);
		return 1;
	}

	static int video_codemap_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
	{
		g_pCurMap = data;
		g_nMapLength = len;
		return 1;
	}

	static void decodeFrame(unsigned char *pFrame, unsigned char *pMap, int mapRemain, unsigned char *pData, int dataRemain);

	static int video_data_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
	{
		printf("VideoData\n");
		short nFrameHot, nFrameCold;
		short nXoffset, nYoffset;
		short nXsize, nYsize;
		short nFlags;
		unsigned char *temp;

		nFrameHot  = get_short(data);
		nFrameCold = get_short(data+2);
		nXoffset   = get_short(data+4);
		nYoffset   = get_short(data+6);
		nXsize     = get_short(data+8);
		nYsize     = get_short(data+10);
		nFlags     = get_short(data+12);

		if (nFlags & 1)
			{
			temp = g_vBackBuf1;
			g_vBackBuf1 = g_vBackBuf2;
			g_vBackBuf2 = temp;
			}

		/* convert the frame */
		decodeFrame(g_vBackBuf1, g_pCurMap, g_nMapLength, data+14, len-14);

		return 1;
	}

	static int end_chunk_handler(unsigned char major, unsigned char minor, unsigned char *data, int len, void *context)
	{
		g_pCurMap=NULL;
		return 1;
	}

	static void dispatchDecoder(unsigned char **pFrame, unsigned char codeType, unsigned char **pData, int *pDataRemain, int *curXb, int *curYb);

	static void relClose(int i, int *x, int *y)
	{
		int ma, mi;

		ma = i >> 4;
		mi = i & 0xf;

		*x = mi - 8;
		*y = ma - 8;
	}

	static void relFar(int i, int sign, int *x, int *y)
	{
		if (i < 56)
			{
			*x = sign * (8 + (i % 7));
			*y = sign *      (i / 7);
			}
		else
			{
			*x = sign * (-14 + (i - 56) % 29);
			*y = sign *   (8 + (i - 56) / 29);
			}
	}

	static void copyFrame(unsigned char *pDest, unsigned char *pSrc)
	{
		int i;

		for (i=0; i<8; i++)
			{
			memcpy(pDest, pSrc, 8);
			pDest += g_width;
			pSrc += g_width;
			}
	}

	static void patternRow4Pixels(unsigned char *pFrame,
								  unsigned char pat0, unsigned char pat1,
								  unsigned char *p)
	{
		unsigned short mask=0x0003;
		unsigned short shift=0;
		unsigned short pattern = (pat1 << 8) | pat0;
	
		while (mask != 0)
			{
			*pFrame++ = p[(mask & pattern) >> shift];
			mask <<= 2;
			shift += 2;
			}
	}

	static void patternRow4Pixels2(unsigned char *pFrame,
								   unsigned char pat0,
								   unsigned char *p)
	{
		unsigned char mask=0x03;
		unsigned char shift=0;
		unsigned char pel;
		int skip=1;

		while (mask != 0)
			{
			pel = p[(mask & pat0) >> shift];
			pFrame[0] = pel;
			pFrame[2] = pel;
			pFrame[g_width + 0] = pel;
			pFrame[g_width + 2] = pel;
			pFrame += skip;
			skip = 4 - skip;
			mask <<= 2;
			shift += 2;
			}
	}

	static void patternRow4Pixels2x1(unsigned char *pFrame, unsigned char pat, unsigned char *p)
	{
		unsigned char mask=0x03;
		unsigned char shift=0;
		unsigned char pel;

		while (mask != 0)
			{
			pel = p[(mask & pat) >> shift];
			pFrame[0] = pel;
			pFrame[1] = pel;
			pFrame += 2;
			mask <<= 2;
			shift += 2;
			}
	}

	static void patternQuadrant4Pixels(unsigned char *pFrame, unsigned char pat0, unsigned char pat1, unsigned char pat2, unsigned char pat3, unsigned char *p)
	{
		unsigned long mask = 0x00000003UL;
		int shift=0;
		int i;
		unsigned long pat = (pat3 << 24) | (pat2 << 16) | (pat1 << 8) | pat0;

		for (i=0; i<16; i++)
			{
			pFrame[i&3] = p[(pat & mask) >> shift];

			if ((i&3) == 3)
				pFrame += g_width;

			mask <<= 2;
			shift += 2;
			}
	}


	static void patternRow2Pixels(unsigned char *pFrame, unsigned char pat, unsigned char *p)
	{
		unsigned char mask=0x01;

		while (mask != 0)
			{
			*pFrame++ = p[(mask & pat) ? 1 : 0];
			mask <<= 1;
		}
	}

	static void patternRow2Pixels2(unsigned char *pFrame, unsigned char pat, unsigned char *p)
	{
		unsigned char pel;
		unsigned char mask=0x1;
		int skip=1;

		while (mask != 0x10)
			{
			pel = p[(mask & pat) ? 1 : 0];
			pFrame[0] = pel;
			pFrame[2] = pel;
			pFrame[g_width + 0] = pel;
			pFrame[g_width + 2] = pel;
			pFrame += skip;
			skip = 4 - skip;
			mask <<= 1;
			}
	}

	static void patternQuadrant2Pixels(unsigned char *pFrame, unsigned char pat0, unsigned char pat1, unsigned char *p)
	{
		unsigned short mask = 0x0001;
		int i;
		unsigned short pat = (pat1 << 8) | pat0;

		for (i=0; i<16; i++)
		{
			pFrame[i&3] = p[(pat & mask) ? 1 : 0];

			if ((i&3) == 3)
				pFrame += g_width;

			mask <<= 1;
		}
	}

private:
	DataStream * str;
	bool autoFree;
	bool stopPlayback;

public:
	MVEPlay(void);
	~MVEPlay(void);
	bool Open(DataStream * stream, bool autoFree = true);
	int Play();
};

#endif
