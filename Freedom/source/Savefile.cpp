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
#include <wiiuse/wpad.h>
#include <sys/dir.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#include <vector>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/errno.h>
#include <ogc/lwp_watchdog.h>

#include <sys/types.h>
#include <sys/errno.h>
#include <fcntl.h>

#include "SaveFile.h"
#include "Identify.h"


extern "C"
{
	#include "misc.h"
}

s32	SaveFile::GetStatus ( void )
{
	return Status;
}
s32	SaveFile::Update ( void )
{
	return update;
}

	SaveFile::SaveFile( void )
{
	Status=0;
	update=0;

	s32 r = ES_GetNumTitles( &TitleCount );
	if(r<0)
	{
		Status=-4;
		update=1;
		return;
	}

	TitlesIDs = (u64*)memalign( 32, sizeof(u64)*TitleCount );
	memset( TitlesIDs, 0, sizeof(u64)*TitleCount );

	r = ES_GetTitles( TitlesIDs, TitleCount );
	if(r<0)
	{
		Status=-4;
		update=1;
		return;
	}

	std::vector<u64> t;

	//Pre sort list
	for(u32 i=0; i < TitleCount; ++i)
	{	
		if( ( (TitlesIDs[i]>>32) != 0x00010000 ) && ( (TitlesIDs[i]>>32) != 0x00010004 ) && ( (TitlesIDs[i]>>32) != 0x00010001 ) )
			continue;

		t.resize( t.size() + 1);
		t[t.size()-1] = TitlesIDs[i];
	}

	free(TitlesIDs);

	TitleCount = t.size();

	TitlesIDs = (u64*)memalign( 32, sizeof(u64)*TitleCount );
	memset( TitlesIDs, 0, sizeof(u64)*TitleCount );

	for(u32 i=0; i < TitleCount; ++i)
	{	
		TitlesIDs[i] = t[i];
	}

	t.resize(0);
	Status=1;
	update=1;
}

char *SaveFile::GetName( u32 ID )
{
	if( ID > TitleCount )
		return NULL;

	s32 r = ID_Title( TitlesIDs[ID] );
	if( r < 0 )
		return strdup("IdentFail");

	char *filepath=(char*)memalign( 32, 1024 );
	if( filepath == NULL )
		return strdup("OUT OF MEMORY");

	r = ES_GetDataDir( TitlesIDs[ID], filepath );
	if( r < 0 )
		return strdup("ES_GetDataDir Fail");

	char *file=(char*)memalign( 32, 1024 );
	if( file == NULL )
		return strdup("OUT OF MEMORY");

	sprintf( file, "%s/banner.bin", filepath );

	free( filepath );

	u32 b = (u32)(TitlesIDs[ID]&0xFFFFFFFF);

	s32 fd = ISFS_Open( (const char*)file, 1 );

	free(file);

	if( fd < 0 )
	{
		update=1;
		char *s = (char*)memalign(32,32);
		sprintf( s, "%c%c%c%c", b>>24, (b>>16)&0xFF, (b>>8)&0xFF, b&0xFF);
		return s;
	}

	fstats *status = (fstats *)memalign(32, sizeof( fstats ) );
	if( status == NULL )
		return strdup("OUT OF MEMORY");
	if( status == NULL )
	{
		ISFS_Close( fd );
		free(status);
		char *s = (char*)memalign(32,32);
		sprintf( s, "%c%c%c%c", b>>24, (b>>16)&0xFF, (b>>8)&0xFF, b&0xFF);
		return s;
	}

	if( ISFS_GetFileStats( fd, status) < 0 )
	{
		ISFS_Close( fd );
		free(status);
		char *s = (char*)memalign(32,32);
		sprintf( s, "%c%c%c%c", b>>24, (b>>16)&0xFF, (b>>8)&0xFF, b&0xFF);
		return s;
	}

	u8 *buf = (u8 *)memalign(32, (status->file_length+31)&(~31) );
	if( buf == NULL )
		return strdup("OUT OF MEMORY");
	memset( buf, 0, (status->file_length+31)&(~31) );
	
	ISFS_Seek( fd, 0, SEEK_SET );
	if(ISFS_Read( fd, buf, status->file_length )!= status->file_length)
	{
		ISFS_Close( fd );
		free(buf);
		free(status);
		char *s = (char*)memalign(32,32);
		sprintf( s, "%c%c%c%c", b>>24, (b>>16)&0xFF, (b>>8)&0xFF, b&0xFF);
		return s;
	}
	
	char *TitleName = (char*)memalign(32, 0x28);
	if( TitleName == NULL )
		return strdup("OUT OF MEMORY");
	memset( TitleName, 0, 0x28 );

	for(int y=0; y < 0x28; ++y )
		TitleName[y]=buf[0x20+(y*2)+1];

	ISFS_Close( fd );
	free(buf);
	free(status);
	return TitleName;
}
s32 SaveFile::Backup( u32 ID )
{
	if( ID > TitleCount )
		return -1;

	s32 r = ID_Title( TitlesIDs[ID] );
	if( r < 0 )
		return -2;

	char *filepath=(char*)memalign( 32, 1024 );
	if( filepath == NULL )
		return -3;

	r = ES_GetDataDir( TitlesIDs[ID], filepath );
	if( r < 0 )
	{
		free(filepath);
		return -4;
	}

	//Create folder on FAT device

	u32 b = (u32)(TitlesIDs[ID]&0xFFFFFFFF);
	char *pre = (char*)memalign(32,32);
	sprintf( pre, "/%c%c%c%c", b>>24, (b>>16)&0xFF, (b>>8)&0xFF, b&0xFF);

	mkdir( pre, 664 );

	//Get

	//static void *xfb = NULL;
	//static GXRModeObj *rmode = NULL;

	//VIDEO_Init();

	//rmode = VIDEO_GetPreferredMode(NULL);

	//xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	//
	//console_init( xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight, rmode->fbWidth*VI_DISPLAY_PIX_SZ );

	//VIDEO_Configure(rmode);
	//VIDEO_SetNextFramebuffer(xfb);
	//VIDEO_SetBlack(FALSE);
	//VIDEO_Flush();

	//VIDEO_WaitVSync();
	//if(rmode->viTVMode&VI_NON_INTERLACE)
	//	VIDEO_WaitVSync();

	DumpDir( pre, filepath );

	free(filepath);

	//sleep(10);
	//exit(0);

	return 1;

}
void SaveFile::DumpDir( char *Dst, char *Src )
{
	char *SrcDir = (char*)memalign( 32, 256 );
	memset( SrcDir, 0, 256 );

	char *DstDir = (char*)memalign( 32, 256 );
	memset( DstDir, 0, 256 );

	sprintf( SrcDir, "%s", Src );
	gprintf("SrcDir:%s\n", SrcDir );

	sprintf( DstDir, "%s", Dst );
	gprintf("DstDir:%s\n", DstDir );

	u32 num=0;
	s32 ret = ISFS_ReadDir( SrcDir, NULL, &num);
	if(ret<0)
	{
		//if( ret != -102)
		//{
		//	printf("ISFS_ReadDir(\"%s\") failed:%d\n",SrcDir,ret);
		//	sleep(5);
		//}
	} else  {
		//printf("Files:%d\n", num );
		if( num > 0 )
		{
			char *FileList = (char*)memalign( 32, num*0xD );
			memset( FileList, 0, num*0xD );

			ISFS_ReadDir( SrcDir, FileList, &num);

			//hack replaces all \0 with ' ', then we can easily grab the names with strtok 
			int i=0;
			while(i<num*0xD)
				if(FileList[i++] == '\0' )
					FileList[i-1]=' ';

			char *token,
				 *nexttoken;

			token = strtok_r( FileList, " ", &nexttoken );
			//printf("\t%s\n", token);

			char *filepath = (char*)memalign( 32, 512 );
			memset( filepath, 0, 256 );

			char *Dstpath = (char*)memalign( 32, 512 );
			memset( Dstpath, 0, 256 );

			if( Src[0] == '/' && Src[1] == '\0' )
				sprintf(filepath,"%s%s", Src, token );
			else
				sprintf(filepath,"%s/%s", Src, token );

			sprintf(Dstpath,"%s/%s", Dst, token );

			//easy hack to check if dir or file
			s32 r = IOS_Open( filepath, 1 );
			if( r < 0 )	//is FOLDER
			{
				mkdir( Dstpath, 664 );
				DumpDir( Dstpath, filepath );
			} else {
			
				u8 *buf = (u8*)memalign( 32, 2048 );
				memset( buf, 0, 2048 );

				fstats *status = (fstats*)memalign( 32, sizeof( fstats ) );
				ISFS_GetFileStats( r, status);
				gprintf("\"%s\"->\"%s\" size:%d\n", filepath, Dstpath, status->file_length );
					
				unsigned int size = status->file_length;
				unsigned int readsize = 2048;

				if( status->file_length < 2048 )
					readsize = status->file_length;

				FILE *o = fopen( Dstpath, "wb" );

				if( o == NULL )
				{
					gprintf("Could  not create:\"%s\"\n", Dstpath );
				} else {

					while( size > 0 )
					{
						ISFS_Read( r, buf, readsize );
						fwrite( buf, sizeof( char ), readsize, o );

						size -= readsize;
						if( size < readsize )
							readsize = size;
						
					}
					fflush( o );
					fclose( o );
				}
				free( buf );
				free( status );
				ISFS_Close( r );
			}


			num--;
			if(num>0)
			while( (token = strtok_r( NULL, " ", &nexttoken )))
			{
				//printf("\t%s\n", token);

				if( Src[0] == '/' && Src[1] == '\0' )
					sprintf(filepath,"%s%s", Src, token );
				else
					sprintf(filepath,"%s/%s", Src, token );

				sprintf( Dstpath,"%s/%s", Dst, token );

				s32 r = IOS_Open( filepath, 1 );
				if( r < 0 )	//is FOLDER
				{
					mkdir( Dstpath, 664 );
					DumpDir( Dstpath, filepath );
				} else {
				
					u8 *buf = (u8*)memalign( 32, 2048 );
					memset( buf, 0, 2048 );

					fstats *status = (fstats*)memalign( 32, sizeof( fstats ) );
					ISFS_GetFileStats( r, status);
					gprintf("\"%s\"->\"%s\" size:%d\n", filepath, Dstpath, status->file_length );
						
					unsigned int size = status->file_length;
					unsigned int readsize = 2048;

					if( status->file_length < 2048 )
						readsize = status->file_length;

					FILE *o = fopen( Dstpath, "wb" );

					if( o == NULL )
					{
						gprintf("Could  not create:\"%s\"\n", Dstpath);
					} else {

						while( size > 0 )
						{
							ISFS_Read( r, buf, readsize );
							fwrite( buf, sizeof( char ), readsize, o );

							size -= readsize;
							if( size < readsize )
								readsize = size;
							
						}
						fflush( o );
						fclose( o );
					}
					free( buf );
					free( status );
					ISFS_Close( r );
				}


				num--;
				if( num <= 0 )
					break;
			}

			free( Dstpath );
			free( filepath );
			free( FileList );
		}
	}

	free( SrcDir );
	free( DstDir );

}
u32 SaveFile::GetCount( void )
{
	return TitleCount;
}
	SaveFile::~SaveFile( )
{
	;
}
