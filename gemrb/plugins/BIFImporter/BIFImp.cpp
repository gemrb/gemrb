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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/BIFImporter/BIFImp.cpp,v 1.7 2003/12/15 09:37:57 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "../Core/Compressor.h"
#include "../Core/FileStream.h"
#include "../Core/CachedFileStream.h"
#include "BIFImp.h"
#include "../Core/Interface.h"

BIFImp::BIFImp(void)
{
	stream = NULL;
}

BIFImp::~BIFImp(void)
{
#ifdef _DEBUG
	printf("BIFImp::~BIFImp()\n");
#endif
	if(stream) {
#ifdef _DEBUG
		printf("delete(stream);\n");
#endif
		delete(stream);
	}
}

int BIFImp::OpenArchive(char* filename, bool cacheCheck)
{
#ifdef _DEBUG
	printf("BIFImp::OpenArchive(%s, %s)\n", filename, (cacheCheck ? "true" : "false"));
#endif
	DataStream *stmp;
	if(stream) {
		delete(stream);
		stream = NULL;
	}
	//printf("[BifImporter]: Opening %s...", filename);
	FILE * t = fopen(filename, "rb");
	//printf("[OK]\n");
	char Signature[8];
	fread(&Signature, 1, 8, t);
	if(strncmp(Signature, "BIFF", 4) == 0)
		cacheCheck = true;
	else
		cacheCheck = false;
	//printf("[BifImporter]: Closing %s...", filename);
	fclose(t);
	//printf("[OK]\n");
	if(cacheCheck) {
		printf("Checking Cache\n");
		stream = new CachedFileStream(filename);
		stream->Read(Signature, 8);
	}
	else {
		FileStream *s = new FileStream();
		s->Open(filename, true);
		stmp = s;
		stmp->Read(Signature, 8);
	}
	if(strncmp(Signature, "BIFFV1  ", 8) == 0) {
		printf("Uncompressed File Found\n");
		strcpy(path, filename);
		ReadBIF();
	}
	else if(strncmp(Signature, "BIF V1.0", 8) == 0) {
		printf("'BIF V1.0' File Found\n");
		unsigned long fnlen, complen, declen;
		stmp->Read(&fnlen, 4);
		char * fname = (char*)malloc(fnlen);
		stmp->Read(fname, fnlen);
		stmp->Read(&declen, 4);
		stmp->Read(&complen, 4);
		strcpy(path, core->CachePath);
		strcat(path, fname);
		free(fname);
		//printf("[BifImporter]: Opening %s...", path);
		FILE * tmp = fopen(path, "rb");
		if(tmp) {
			printf("Found in Cache\n");
			//printf("[OK] -> File Exists\n[BifImporter]: Closing %s...", path);
			fclose(tmp);
			//printf("[OK]\n");
			delete(stmp);
			stream = new CachedFileStream(path);
			stream->Read(Signature, 8);
			if(strncmp(Signature, "BIFFV1  ", 8) == 0)
				ReadBIF();
			else
				return GEM_ERROR;
		}
		else {
			printf("Decompressing\n");
			if(!core->IsAvailable(IE_COMPRESSION_CLASS_ID))
				return GEM_ERROR;
			Compressor * comp = (Compressor*)core->GetInterface(IE_COMPRESSION_CLASS_ID);
			void * inb = malloc(complen);
			stmp->Read(inb, complen);
			void * outb = malloc(declen);
			comp->Decompress(outb, &declen, inb, complen);
			free(inb);
			core->FreeInterface(comp);
			//printf("[BifImporter]: Opening %s...", path);
			tmp = fopen(path, "wb");
			//printf("[OK]\n");
			fwrite(outb, 1, declen, tmp);
			//printf("[BifImporter]: Closing %s...", path);
			fclose(tmp);
			free(outb);
			//printf("[OK]\n");
			delete(stmp);
			if(stream)
				printf("DANGER WILL ROBINSON!!! stream != NULL in BIFImp line 96, possible File Pointer Leak...");
			stream = new CachedFileStream(path);
			stream->Read(Signature, 8);
			if(strncmp(Signature, "BIFFV1  ", 8) == 0)
				ReadBIF();
			else
				return GEM_ERROR;
		}
	}
	else if(strncmp(Signature, "BIFCV1.0", 8) == 0) {
		printf("'BIFCV1.0' Compressed File Found\n");
		char cpath[_MAX_PATH];
		strcpy(cpath, core->CachePath);
		strcat(cpath, stmp->filename);
		FILE * exist_in_cache = fopen(cpath, "rb");
		if(exist_in_cache) {
			printf("Found in Cache\n");
			//printf("[OK] -> File Exists\n[BifImporter]: Closing %s...", path);
			fclose(exist_in_cache);
			//printf("[OK]\n");
			delete(stmp);
			if(stream)
				printf("DANGER WILL ROBINSON!!! stream != NULL in BIFImp line 116, possible File Pointer Leak...");
			stream = new CachedFileStream(cpath);
			stream->Read(Signature, 8);
			if(strncmp(Signature, "BIFFV1  ", 8) == 0) {
				ReadBIF();				
				return GEM_OK;
			}
			else
				return GEM_ERROR;
		}
		printf("Decompressing\n");
		if(!core->IsAvailable(IE_COMPRESSION_CLASS_ID))
			return GEM_ERROR;
		Compressor * comp = (Compressor*)core->GetInterface(IE_COMPRESSION_CLASS_ID);
		int unCompBifSize, deCompBlockSize, CompBlockSize;
		stmp->Read(&unCompBifSize, 4);
		printf("\nDecompressing file: [..........]");
		char fname[_MAX_PATH];
		ExtractFileFromPath(fname, filename);
		strcpy(path, core->CachePath);
		strcat(path, fname);
		//printf("[BifImporter]: Opening %s...", path);
		t = fopen(path, "wb");
		//printf("[OK]\n");
		int finalsize = 0;
		int lastInSize = 0, lastOutSize = 0;
		unsigned long tempout = 0;
		void * inbuf = NULL;
		void * outbuf = NULL;
		int laststep = 0;
		while(finalsize < unCompBifSize) {
			stmp->Read(&deCompBlockSize, 4);
			stmp->Read(&CompBlockSize, 4);
			if(lastInSize < CompBlockSize) {
				inbuf = realloc(inbuf, CompBlockSize);
				lastInSize = CompBlockSize;
			}
			if(lastOutSize < deCompBlockSize) {
				outbuf = realloc(outbuf, deCompBlockSize);
				lastOutSize = deCompBlockSize;
			}
			tempout = deCompBlockSize;
			stmp->Read(inbuf, CompBlockSize);
			comp->Decompress(outbuf, &tempout, inbuf, CompBlockSize);
			fwrite(outbuf, 1, tempout, t);
			finalsize += tempout;
			if((int)(finalsize*(10.0/unCompBifSize)) != laststep) {
				laststep++;
				printf("\b\b\b\b\b\b\b\b\b\b\b");
				for(int l = 0; l < laststep; l++)
					printf("|");
				for(int l = laststep; l < 10; l++)
					printf(".");
				printf("]");
			}
		}
		core->FreeInterface(comp);
		//printf("[BifImporter]: Closing %s...", path);
		fclose(t);
		//printf("[OK]\n");
		delete(stmp);
		free(inbuf);
		free(outbuf);
		strcpy(filename, path);
		if(stream)
			printf("DANGER WILL ROBINSON!!! stream != NULL in BIFImp line 116, possible File Pointer Leak...");
		stream = new CachedFileStream(filename);
		stream->Read(Signature, 8);
		if(strncmp(Signature, "BIFFV1  ", 8) == 0)
			ReadBIF();
		else
			return GEM_ERROR;
	}
	else
		return GEM_ERROR;
	return GEM_OK;
}

DataStream* BIFImp::GetStream(unsigned long Resource, unsigned long Type)
{
	printf("[BIFImporter]: Searching for a 0x%04lX Resource...", Type);
	DataStream * s = NULL;
	if(Type == IE_TIS_CLASS_ID) {
		unsigned int srcResLoc = Resource & 0xFC000;
		for(unsigned long i = 0; i < tentries.size(); i++) {
			if((tentries[i].resLocator & 0xFC000) == srcResLoc) {
				s = new CachedFileStream(stream, tentries[i].dataOffset, tentries[i].fileSize);
				break;
			}
		}
	}
	else {
		unsigned int srcResLoc = Resource & 0x3FFF;
		for(unsigned long i = 0; i < fentries.size(); i++) {
			if((fentries[i].resLocator & 0x3FFF) == srcResLoc) {
				s = new CachedFileStream(stream, fentries[i].dataOffset, fentries[i].fileSize);
				break;
			}
		}
	}
	if(s)
		printf("[FOUND]\n");
	else
		printf("[NOT_FOUND]\n");
	return s;
}
void BIFImp::ReadBIF(void)
{
	unsigned long fentcount, tentcount, foffset;
	stream->Read(&fentcount, 4);
	stream->Read(&tentcount, 4);
	stream->Read(&foffset, 4);
	stream->Seek(foffset, GEM_STREAM_START);
	for(unsigned long i = 0; i < fentcount; i++) {
		FileEntry fe;
		stream->Read(&fe, 16);
		fentries.push_back(fe);
	}
	for(unsigned long i = 0; i < tentcount; i++) {
		TileEntry te;
		stream->Read(&te, 20);
		tentries.push_back(te);
	}
}
