#include "../../includes/win32def.h"
#include "ScriptedAnimation.h"
#include "AnimationMgr.h"
#include "Interface.h"

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
	ieDword seq1, seq2;
	stream->Read( Anim1ResRef, 8 );
	Anim1ResRef[8] = 0;
	stream->Read( Anim2ResRef, 8 );
	Anim2ResRef[8] = 0;
	stream->ReadDword( &Transparency );
	stream->Seek( 4, GEM_CURRENT_POS );
	stream->ReadDword( &SequenceFlags );
	stream->Seek( 4, GEM_CURRENT_POS );
	stream->ReadDword( &XPos );
	stream->ReadDword( &YPos );
	stream->Seek( 4, GEM_CURRENT_POS );
	stream->ReadDword( &FrameRate );
	stream->ReadDword( &FaceTarget );
	stream->Seek( 16, GEM_CURRENT_POS );
	stream->ReadDword( &ZPos );
	stream->Seek( 24, GEM_CURRENT_POS );
	stream->ReadDword( &seq1 );
	stream->ReadDword( &seq2 );
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
	core->FreeInterface( aM );
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
