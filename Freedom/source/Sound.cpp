/*

Freedom - A system menu replacement for Nintendo Wii

Copyright (C) 2008-2009  crediar

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation version 2.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <sys/dir.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/errno.h>
#include <ogc/lwp_watchdog.h>

#include <sys/types.h>
#include <sys/errno.h>
#include <fcntl.h>

#include "Sound.h"

extern "C"
{
	#include "tremor\ivorbiscodec.h"
	#include "tremor\ivorbisfile.h"
	#include "misc.h"
}

//OGG Vorbis 

static OggVorbis_File vf;
extern u32 MusicStatus;

u32 off=0;
u32 BufferLength=0;
u32	ogg_file_size=0;
u32 Playing=0;


//Ogg Vorbis file wrappers
size_t callbacks_read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	return fread(ptr, size, nmemb, (FILE *)datasource);
}
int callbacks_seek_func(void *datasource, ogg_int64_t offset, int whence)
{
	return fseek((FILE *)datasource, offset, whence);
}
int callbacks_close_func(void *datasource)
{
	fclose((FILE*)datasource);
	
	return 0;
}
long callbacks_tell_func(void *datasource)
{
	return(ftell((FILE *)datasource));
}

u32 FileOffset=0;
size_t mRead(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	printf("callbacks_read_func(%u)\n",size*nmemb);

	if(FileOffset>ogg_file_size || FileOffset<0)
	{
		printf("Read(%X:%u,%X) failed\n",FileOffset,size*nmemb,ogg_file_size);
		return 0;
	}

	memcpy((u8*)ptr,(u8*)datasource+FileOffset,size*nmemb);
	FileOffset+=size*nmemb;
	return size*nmemb;
}

int mSeek(void *datasource, ogg_int64_t offset, int whence)
{
	printf("callbacks_seek_func("); 
	switch(whence)
	{
		case SEEK_CUR:
			FileOffset+=offset;
			printf("SEEK_CUR)\n");
			break;
		case SEEK_END:
			FileOffset=ogg_file_size+offset;
			printf("SEEK_END)\n");
			break;
		case SEEK_SET:
			FileOffset=offset;
			printf("SEEK_SET)\n");
			break;
		default:
			printf("Seek(%X:%X:%u) failed\n",FileOffset,offset,whence);
			while(1);
			return 1;
			break;
	}

	if(FileOffset>ogg_file_size || FileOffset<0)
	{
		printf("Seek(%X:%X:%u) failed\n",FileOffset,offset,whence);
		while(1);
		return 1;
	}

	return 0;
}

int mClose(void *datasource)
{	
	printf("callbacks_close_func\n");
	FileOffset=0;
	return 0;
}
long mTell(void *datasource)
{
	printf("callbacks_tell_func\n");
	return FileOffset;
}


ov_callbacks oggCallBacks = { callbacks_read_func, callbacks_seek_func, callbacks_close_func, callbacks_tell_func };
ov_callbacks mCallBacks = { mRead, mSeek, mClose, mTell };

void SoundInit( void )
{
	ASND_Init();
}
void SoundDeInit( void )
{
	ASND_End();
}
void SoundPause( void )
{
	ASND_Pause( 1 );
}
void SoundResume( void )
{
	ASND_Pause( 0 );
}
void SoundPauseResume( void )
{
	if( ASND_Is_Paused() )
	{
		ASND_Pause( 0 );
		gprintf("Sound:Resumed!\n");
	} else {
		ASND_Pause( 1 );
		gprintf("Sound:Stopped!\n");
	}
}
void SoundStop( void )
{
	ASND_StopVoice( 0 );
}
s32 SoundPlayBuffer( void *Buffer, u32 size )
{
	if( Buffer == NULL )
		return -1;

	ogg_file_size=size;

#ifdef OGG_DEBUG
	gprintf("Sound:SoundPlayFile->MagicBytes:%x\n", *(vu32*)Buffer );
#endif

	switch( *(vu32*)Buffer )
	{
		case 0x4F676753: // "OggS" OggVorbis
		{
#ifdef OGG_DEBUG
			gprintf("SoundPlayFile->OggVorbis\n");
#endif
			lwp_t *Thread=new lwp_t;
			LWP_CreateThread( Thread, SoundOggPlayBuf, (void*)Buffer, NULL, NULL, 10);
		} break;

		default:
			gprintf("Sound:SoundPlayFile->ERROR: Unknown MagicBytes:%x\n", *(vu32*)Buffer );
			return -3;
	}

	return 1;
}
s32 SoundPlayFile( FILE *f )
{
	if( f == NULL )
		return -1;

	u32 MagicBytes=0;
	fseek( f, 0, 0 );

	if( fread( &MagicBytes, sizeof(u32), 1, f ) != 1 )
		return -2;

#ifdef OGG_DEBUG
	gprintf("Sound:SoundPlayFile->MagicBytes:%x\n", MagicBytes );
#endif

	switch( MagicBytes )
	{
		case 0x4F676753: // "OggS" OggVorbis
		{
#ifdef OGG_DEBUG
			gprintf("SoundPlayFile->OggVorbis\n");
#endif
			lwp_t *Thread=new lwp_t;
			LWP_CreateThread( Thread, SoundOggPlay, (void*)f, NULL, NULL, 10);
		} break;

		default:
			return -3;
	}

	return 1;
}
void *SoundOggPlay( void *f )
{
	fseek( (FILE *)f, 0, SEEK_END );

	ogg_file_size = ftell( (FILE *)f );
	fseek( (FILE *)f, 0, 0 );

	s32 r = ov_open_callbacks( (FILE *)f, &vf, NULL, 0, oggCallBacks);
	if( r < 0 )
	{
#ifdef OGG_DEBUG
		gprintf("Sound:SoundOggPlay->ov_open_callbacks() failed: %d\n", r);
#endif
		ov_clear( &vf );
		fclose( (FILE *)f );
		MusicStatus = -6;
		return NULL;
	}

	vorbis_info *vi=NULL;
	if( (vi=ov_info( &vf, -1 )) == NULL )
	{
#ifdef OGG_DEBUG
		gprintf("Sound:SoundOggPlay->ov_info() failed!:%d\n", vi );
#endif
		ov_clear( &vf );
		fclose( (FILE *)f );
		MusicStatus = -5;
		return NULL;
	}

#ifdef OGG_DEBUG
	gprintf("Sound:SoundOggPlay->Len:%d\n", (((long)ov_pcm_total(&vf,-1))*2) );
	gprintf("Sound:SoundOggPlay->Rate:%d\n", vi->rate );
#endif

	u8 *SoundBuffer=NULL;
	u32 BufferSize = 1024*1024;

	BufferSize = vi->rate * 4 * 2;
	SoundBuffer = (u8*)memalign( 32, BufferSize*2 );

#ifdef OGG_DEBUG
	gprintf("Sound:SoundOggPlay->BufferSize:%d\n", BufferSize );
#endif

	if( ASND_Is_Paused() );
		ASND_Pause(0);

	u32 flip=1;

	Playing=1;
	SoundOggReadSample( SoundBuffer, BufferSize );
	ASND_SetVoice( 0, VOICE_STEREO_16BIT, vi->rate, 0, SoundBuffer, BufferSize, 255, 255, 0 );
	SoundOggReadSample( SoundBuffer+BufferSize, BufferSize );

	MusicStatus = 3;

	while(1)
	{
		if( flip )
		{
			while(ASND_StatusVoice(0)==SND_WORKING);

			if( !Playing )
				break;

			ASND_SetVoice( 1, VOICE_STEREO_16BIT, vi->rate, 0, SoundBuffer+BufferSize, BufferSize, 255, 255, 0 );

			if( !SoundOggReadSample( SoundBuffer, BufferSize ))
				break;

		} else {

			while(ASND_StatusVoice(1)==SND_WORKING);

			if( !Playing )
				break;

			ASND_SetVoice( 0, VOICE_STEREO_16BIT, vi->rate, 0, SoundBuffer, BufferSize, 255, 255, 0 );

			if( !SoundOggReadSample( SoundBuffer+BufferSize, BufferSize ))
				break;

		}
		flip = !flip;
	}

	free(SoundBuffer);
	ov_clear(&vf);
	fclose( (FILE *)f );
	ASND_StopVoice(0);
	ASND_StopVoice(1);
#ifdef OGG_DEBUG
	gprintf("Sound:SoundOggPlay->Thread end!\n");
#endif
	return NULL;
}
void *SoundOggPlayBuf( void *Buffer )
{
	s32 r = ov_open_callbacks( Buffer, &vf, NULL, ogg_file_size, mCallBacks);
	if( r < 0 )
	{
#ifdef OGG_DEBUG
		gprintf("Sound:SoundOggPlay->ov_open_callbacks() failed:%d\n", r );
#endif
		ov_clear( &vf );
		free(Buffer);
		MusicStatus = -6;
		return NULL;
	}

	vorbis_info *vi=NULL;
	if( (vi=ov_info( &vf, -1 )) == NULL )
	{
#ifdef OGG_DEBUG
		gprintf("Sound:SoundOggPlay->ov_info() failed!:%d\n", vi );
#endif
		ov_clear( &vf );
		free(Buffer);
		MusicStatus = -5;
		return NULL;
	}

#ifdef OGG_DEBUG
	gprintf("Sound:SoundOggPlay->Len:%d\n", (((long)ov_pcm_total(&vf,-1))*2) );
	gprintf("Sound:SoundOggPlay->Rate:%d\n", vi->rate );
#endif

	u8 *SoundBuffer=NULL;
	u32 BufferSize = 1024*1024;

	BufferSize = vi->rate * 4 * 2;
	SoundBuffer = (u8*)memalign( 32, BufferSize*2 );

#ifdef OGG_DEBUG
	gprintf("Sound:SoundOggPlay->BufferSize:%d\n", BufferSize );
#endif

	if( ASND_Is_Paused() );
		ASND_Pause(0);

	u32 flip=1;

	Playing=1;
	SoundOggReadSample( SoundBuffer, BufferSize );
	ASND_SetVoice( 0, VOICE_STEREO_16BIT, vi->rate, 0, SoundBuffer, BufferSize, 255, 255, 0 );
	SoundOggReadSample( SoundBuffer+BufferSize, BufferSize );

	MusicStatus = 3;

	while(1)
	{
		if( flip )
		{
			while(ASND_StatusVoice(0)==SND_WORKING);

			if( !Playing )
				break;

			ASND_SetVoice( 1, VOICE_STEREO_16BIT, vi->rate, 0, SoundBuffer+BufferSize, BufferSize, 255, 255, 0 );

			if( !SoundOggReadSample( SoundBuffer, BufferSize ))
				break;

		} else {

			while(ASND_StatusVoice(1)==SND_WORKING);

			if( !Playing )
				break;

			ASND_SetVoice( 0, VOICE_STEREO_16BIT, vi->rate, 0, SoundBuffer, BufferSize, 255, 255, 0 );

			if( !SoundOggReadSample( SoundBuffer+BufferSize, BufferSize ))
				break;

		}
		flip = !flip;
	}

	free(SoundBuffer);
	ov_clear(&vf);
	free(Buffer);
	ASND_StopVoice(0);
	ASND_StopVoice(1);
#ifdef OGG_DEBUG
	gprintf("Sound:SoundOggPlay->Thread end!\n");
#endif
	return NULL;
}
s32 SoundOggReadSample( u8 *buffer, u32 BufferSize )
{
	int t_section=0;
	s32 ret=0;
	u32 size=0;

	/*
		since ov_read doesn't always return the desired size,
		we have to make a loop till we have the correct amount
	*/
	do
	{
		ret = ov_read( &vf, (char*)( buffer + size ), BufferSize-size, &t_section );

		//iprintf("r:%d s:%d f:%d:%d\n", ret, size, fileoff, BufferSize );

		if( ret == 0 )//EOF
		{	
#ifdef OGG_DEBUG
			gprintf("Sound:SoundOggReadSample->EOF\n");
#endif
			MusicStatus = -7;
			break;

		} else if ( ret > 0 ) {
			size+=ret;
		} else {
#ifdef OGG_DEBUG
			gprintf("Sound:SoundOggReadSample->ERROR:%d\n",ret);
#endif
			MusicStatus = -8;
			break;
		}

		//if( fileoff >= ogg_file_size )
		//	ASND_StopVoice(0);

		if( size >= BufferSize )
		{			
#ifdef OGG_DEBUG
			gprintf("Sound:SoundOggReadSample->sample done:%d DSP Usage%d%%\n",ASND_GetTime(),ASND_GetDSP_PercentUse());
#endif
			return 1;
		}


	} while(ret);

	//sleep(5);
	ov_clear(&vf);
	MusicStatus = 2;
	return 0;
}

char *SoundFileGetTagInfo( FILE *f, u32 Tag )
{
	if( f == NULL )
		return NULL;

	u32 MagicBytes=0;
	fseek( f, 0, 0 );

	if( fread( &MagicBytes, sizeof(u32), 1, f ) != 1 )
		return NULL;

#ifdef OGG_DEBUG
	gprintf("Sound:SoundFileGetTagInfo->MagicBytes:%X\n", MagicBytes );
#endif

	switch( MagicBytes )
	{
		case 0x4F676753: // "OggS" OggVorbis
		{
#ifdef OGG_DEBUG
			gprintf("Sound:SoundFileGetTagInfo->OggVorbis\n");
#endif
			
			return SoundGetOggTag( f, Tag );

		} break;

		default:
			return NULL;
	}

	return NULL;
}
char *SoundGetOggTag( FILE *f, u32 Tag )
{
	OggVorbis_File vf;
	vorbis_comment *comment = NULL;
	vorbis_info *vi = NULL;

	fseek( (FILE *)f, 0, 0 );

	if( (vi=ov_info( &vf, -1 )) == NULL )
	{
#ifdef OGG_DEBUG
		gprintf("Sound:SoundGetOggTag->ov_info() failed!\n");
#endif
		ov_clear( &vf );
		fclose( (FILE *)f );
		MusicStatus = -5;
		return NULL;
	}

	if( ov_open_callbacks( (FILE *)f, &vf, NULL, 0, oggCallBacks) < 0 )
	{ 
#ifdef OGG_DEBUG
		gprintf("Sound:SoundGetOggTag->ov_open_callbacks() failed!\n");
#endif
		ov_clear( &vf );
		fclose(f); 
		return NULL; 
	} 

	comment = ov_comment( &vf, -1);
	if( comment == NULL )
	{
#ifdef OGG_DEBUG
		gprintf("Sound:SoundGetOggTag->File has no comments!\n");
#endif
		ov_clear(&vf);
		fclose( f );
		return NULL;
	}

	char *s=NULL;

	switch( Tag )
	{
		case ARTIST:
			s = vorbis_comment_query(comment, "artist", 0);
			break;
		case TITLE:
			s = vorbis_comment_query(comment, "title", 0);
			break;
		case ALBUM:
			s = vorbis_comment_query(comment, "album", 0);
			break;
		case TRACKNUMBER:
			s = vorbis_comment_query(comment, "tracknumber", 0);
			break;
		case GERNE:
			s = vorbis_comment_query(comment, "genre", 0);
			break;
		default:
			ov_clear(&vf);
			fclose( f );
			return NULL;
			break;
	}

	if( s == NULL )
	{
		ov_clear(&vf);
		fclose( f );
		return NULL;
	}

	char *str = strdup(s);

	ov_clear(&vf);
	fclose( f );

	return str;
}
