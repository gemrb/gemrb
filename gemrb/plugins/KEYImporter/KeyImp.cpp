#include "../../includes/win32def.h"
#include "KeyImp.h"
#include "../../includes/globals.h"
#include "../Core/FileStream.h"
#include "../Core/Interface.h"
#include "../Core/ArchiveImporter.h"
#include "../Core/AnimationMgr.h"
#ifndef WIN32
#include <unistd.h>
#endif

#ifndef WIN32
#include <ctype.h>
char *strlwr(char *string)
{
	char *s;
	if(string)
	{
		for(s = string; *s; ++s)
			*s = tolower(*s);
	}
	return string;
}
#endif

KeyImp::KeyImp(void)
{
}

KeyImp::~KeyImp(void)
{
	for(unsigned int i = 0; i < biffiles.size(); i++) {
		free(biffiles[i].name);
	}
}

bool KeyImp::LoadResFile(const char * resfile)
{
	printf("[KEY Importer]: Opening %s...", resfile);
	FileStream * f = new FileStream();
	if(!f->Open(resfile)) {
		printf("[ERROR]\nCannot open Chitin.key\n");
		delete(f);
		return false;
	}
	printf("[OK]\nChecking file type...");
	char Signature[8];
	f->Read(Signature, 8);
	if(strncmp(Signature, "KEY V1  ", 8) != 0) {
		printf("[ERROR]\nFile has an Invalid Signature.\n");
		delete(f);
		return false;
	}
	printf("[OK]\nReading Resources...\n");
	unsigned long BifCount, ResCount, BifOffset, ResOffset;
	f->Read(&BifCount, 4);
	f->Read(&ResCount, 4);
	f->Read(&BifOffset, 4);
	f->Read(&ResOffset, 4);
	printf("BIF Files Count: %ld (Starting at %ld Bytes)\nRES Count: %ld (Starting at %ld Bytes)\n", BifCount, BifOffset, ResCount, ResOffset);
	f->Seek(BifOffset, GEM_STREAM_START);
	unsigned long BifLen, ASCIIZOffset;
	unsigned short ASCIIZLen;
	for(unsigned int i = 0; i < BifCount; i++) {
		BIFEntry be;
		f->Seek(BifOffset + (12*i), GEM_STREAM_START);
		f->Read(&BifLen, 4);
		f->Read(&ASCIIZOffset, 4);
		f->Read(&ASCIIZLen, 2);
		f->Read(&be.BIFLocator, 2);
		be.name = (char*)malloc(ASCIIZLen);
		f->Seek(ASCIIZOffset, GEM_STREAM_START);
		f->Read(be.name, ASCIIZLen);
#ifndef WIN32
		for(int p = 0; p < ASCIIZLen; p++) {
		  if(be.name[p] == '\\')
		    be.name[p] = '/';
		}
#endif
		biffiles.push_back(be);
	}
	f->Seek(ResOffset, GEM_STREAM_START);
        resources.InitHashTable(ResCount);
	for(unsigned int i = 0; i < ResCount; i++) {
		RESEntry re;
		f->Read(re.ResRef, 8);
		f->Read(&re.Type, 2);
		f->Read(&re.ResLocator, 4);
		char *key;
		key=new char[8];
		for(int j=0;j<8;j++) key[j]=toupper(re.ResRef[j]);
		resources.SetAt(key, re.Type, re.ResLocator);
	}
	printf("Resources Loaded Succesfully.\n");
	delete(f);
	return true;
}

DataStream * KeyImp::GetResource(const char * resname, SClass_ID type)
{
	char path[_MAX_PATH], filename[_MAX_PATH] = {0};
	//Search it in the GemRB override Directory
	strcpy(path, core->GemRBPath);
	strcat(path, "override");
	strcat(path, SPathDelimiter);
	strcat(path, core->GameType);
	strcat(path, SPathDelimiter);
	strncpy(filename, resname, 8);
	filename[8]=0;
	strcat(filename, core->TypeExt(type));
	strlwr(filename);
	strcat(path, filename);
	FILE * exist = fopen(path, "rb");
	if(exist) {
		printf("[KEYImporter]: Found in GemRB Override...\n");
		fclose(exist);
		FileStream * fs = new FileStream();
		if(!fs)
			return NULL;
		fs->Open(path, true);
		return fs;
	}
	strcpy(path, core->GamePath);
	strcat(path, "override");
	strcat(path, SPathDelimiter);
	strncat(path, resname, 8);
	strcat(path, core->TypeExt(type));
	exist = fopen(path, "rb");
	if(exist) {
		printf("[KEYImporter]: Found in Override...\n");
		fclose(exist);
		FileStream * fs = new FileStream();
		if(!fs)
			return NULL;
		fs->Open(path, true);
		return fs;
	}
	printf("[KEYImporter]: Searching for %.8s%s...\n", resname, core->TypeExt(type));
	unsigned long ResLocator;
	if(resources.Lookup(resname,type,ResLocator) ) {
		if(!core->IsAvailable(IE_BIF_CLASS_ID)) {
			printf("[ERROR]\nAn Archive Plug-in is not Available\n");
			return NULL;
		}
		int bifnum = (ResLocator & 0xFFF00000) >> 20;
		ArchiveImporter * ai = (ArchiveImporter*)core->GetInterface(IE_BIF_CLASS_ID);
		FILE * exist = NULL;
		if(exist == NULL) {
			strcpy(path, core->GamePath);
			strcat(path, biffiles[bifnum].name);
			exist = fopen(path, "rb");
		}
		if(exist == NULL) {
			if((biffiles[bifnum].BIFLocator & (1<<2)) != 0) {
				strcpy(path, core->CD1);
			}
			else if((biffiles[bifnum].BIFLocator & (1<<3)) != 0) {
				strcpy(path, core->CD2);
			}
			else if((biffiles[bifnum].BIFLocator & (1<<4)) != 0) {
				strcpy(path, core->CD3);
			}
			else if((biffiles[bifnum].BIFLocator & (1<<5)) != 0) {
				strcpy(path, core->CD4);
			}
			else if((biffiles[bifnum].BIFLocator & (1<<6)) != 0) {
				strcpy(path, core->CD5);
			}
			else {
				printf("[KEYImporter]: Error in Bif Locator... Resource unavailable.\n");
				return NULL;
			}
			strcat(path, biffiles[bifnum].name);
			exist = fopen(path, "rb");
			if(exist == NULL) {
				printf("[KEYImporter]: Cannot find %s.", path);
				core->FreeInterface(ai);
				return NULL;
			}
		}
		else
			fclose(exist);
		ai->OpenArchive(path);
		DataStream * ret = ai->GetStream(ResLocator, type);
		if(ret == NULL)
			printf("[NOT_FOUND]\n");
		core->FreeInterface(ai);
		strncpy(ret->filename, resname,8);
		ret->filename[8]=0;
		strcat(ret->filename, core->TypeExt(type));
		return ret;
	}
	return NULL;
}
void * KeyImp::GetFactoryResource(const char * resname, SClass_ID type, unsigned char mode)
{
	if(type != IE_BAM_CLASS_ID) {
		printf("[KEYImporter]: %s files are not supported.\n", core->TypeExt(type));
		return NULL;
	}
	int fobjindex;
	if((fobjindex = core->GetFactory()->IsLoaded(resname, type)) != -1) {
		printf("[KEYImporter]: Factory Object Found!\n");
		return core->GetFactory()->GetFactoryObject(fobjindex);
	}
	printf("[KEYImporter]: No Factory Object Found, Loading...\n");
	char path[_MAX_PATH], filename[_MAX_PATH] = {0};
	//Search it in the GemRB override Directory
	strcpy(path, core->GemRBPath);
	strcat(path, "override");
	strcat(path, SPathDelimiter);
	strcat(path, core->GameType);
	strcat(path, SPathDelimiter);
	strncpy(filename, resname, 8);
	filename[8]=0;
	strcat(filename, core->TypeExt(type));
	strlwr(filename);
	strcat(path, filename);
	FILE * exist = fopen(path, "rb");
	if(exist) {
		printf("[KEYImporter]: Found in GemRB Override...\n");
		fclose(exist);
		FileStream * fs = new FileStream();
		if(!fs)
			return NULL;
		fs->Open(path, true);
		return fs;
	}
	strcpy(path, core->GamePath);
	strcat(path, "override");
	strcat(path, SPathDelimiter);
	strncat(path, resname, 8);
	strcat(path, core->TypeExt(type));
	exist = fopen(path, "rb");
	if(exist) {
		printf("[KEYImporter]: Found in Override...\n");
		fclose(exist);
		AnimationMgr * ani = (AnimationMgr*)core->GetInterface(IE_BAM_CLASS_ID);
		if(!ani)
			return NULL;
		FileStream * fs = new FileStream();
		if(!fs)
			return NULL;
		fs->Open(path, true);
		ani->Open(fs, true);
		AnimationFactory * af = ani->GetAnimationFactory(resname, mode);
		core->FreeInterface(ani);
		core->GetFactory()->AddFactoryObject(af);
		return af;
/*
		FileStream * fs = new FileStream();
		if(!fs)
			return NULL;
		fs->Open(path, true);
		return fs;
*/
	}
	printf("[KEYImporter]: Searching for %.8s%s...\n", resname, core->TypeExt(type));
	unsigned long ResLocator;
	if(resources.Lookup(resname, type, ResLocator) ) {
		int bifnum = (ResLocator & 0xFFF00000) >> 20;
		ArchiveImporter * ai = (ArchiveImporter*)core->GetInterface(IE_BIF_CLASS_ID);
		FILE * exist = NULL;
		if(exist == NULL) {
			strcpy(path, core->GamePath);
			strcat(path, biffiles[bifnum].name);
			exist = fopen(path, "rb");
		}
		if(exist == NULL) {
			if((biffiles[bifnum].BIFLocator & (1<<2)) != 0) {
				strcpy(path, core->CD1);
			}
			else if((biffiles[bifnum].BIFLocator & (1<<3)) != 0) {
				strcpy(path, core->CD2);
			}
			else if((biffiles[bifnum].BIFLocator & (1<<4)) != 0) {
				strcpy(path, core->CD3);
			}
			else if((biffiles[bifnum].BIFLocator & (1<<5)) != 0) {
				strcpy(path, core->CD4);
			}
			else if((biffiles[bifnum].BIFLocator & (1<<6)) != 0) {
				strcpy(path, core->CD5);
			}
			else {
				printf("[KEYImporter]: Error in Bif Locator... Resource unavailable.\n");
				return NULL;
			}
			strcat(path, biffiles[bifnum].name);
			exist = fopen(path, "rb");
			if(exist == NULL) {
				printf("[KEYImporter]: Cannot find %s.\n", biffiles[bifnum].name);
				core->FreeInterface(ai);
				return NULL;
			}
		}
		else
			fclose(exist);
		ai->OpenArchive(path);
		DataStream * ret = ai->GetStream(ResLocator, type);
		if(ret == NULL)
			printf("[NOT_FOUND]\n");
		core->FreeInterface(ai);
		strncpy(ret->filename, resname, 8);
		ret->filename[8] = 0;
		strcat(ret->filename, core->TypeExt(type));
		AnimationMgr * ani = (AnimationMgr*)core->GetInterface(IE_BAM_CLASS_ID);
		if(!ani)
			return NULL;
		ani->Open(ret, true);
		AnimationFactory * af = ani->GetAnimationFactory(resname, mode);
		core->FreeInterface(ani);
		core->GetFactory()->AddFactoryObject(af);
		return af;
	}
	return NULL;
}
