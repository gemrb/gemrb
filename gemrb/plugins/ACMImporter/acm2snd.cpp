//#include "stdafx.h"
//acm2snd.cpp
// Acm to Wav conversion
#include <iostream>
#include <stdio.h>
#ifdef WIN32
#include <io.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif
#include <string.h>
#include "readers.h"
#include "general.h"
#include "riffhdr.h"

typedef unsigned char BYTE;

static CACMReader* acm = NULL;

void finalize() {
  if (acm) delete acm;
  acm=NULL;
}

int ConvertAcmWav(int fhandle,long maxlen, unsigned char *&memory, long &samples_written, int forcestereo)
{
  int riff_chans;
  long rawsize=0;
  long cnt, cnt1;
  RIFF_HEADER riff;
  
  memory=0;
  #ifdef WIN32
  if(maxlen==-1) maxlen=filelength(fhandle);
  #else
  if(maxlen==-1) {
		struct stat st;
		fstat(fhandle, &st);
		maxlen=st.st_size;
  }
  #endif
  try
  {
    //handling normal riff, it is not a standard way, but hopefully able to handle most of the files
    if(read(fhandle,&riff, 4) !=4) return 3;
    if(!memcmp(riff.riff_sig,"RIFF",4))
    {    
      if(read(fhandle,&riff.total_len_m8, sizeof(RIFF_HEADER)-4 )==sizeof(RIFF_HEADER)-4 )
      {
        // data_sig-wformattag=16
        if(riff.formatex_len!=(unsigned long) ((BYTE *) riff.data_sig-(BYTE *) &riff.wFormatTag))
        {
          cnt=riff.formatex_len-24;
          lseek(fhandle,cnt,SEEK_CUR);
          read(fhandle,riff.data_sig,8);//8 = sizeof sig+sizeof length
          riff.formatex_len=16;
        }
        if(!memcmp(riff.data_sig,"fact",4) ) //skip 'fact' (an optional RIFF tag)
        {
          cnt=riff.raw_data_len; 
          lseek(fhandle,cnt,SEEK_CUR);
          read(fhandle,riff.data_sig,8); //8 is still the same 
          if(memcmp(riff.data_sig,"data",4) )
          {
            finalize();
            return 3;
          }
        }
        memory=new unsigned char[riff.raw_data_len+sizeof(RIFF_HEADER)];
        if(!memory)
        {
          finalize();
          return 3;
        }
        maxlen-=sizeof(RIFF_HEADER);
        if(riff.raw_data_len>(unsigned long) maxlen)
        {
          riff.total_len_m8=maxlen+sizeof(RIFF_HEADER);
          riff.raw_data_len=maxlen;
        }
        memcpy(memory,&riff,sizeof(RIFF_HEADER) );
        samples_written = riff.raw_data_len+sizeof(RIFF_HEADER);
        if(read(fhandle,(unsigned char *) memory+sizeof(RIFF_HEADER),riff.raw_data_len)!=(long) riff.raw_data_len)
        {
          finalize();
          return 3;
        }
        finalize();
        return 0;
      }
    }
    lseek(fhandle,-4,SEEK_CUR);
    acm = (CACMReader*)CreateSoundReader(fhandle, SND_READER_ACM, maxlen);
    if (!acm)
    {
      //Error 1: Cannot create sound reader
      finalize();
      return 1;
    }
    
    cnt = acm->get_length();
    riff_chans = acm->get_channels();
    if(forcestereo && (riff_chans==1)) riff_chans=2;
    if(riff_chans!=1 && riff_chans!=2)
    {
      finalize();
      return 4;
    }
    
    memory=new unsigned char[cnt*2+sizeof(RIFF_HEADER)];
    if(!memory)
    {
      finalize();
      return 3;
    }
    samples_written = sizeof(RIFF_HEADER);  
    memset(memory,0,sizeof(memory));
    write_riff_header (memory, cnt, riff_chans, acm->get_samplerate());
    
    cnt1 = acm->read_samples ((short*)(memory+samples_written), cnt);
    rawsize=cnt1*sizeof(short);
    samples_written+=rawsize;
    cnt -= cnt1;
    
    finalize();
    if(cnt) return 4;
    return 0;
  }
  catch(...)
  {
    finalize();
		return 4;
  }
}
