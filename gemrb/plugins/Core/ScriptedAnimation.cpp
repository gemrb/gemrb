#include "../../includes/win32def.h"
#include "ScriptedAnimation.h"
#include "AnimationMgr.h"
#include "Interface.h"

extern Interface* core;

ScriptedAnimation::ScriptedAnimation(DataStream* stream, bool autoFree,
	long X, long Y)
{
	anims[0] = NULL;
	anims[1] = NULL;
	Transparency = 0;
	SequenceFlags = 0;
	XPos = YPos = ZPos = 0;
	FrameRate = 0;
	FaceTarget = 0;
	Sounds[0][0] = 0;
	Sounds[1][0] = 0;
	if (!stream) {
		return;
	}
	char Signature[8];
	stream->Read( Signature, 8 );
	if (strnicmp( Signature, "VVC V1.0", 8 ) != 0) {
		printf( "Not a valid VVC File\n" );
		if (autoFree)
			delete( stream );
		return;
	}
	char Anim1ResRef[9], Anim2ResRef[9];
	unsigned long seq1, seq2;
	stream->Read( Anim1ResRef, 8 );
	Anim1ResRef[8] = 0;
	stream->Read( Anim2ResRef, 8 );
	Anim2ResRef[8] = 0;
	stream->Read( &Transparency, 4 );
	stream->Seek( 4, GEM_CURRENT_POS );
	stream->Read( &SequenceFlags, 4 );
	stream->Seek( 4, GEM_CURRENT_POS );
	stream->Read( &XPos, 4 );
	stream->Read( &YPos, 4 );
	stream->Seek( 4, GEM_CURRENT_POS );
	stream->Read( &FrameRate, 4 );
	stream->Read( &FaceTarget, 4 );
	stream->Seek( 16, GEM_CURRENT_POS );
	stream->Read( &ZPos, 4 );
	stream->Seek( 24, GEM_CURRENT_POS );
	stream->Read( &seq1, 4 );
	stream->Read( &seq2, 4 );
	stream->Seek( 8, GEM_CURRENT_POS );
	stream->Read( Sounds[0], 8 );
	Sounds[0][8] = 0;
	stream->Read( Sounds[1], 8 );
	Sounds[1][8] = 0;
	AnimationMgr* aM = ( AnimationMgr* ) core->GetInterface( IE_BAM_CLASS_ID );
	DataStream* dS = core->GetResourceMgr()->GetResource( Anim1ResRef, IE_BAM_CLASS_ID );
	aM->Open( dS, true );
	anims[0] = aM->GetAnimation( ( unsigned char ) seq1, 0, 0 );
	anims[1] = aM->GetAnimation( ( unsigned char ) seq2, 0, 0 );
	//anims[0] = af->GetCycle((unsigned char)seq1);
	//anims[1] = af->GetCycle((unsigned char)seq2);
	XPos += X;
	YPos += Y;
	if (anims[0]) {
		anims[0]->autoSwitchOnEnd = true;
		//anims[1]->autoSwitchOnEnd = true;
		anims[0]->pos = 0;
		//anims[1]->pos = 0;
	}
	justCreated = true;
	if (autoFree) {
		delete( stream );
	}
}

ScriptedAnimation::~ScriptedAnimation(void)
{
	if (anims[0]) {
		delete( anims[0] );
	}
	if (anims[1]) {
		delete( anims[1] );
	}
}
