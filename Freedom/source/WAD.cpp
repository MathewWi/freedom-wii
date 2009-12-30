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
#include <wiisprite.h>
#include <fat.h>
#include <sys/dir.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#include <vector>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <network.h>
#include <sys/errno.h>
#include <ogc/lwp_watchdog.h>

#include <sys/types.h>
#include <sys/errno.h>
#include <fcntl.h>

#include "Music.h"
#include "Sound.h"
#include "Identify.h"
#include "SaveFile.h"
#include "FPad.h"
#include "WAD.h"

extern "C"
{
	#include "misc.h"
	#include "rijndael.h"
}


u8 SD_IV[16] =
{
    0x21, 0x67, 0x12, 0xE6,
	0xAA, 0x1F, 0x68, 0x9F,
	0x95, 0xC5, 0xA2, 0x23,
	0x24, 0xDC, 0x6A, 0x98, 
};


u32 ImportProgress=0;

u32 WADImportProgress( void )
{
	return ImportProgress;
}
int get_title_key(signed_blob *s_tik, u8 *key)
{
  static u8 iv[16] ATTRIBUTE_ALIGN(0x20);
  static u8 keyin[16] ATTRIBUTE_ALIGN(0x20);
  static u8 keyout[16] ATTRIBUTE_ALIGN(0x20);
  int retval;

  const tik *p_tik;
  p_tik = (tik*)SIGNATURE_PAYLOAD(s_tik);
  u8 *enc_key = (u8 *)&p_tik->cipher_title_key;
  memcpy(keyin, enc_key, sizeof keyin);
  memset(keyout, 0, sizeof keyout);
  memset(iv, 0, sizeof iv);
  memcpy(iv, &p_tik->titleid, sizeof p_tik->titleid);
  
  retval = ES_Decrypt(ES_KEY_COMMON, iv, keyin, sizeof keyin, keyout);
  if (retval) gprintf("ES_Decrypt returned %d\n", retval);
  memcpy(key, keyout, sizeof keyout);
  return retval;
}
u32 WADTest( void )
{
	FILE *f = fopen("sd:/private/wii/title/HCDJ/content.bin","rb");
	if( f == NULL )
		return 0;

	fseek( f, 0, SEEK_END );
	u32 size = ftell( f );
	fseek( f, 0, 0);
	u8 *buf = (u8*)memalign( 32, size );
	ContentHeader *Cheader = (ContentHeader*)memalign( 32, sizeof(ContentHeader) );

	fread( buf, 1, sizeof(ContentHeader), f );
	fclose( f );

	u8 *IV=(u8*)memalign(32,16); 

	memcpy( IV, SD_IV, 16 );

	s32 r = ES_Decrypt( ES_KEY_SDCARD, IV, buf, sizeof(ContentHeader), (u8*)Cheader);
	gprintf("ES_Decrypt:%d\n", r );
	if( r >= 0 )
	{
		gprintf("TitleID :%016llX\n", Cheader->TitleID );
		gprintf("IconSize:%08X\n", Cheader->IconSize );

		u32 BkOffset = (0x640+0x3F)&(~0x3F);
		BkOffset = (BkOffset+Cheader->IconSize+0x3F)&(~0x3F);
		gprintf("Bk Offset:%08X\n", BkOffset );
	}

	return 0;
}
u32 WADExportContent( u64 TitleID )
{
	char file[1024]={0};

	sprintf( file, "/wad/%c%c%c%c.wad", (u8)((TitleID&0xFF000000)>>24),
										(u8)((TitleID&0x00FF0000)>>16),
										(u8)((TitleID&0x0000FF00)>>8),
										(u8)((TitleID&0x000000FF))
										);
	gprintf("WADExportContent:Exporting to \"%s\"\n", file );

	FILE *w = fopen( file, "wb" ) ;
	if( w == NULL )
	{
		gprintf("WADExportContent:Could not create \"%s\"!\n", file ); 
		return -1;
	}

	WADHeader wad;
	memset( &wad, 0, sizeof( WADHeader ) );

	wad.HeaderSize=0x20;
	wad.WadType=0x49730000;

	wad.CertChainSize=0;
	wad.TicketSize=0;

//Get TMD
	s32 r=ES_GetStoredTMDSize( TitleID, &(wad.TMDSize) );
#ifdef WAD_DEBUG
	gprintf("WADExportContent->ES_GetStoredTMDSize():%d\n", r );
#endif
	if( r < 0 )
	{
#ifndef WAD_DEBUG
		gprintf("WADExportContent->ES_GetStoredTMDSize():%d\n", r );
#endif
		return -1;
	}

	signed_blob *TMD = (signed_blob *)memalign( 32, (wad.TMDSize+31)&(~31) );
	if( TMD == NULL )
	{
		return -2;
	}
	memset(TMD, 0, wad.TMDSize);

	r=ES_GetStoredTMD( TitleID, TMD, wad.TMDSize);
#ifdef WAD_DEBUG
	gprintf("WADExportContent->ES_GetStoredTMD():%d\n", r );
#endif
	if(r<0)
	{
#ifndef WAD_DEBUG
		gprintf("WADExportContent->ES_GetStoredTMD():%d\n", r );
#endif
		free( TMD );
		return -3;
	}

	tmd *rTMD = (tmd *)(TMD+(0x140/sizeof(tmd *)));

//Get Ticket for titleKey

	//read ticket from FS
	char *TicketFile=(char*)memalign(32, 1024);
	sprintf( TicketFile, "/ticket/%08x/%08x.tik", (u32)(TitleID>>32), (u32)(TitleID&0xFFFFFFFF) );
	s32 fd = ISFS_Open( TicketFile, 1 );
#ifdef WAD_DEBUG
	gprintf("WADExportContent->ISFS_Open(%s):%d\n", TicketFile, r );
#endif
	if( fd < 0 )
	{
#ifndef WAD_DEBUG
		gprintf("WADExportContent->ISFS_Open(%s):%d\n", TicketFile, r );
#endif
		free( TMD );
		return fd;
	}

	//get size
	fstats * tstatus = (fstats*)memalign( 32, sizeof( fstats ) );
	r = ISFS_GetFileStats( fd, tstatus );

#ifdef WAD_DEBUG
	gprintf("WADExportContent->ISFS_GetFileStats():%d\n", r );
#endif

	if( r < 0 )
	{
#ifndef WAD_DEBUG
		gprintf("WADExportContent->ISFS_GetFileStats():%d\n", r );
#endif
		ISFS_Close( fd );
		return -1;
	}

	//create buffer
	char * TicketData = (char*)memalign( 32, (tstatus->file_length+31)&(~31) );
	if( TicketData == NULL )
	{
		ISFS_Close( fd );
		return -1;
	}
	memset(TicketData, 0, (tstatus->file_length+31)&(~31) );

	//read file
	r = ISFS_Read( fd, TicketData, tstatus->file_length );

#ifdef WAD_DEBUG
	gprintf("WADExportContent->ISFS_Read():%d\n", r );
#endif

	if( r < 0 )
	{
#ifndef WAD_DEBUG
		gprintf("WADExportContent->ISFS_Read():%d\n", r );
#endif
		free( TicketData );
		ISFS_Close( fd );
		return r;
	}

	ISFS_Close( fd );
	free( TicketFile );
//Forge WAD

//Add cert chain
	wad.CertChainSize=0xA00;
	fseek( w, 0x40, SEEK_SET );
	gprintf("WADExportContent:adding cert at 0x%08X", ftell(w) ); 
	gprintf(" ... 0x%08X written\n", fwrite( certs_bin, sizeof( char ), wad.CertChainSize, w ) ); 

//Add Ticket
	fseek( w, (ftell(w)+0x3F)&(~0x3F), SEEK_SET );
	gprintf("WADExportContent:adding Ticket at 0x%08X", ftell(w) ); 
	wad.TicketSize = 0x2A4;
	gprintf(" ... 0x%08X written\n", fwrite( TicketData, sizeof( char ), wad.TicketSize, w ) ); 

//Add TMD
	wad.TMDSize = 0x1E4 + 0x24 * rTMD->num_contents;

	fseek( w, (ftell(w)+0x3F)&(~0x3F), SEEK_SET );
	gprintf("WADExportContent:adding TMD at 0x%08X", ftell(w) ); 
	gprintf(" ... 0x%08X written\n", fwrite( TMD, sizeof( char ), wad.TMDSize, w ) ); 

	printf("WADExportContent:Content count:%d\n", rTMD->num_contents );

	wad.DataSize=0;

	u8* buf = (u8*)memalign( 32, 0x8000);
	memset( buf, 0, 0x8000 );
	u8* ebuf = (u8*)memalign( 32, 0x8000 );
	memset( ebuf, 0, 0x8000 );

//Create key for encryption

	u8 key[16];
	get_title_key((signed_blob*)TicketData, key);
	aes_set_key(key);

	for( u32 i=0; i < rTMD->num_contents; ++i)
	{
		fseek( w, (ftell(w)+0x3F)&(~0x3F), SEEK_SET );
		u32 size = ((u32*)TMD)[(0x1F0+0x24*i)>>2];

		if( (rTMD->contents[i].type & 0x8000) == 0x8000 )
		{
			gprintf("WADExportContent:adding %08X.app(S) size:%d at 0x%08X\n", rTMD->contents[i].cid, size, ftell(w) );

			//Open content map
			//build path and read file
			char *FilePath = (char *)memalign( 32, 512 );
			sprintf( FilePath, "/shared1/content.map" );

			s32 ContentMapHandle = ISFS_Open( FilePath, 1 );
			if( ContentMapHandle < 0 )
			{
				gprintf("Failed to open \"%s\"!(%d)\n", FilePath, ContentMapHandle );
				free( TMD );
				return -1;
			}
			
			fstats *FileStatus = (fstats *)memalign( 32, sizeof(fstats) );
			r = ISFS_GetFileStats( ContentMapHandle, FileStatus);
			if( ContentMapHandle < 0 )
			{
				gprintf("Failed to get file status!(%d)\n", r );
				free( TMD );
				return -1;
			}
			//create buffer; read file

			u8* fbuf = (u8*)memalign( 32, (FileStatus->file_length+31)&(~31) );
			memset( fbuf, 0, (FileStatus->file_length+31)&(~31) );

			r = ISFS_Read( ContentMapHandle, fbuf, FileStatus->file_length );
			if( ContentMapHandle < 0 || r != FileStatus->file_length )
			{
				gprintf("Failed to read file!(%d)\n", r );
				free( TMD );
				free( fbuf );
				return -1;
			}
			ISFS_Close( ContentMapHandle );
			
			//Search for file
			for( u32 j=8; j<FileStatus->file_length; j+=0x1C )
			{
				if( memcmp( fbuf+j, rTMD->contents[i].hash, 0x14) == 0 )
				{
					sprintf( FilePath, "/shared1/%.8s.app",(fbuf+j)-8 );
					//gprintf("Found:\"%s\"\n", FilePath);

					s32 fd = ISFS_Open( FilePath, 1 );
					if( fd < 0 )
					{
						gprintf("Failed to open \"%s\"!(%d)\n", FilePath, fd );
						free( fbuf );
						return -1;
					}

					fstats *FileStatus = (fstats *)memalign( 32, sizeof(fstats) );
					s32 r = ISFS_GetFileStats( fd, FileStatus);
					if( fd < 0 )
					{
						gprintf("Failed to get file status!(%d)\n", r );
						free( fbuf );
						return -1;
					}

					//read file, encrypt and write
					s32 Size = FileStatus->file_length;
					u32 ReadSize = 0x8000;

					if( Size < ReadSize )
						ReadSize = Size;

					set_encrypt_iv( rTMD->contents[i].index );

					while( Size > 0 )
					{
						memset( buf, 0, 0x8000);
						r = ISFS_Read( fd, buf, ReadSize );
						if( r < 0 || r != ReadSize )
						{
							gprintf("Failed to read file!\nReadSize:%d Read:%d FileSize:%d\n", ReadSize, r, FileStatus->file_length );
							return -1;
						}

						memset( ebuf, 0, 0x8000 );
						encrypt_buffer( buf, ebuf, ReadSize );

						//add padding
						if( ReadSize&0x1F )
							ReadSize = (ReadSize+0x1F)&(~0x1F);

						wad.DataSize += fwrite( ebuf, sizeof( char ), ReadSize, w );
						Size -= ReadSize;

						if( Size < ReadSize )
							ReadSize = Size;

					}

					ISFS_Close( fd );

					free( FileStatus );

					break;
				}
			}
			free( fbuf );
			free(FilePath);

		} else {
			gprintf("WADExportContent:adding %08X.app size:%d at 0x%X\n", rTMD->contents[i].cid, size, ftell(w) );

			char *FilePath = (char *)memalign( 32, 512 );
			sprintf( FilePath, "/title/%08x/%08x/content/%08x.app", (u32)(TitleID>>32), (u32)(TitleID&0xFFFFFFFF), rTMD->contents[i].cid );

			s32 fd = ISFS_Open( FilePath, 1 );
			if( fd < 0 )
			{
				gprintf("WADExportContent:Failed to open \"%s\"!(%d)\n", FilePath, fd );
				return -1;
			}

			fstats *FileStatus = (fstats *)memalign( 32, sizeof(fstats) );
			s32 r = ISFS_GetFileStats( fd, FileStatus);
			if( fd < 0 )
			{
				gprintf("WADExportContent:Failed to get file status!(%d)\n", r );
				return -1;
			}

			//read file, encrypt and write
			s32 Size = FileStatus->file_length;
			u32 ReadSize = 0x8000;

			if( Size < ReadSize )
				ReadSize = Size;

			set_encrypt_iv( rTMD->contents[i].index );

			while( Size > 0 )
			{
				r = ISFS_Read( fd, buf, ReadSize );
				if( r < 0 || r != ReadSize )
				{
					gprintf("WADExportContent:Failed to read file!\nReadSize:%d Read:%d FileSize:%d\n", ReadSize, r, FileStatus->file_length );
					return -1;
				}

				memset( ebuf, 0, 0x8000 );
				encrypt_buffer( buf, ebuf, ReadSize );

				//add padding
				if( ReadSize&0x1F )
					ReadSize = (ReadSize+0x1F)&(~0x1F);

				wad.DataSize += fwrite( ebuf, sizeof( char ), ReadSize, w );
				Size -= ReadSize;

				if( Size < ReadSize )
					ReadSize = Size;
			}

			ISFS_Close( fd );

			free( FilePath );
			free( FileStatus );
		}

	}

	free( ebuf );
	free( buf );

	fseek( w, 0, 0 );

	fwrite( &wad, sizeof( WADHeader ), 1, w );

	fclose( w );
	gprintf("WADExportContent:Done\n");

	return 1;
}
u32 WADImportContent( char *name )
{
	FILE *wad = fopen( name, "rb" ) ;
	if( wad == NULL )
	{
		gprintf("Import:Could not open \"%s\" for reading!\n", name ); 
		return -1;
	}

	WADHeader wh;
	u32 ImportedBytes=0;
	ImportProgress=0;
	fread( &wh, sizeof( WADHeader ), 1, wad );

	u8 *cert=NULL;
	u8 *tik=NULL;

	if( wh.CertChainSize )
	{
		fseek( wad, (wh.HeaderSize+0x3F)&(~0x3F), 0 );
		gprintf("\tCert off:%08X Size:%X\n", ftell(wad), wh.CertChainSize );
		cert = (u8 *)memalign( 32, (wh.CertChainSize+31)&(~31) );

		if( fread( cert, sizeof( u8 ), wh.CertChainSize, wad ) != wh.CertChainSize )
		{
			gprintf("Import:Error while reading the cert\n");
			free(cert);
			return -2;
		}
	}

	if( wh.TicketSize )
	{
		fseek( wad, (ftell(wad)+0x3F)&(~0x3F), 0 );
		gprintf("\tTIK  off:%08X Size:%X\n", ftell(wad), wh.TicketSize );
		tik = (u8*)memalign( 32, (wh.TicketSize+31)&(~31) );

		if( fread( tik, sizeof( u8 ), wh.TicketSize, wad ) != wh.TicketSize )
		{
			gprintf("Import:Error while reading the ticket\n");
			if( tik != NULL );
				free(tik);
			if( cert != NULL );
				free(cert);
			return -2;
		}
	}

	fseek( wad, (ftell(wad)+0x3F)&(~0x3F), 0 );
	gprintf("\tTMD  off:%08X Size:%X\n\n", ftell(wad), wh.TMDSize );
	u8 *tmdbuf = (u8 *)memalign( 32, (wh.TMDSize+31)&(~31) );

	if( fread( tmdbuf, sizeof( u8 ), wh.TMDSize, wad ) != wh.TMDSize )
	{
		gprintf("Error while reading the TMD\n");
		free(tmdbuf);
		if( tik != NULL );
			free(tik);
		if( cert != NULL );
			free(cert);
		return -2;
	}

	s32 r=0;

	r = ES_AddTitleStart((signed_blob*)tmdbuf, wh.TMDSize, (signed_blob*)cert, wh.CertChainSize, NULL, 0);
	gprintf("Import:ES_AddTitleStart():%d\n", r);

	if( r < 0 )
	{
		switch(r)
		{
			case -1035:
				gprintf("Import:Title with a higher version is already installed!\n");
			break;
			case -1028:
				gprintf("Import:No ticket installed\n");
			break;
			case -1029:
				gprintf("Import:Installed ticket is invalid!\n");
			break;
			case -2011:
				gprintf("Import:Signature check failed\n");
			break;
		}

		free(tmdbuf);
		if( tik != NULL );
			free(tik);
		if( cert != NULL );
			free(cert);
		return -3;
	}

	u32 ImportStart = time(0);
	for( int i=0; i < ((u16*)tmdbuf)[0x1DE>>1]; ++i)
	{
		fseek( wad, (ftell(wad)+0x3F)&(~0x3F), SEEK_SET );
		gprintf("Import:adding %08X.app size:%d from 0x%08X\n", ((u32*)tmdbuf)[(0x1E4+0x24*i)>>2], ((u32*)tmdbuf)[(0x1F0+0x24*i)>>2], ftell(wad) );

		//if( (rTMD->contents[i].type & 0x8000) == 0x8000 )
		//{
		//	fseek( wad, (((u32)((u32*)tmdbuf)[(0x1F0+0x24*i)>>2])+31)&(~31), SEEK_CUR);
		//} else {

			s32 fd = ES_AddContentStart( ((u64*)(tmdbuf+0x18C))[0], ((u32*)tmdbuf)[(0x1E4+0x24*i)>>2] );
			gprintf("Import:ES_AddContentStart(%x):%d\n",( (u32*)tmdbuf)[(0x1E4+0x24*i)>>2], r );
			if( fd < 0 )
			{
				gprintf("Import:ES_AddTitleCancel():%d\n", ES_AddTitleCancel());
				free(tmdbuf);
				if( tik != NULL );
					free(tik);
				if( cert != NULL );
					free(cert);
				return -3;
			}

			unsigned int ContentSize = (((u32)((u32*)tmdbuf)[(0x1F0+0x24*i)>>2])+31)&(~31);

			if( ContentSize > 5*1024*1024 )
			{
				u32 ReadSize=5*1024*1024;
				u8 *buf = (u8*)memalign( 32, ReadSize );

				while( ContentSize > 0 )
				{
					//memset( buf, 0, ReadSize );
					if( fread( buf, sizeof( u8 ), ReadSize, wad) != ReadSize )
					{
						gprintf("Import:fread() did not return right amount of read size\n" );
						free(tmdbuf);
						if( tik != NULL );
							free(tik);
						if( cert != NULL );
							free(cert);
						free(buf);
						return -3;
					}

					gprintf("Import:ES_AddContentData(0x%X, 0x%08X, %d):", ((u32*)tmdbuf)[(0x1E4+0x24*i)>>2], buf, ReadSize );
					r = ES_AddContentData( fd, buf, ReadSize );
					gprintf("%d\n", r);
					if( r < 0 )
					{
						gprintf("Import:ES_AddTitleCancel():%d\n", ES_AddTitleCancel());
						free(buf);
						free(tmdbuf);
						if( tik != NULL );
							free(tik);
						if( cert != NULL );
							free(cert);
						return -3;
					}

					ImportedBytes += ReadSize;
					ImportProgress = (ImportedBytes*100)/wh.DataSize;

					u32 TimePast = time(0) - ImportStart;
					u32 speed = ImportedBytes / TimePast;

					gprintf("Progress:%d%% Timleft: %d:%d\n", ImportProgress, (((wh.DataSize-ImportedBytes)/speed)/60)%60, (((wh.DataSize-ImportedBytes)/speed)%60)%60 );

					ContentSize -= ReadSize;

					if( ContentSize < ReadSize )
						ReadSize = ContentSize;
				}

				free(buf);

			} else {

				u8 *buf = (u8*)memalign( 32, ContentSize );
				fread( buf, sizeof( u8 ), ContentSize, wad);

				r = ES_AddContentData( fd, buf, ContentSize );
				gprintf("Import:ES_AddContentData(0x%X, 0x%08X, %d):%d\n", ((u32*)tmdbuf)[(0x1E4+0x24*i)>>2], buf, ContentSize, r);
				if( r < 0 )
				{
					gprintf("Import:ES_AddTitleCancel():%d\n", ES_AddTitleCancel());
					free(tmdbuf);
					if( tik != NULL );
						free(tik);
					if( cert != NULL );
						free(cert);
					return -3;
				}

				ImportedBytes += ContentSize;
				ImportProgress = (ImportedBytes*100)/wh.DataSize;

				u32 TimePast = time(0) - ImportStart;
				u32 speed = ImportedBytes / TimePast;

				gprintf("Progress:%d%% Timleft: %d:%d\n", ImportProgress, (((wh.DataSize-ImportedBytes)/speed)/60)%60, (((wh.DataSize-ImportedBytes)/speed)%60)%60 );


				free(buf);

			}

			r = ES_AddContentFinish(fd);
			gprintf("Import:ES_AddContentFinish(%d):%d\n", fd, r);
			if( r < 0 )
			{
				switch(r)
				{
					case -1022:
						gprintf("Import:Content does not match HASH in TMD\n");
					break;
				}

				gprintf("Import:ES_AddTitleCancel():%d\n", ES_AddTitleCancel());
				free(tmdbuf);
				if( tik != NULL );
					free(tik);
				if( cert != NULL );
					free(cert);
				return -3;
			}
		//}

	}	

	gprintf("Import:ES_AddTitleFinish():%d\n", ES_AddTitleFinish());
	
	fclose(wad);
	free(tmdbuf);
	if( tik != NULL );
		free(tik);
	if( cert != NULL );
		free(cert);

	return 1;	
}
u32 WADImport( char *name )
{
	//prefix /wad
	char Path[1024];
	sprintf( Path, "sd:/wad/%s.wad", name );

	FILE *wad = fopen( Path, "rb" ) ;
	if( wad == NULL )
	{
		gprintf("Import:Could not open \"%s\" for reading!\n", name ); 
		return -1;
	}

	WADHeader wh;
	fread( &wh, sizeof( WADHeader ), 1, wad );

	if( wh.TicketSize != 0x2A4 )
	{
		gprintf("Import:Warning TicketSize is not 0x2A4\n");
	}

	if( wh.CertChainSize != 0xA00 )
	{
		gprintf("Import:Warning CertChainSize is not 0xA00\n");
	}

	fseek( wad, (wh.HeaderSize+0x3F)&(~0x3F), 0 );
	gprintf("\tCert off:%X Size:%X\n", ftell(wad), wh.CertChainSize );
	u8 *cert = (u8 *)memalign( 32, (wh.CertChainSize+31)&(~31) );

	if( fread( cert, sizeof( u8 ), wh.CertChainSize, wad ) != wh.CertChainSize )
	{
		gprintf("Import:Error while reading the cert\n");
		free(cert);
		return -2;
	}

	wh.CertChainSize = 0xA00;

	fseek( wad, (ftell(wad)+0x3F)&(~0x3F), 0 );
	gprintf("\tTIK  off:%X Size:%X\n", ftell(wad), wh.TicketSize );
	u8 *tik = (u8*)memalign( 32, (wh.TicketSize+31)&(~31) );

	if( fread( tik, sizeof( u8 ), wh.TicketSize, wad ) != wh.TicketSize )
	{
		gprintf("Import:Error while reading the ticket\n");
		free(tik);
		free(cert);
		return -2;
	}

	fseek( wad, (ftell(wad)+0x3F)&(~0x3F), 0 );
	gprintf("\tTMD  off:%X Size:%X\n\n", ftell(wad), wh.TMDSize );
	u8 *tmdbuf = (u8 *)memalign( 32, (wh.TMDSize+31)&(~31) );

	if( fread( tmdbuf, sizeof( u8 ), wh.TMDSize, wad ) != wh.TMDSize )
	{
		gprintf("Error while reading the TMD\n");
		free(tmdbuf);
		free(tik);
		free(cert);
		return -2;
	}

	u64 TitleID = *(u64*)(tmdbuf+0x18C);
	s32 r=0;
	u32 count=0;

	if( (u32)(TitleID>>32) == 0x00010001 )
	{
		r = ES_GetNumTicketViews( TitleID, &count );
		gprintf("Import:ES_GetNumTicketViews():%d\n", r);
		if( r < 0 )
		{
			free(tmdbuf);
			free(tik);
			free(cert);
			return -3;
		}
		if( count == 0 )
		{
			gprintf("Import:Ticket not installed\n");
		}

	} 
	if( count == 0 || (u32)(TitleID>>32) != 0x00010001 )
	{
		r = ES_AddTicket( (signed_blob*)tik, wh.TicketSize, (signed_blob*)cert, wh.CertChainSize, NULL, 0);
		gprintf("Import:ES_AddTicket():%d\n", r);
		if( r < 0 )
		{
			switch(r)
			{
				case -4100:
					if( wh.CertChainSize == 0xC40 )
					{
						gprintf("Import:The CertChain size implies that this is a WAD for a devkit!\n");
					} else {
						gprintf("Import:Wrong Ticket-, Cert size or invalid Ticket-, Cert data.\n");
					}
				break;
				case -2011:
					gprintf("Import:Signature check failed\n");
				break;
			}


			free(tmdbuf);
			free(tik);
			free(cert);
			return -3;
		}
	}

	r = ES_AddTitleStart((signed_blob*)tmdbuf, wh.TMDSize, (signed_blob*)cert, wh.CertChainSize, NULL, 0);
	gprintf("Import:ES_AddTitleStart():%d\n", r);

	if( r < 0 )
	{
		switch(r)
		{
			case -1035:
				gprintf("Import:Title with a higher version is already installed!\n");
			break;
			case -1036:
				gprintf("Import:Required system version is not installed!\n");
			break;
			case -1028:
				gprintf("Import:No ticket installed\n");
			break;
			case -1029:
				gprintf("Import:Installed ticket is invalid!\n");
			break;
			case -2011:
				gprintf("Import:Signature check failed\n");
			break;
		}

		free(tmdbuf);
		free(tik);
		free(cert);
		return -3;
	}

	for( int i=0; i < ((u16*)tmdbuf)[0x1DE>>1]; ++i)
	{
		fseek( wad, (ftell(wad)+0x3F)&(~0x3F), SEEK_SET );
		gprintf("Import:adding %08X.app size:%d from %08X\n", ((u32*)tmdbuf)[(0x1E4+0x24*i)>>2], ((u32*)tmdbuf)[(0x1F0+0x24*i)>>2], ftell(wad) );

		s32 fd = ES_AddContentStart( ((u64*)(tmdbuf+0x18C))[0], ((u32*)tmdbuf)[(0x1E4+0x24*i)>>2] );
		gprintf("Import:ES_AddContentStart(%x):%d\n",( (u32*)tmdbuf)[(0x1E4+0x24*i)>>2], r );
		if( fd < 0 )
		{
			gprintf("Import:ES_AddTitleCancel():%d\n", ES_AddTitleCancel());
			free(tmdbuf);
			free(tik);
			free(cert);
			return -3;
		}

		unsigned int ContentSize = (((u32)((u32*)tmdbuf)[(0x1F0+0x24*i)>>2])+31)&(~31);

		if( ContentSize > 5*1024*1024 )
		{
			u32 ReadSize=5*1024*1024;
			u8 *buf = (u8*)memalign( 32, ReadSize );

			while( ContentSize > 0 )
			{
				s32 r = fread( buf, sizeof( u8 ), ReadSize, wad);
				if( r != ReadSize )
				{
					gprintf("Import:fread() did not return right amount of read size! (%d != %d)\n", r, ReadSize );
					//r = fread( buf+r, sizeof( u8 ), ReadSize-r, wad);
					//gprintf("Import:fread() did not return right amount of read size! (%d != %d)\n", r, ReadSize );
					//gprintf("Import:error:%d\n", ferror( wad ) );
					//gprintf("Import:Position:0x%08X\n", ftell( wad ) );
					//fseek( wad, 0, SEEK_END );
					//gprintf("Import:Size:0x%08X\n", ftell( wad ) );

					gprintf("Import:ES_AddTitleCancel():%d\n", ES_AddTitleCancel());
					free(tmdbuf);
					free(tik);
					free(cert);
					free(buf);
					return -3;
				}

				gprintf("Import:ES_AddContentData(%X, %X, %X):", ((u32*)tmdbuf)[(0x1E4+0x24*i)>>2], buf, ReadSize );
				r = ES_AddContentData( fd, buf, ReadSize );
				gprintf("%d\n", r);
				if( r < 0 )
				{
					gprintf("Import:ES_AddTitleCancel():%d\n", ES_AddTitleCancel());
					free(buf);
					free(tmdbuf);
					free(tik);
					free(cert);
					return -3;
				}

				ContentSize -= ReadSize;

				if( ContentSize < ReadSize )
					ReadSize = ContentSize;
			}

			free(buf);

		} else {

			u8 *buf = (u8*)memalign( 32, ContentSize );
			fread( buf, sizeof( u8 ), ContentSize, wad);

			r = ES_AddContentData( fd, buf, ContentSize );
			gprintf("Import:ES_AddContentData(%X, %X, %X):%d\n", ((u32*)tmdbuf)[(0x1E4+0x24*i)>>2], buf, ContentSize, r);
			if( r < 0 )
			{
				gprintf("Import:ES_AddTitleCancel():%d\n", ES_AddTitleCancel());
				free(tmdbuf);
				free(tik);
				free(cert);
				return -3;
			}

			free(buf);

		}

		r = ES_AddContentFinish(fd);
		gprintf("Import:ES_AddContentFinish(%d):%d\n", fd, r);
		if( r < 0 )
		{
			switch(r)
			{
				case -1022:
					gprintf("Import:Content does not match HASH in TMD\n");
				break;
			}

			gprintf("Import:ES_AddTitleCancel():%d\n", ES_AddTitleCancel());
			free(tmdbuf);
			free(tik);
			free(cert);
			return -3;
		}

	}	

	gprintf("Import:ES_AddTitleFinish():%d\n", ES_AddTitleFinish());
	
	fclose(wad);
	free(tmdbuf);
	free(tik);
	free(cert);

	return 1;	
}

