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
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include "DI.h"
#include "misc.h"

int di_fd=0;

u8 *inbuf;
u8 *outbuf;

int DVDInit( void )
{
	if( di_fd )
		return di_fd;

	di_fd = IOS_Open("/dev/di",0);

	inbuf = (u8*)memalign( 32, 32 );
	outbuf = (u8*)memalign( 32, 32 );

	return di_fd;
}
int DVDGetHandle( void )
{
	return di_fd;
}
int DVDLowGetCoverStatus( u32 *status )
{
	memset(inbuf, 0, 0x20 );
	memset(outbuf, 0, 0x20 );

	((u32*)inbuf)[0x00] = 0x88<<24;

	DCFlushRange(inbuf, 0x20);
	int ret = IOS_Ioctl( di_fd, 0x88, inbuf, 0x20, outbuf, 0x20);
	if( ret >= 0 )
		memcpy( status, (void*)outbuf, sizeof(u32) );

	return ret;
}
int DVDLowRequestError( u32 *error )
{
	memset(inbuf, 0, 0x20 );
	memset(outbuf, 0, 0x20 );

	((u32*)inbuf)[0x00] = 0xE0<<24;

	DCFlushRange(inbuf, 0x20);
	int ret = IOS_Ioctl( di_fd, 0xE0, inbuf, 0x20, outbuf, 0x20);
	if( ret >= 0 )
		memcpy( error, (void*)outbuf, sizeof(u32) );

	return ret;
}
int DVDLowRead( void *dst, u32 size, u32 offset)
{
	memset(inbuf, 0, 0x20 );

	((u32*)inbuf)[0x00] = 0x71<<24;
	((u32*)inbuf)[0x01] = size;
	((u32*)inbuf)[0x02] = offset;

	DCFlushRange(inbuf, 0x20);
	return IOS_Ioctl( di_fd, 0x71, inbuf, 0x20, (u8*)dst, size);
}
int DVDLowUnencryptedRead( void *dst, u32 size, u32 offset)
{
	memset(inbuf, 0, 0x20 );

	((u32*)inbuf)[0x00] = 0x8D<<24;
	((u32*)inbuf)[0x01] = size;
	((u32*)inbuf)[0x02] = offset;

	DCFlushRange(inbuf, 0x20);
	return IOS_Ioctl( di_fd, 0x8D, inbuf, 0x20, (u8*)dst, size);
}
int DVDLowReset( void )
{
	memset(inbuf, 0, 0x20 );

	((u32*)inbuf)[0x00] = 0x8A<<24;
	((u32*)inbuf)[0x01] = 1;

	DCFlushRange(inbuf, 0x20);
	return IOS_Ioctl( di_fd, 0x8A, inbuf, 0x20, 0, 0);
}
int DVDLowStopMotor( void )
{
	memset(inbuf, 0, 0x20 );
	((u32*)inbuf)[0x00] = 0xE3<<24;
	((u32*)inbuf)[0x01] = 0;	// optional, unknown
	((u32*)inbuf)[0x02] = 0;	// optional, unknown

	DCFlushRange(inbuf, 0x20);
	return IOS_Ioctl( di_fd, 0xE3, inbuf, 0x20, outbuf, 0x20);	//does not really return something (outbuf)
}
int DVDLowOpenPartition( u32 PartitionOffset, u8 *tmd )
{
	u8 *buffer = (u8*)memalign( 32, 0x100 );
	memset( buffer, 0, 0x100 );

	memset( tmd, 0, 0x49E4 );

	((u32*)buffer)[(0x40>>2)]	=  0x8B<<24;
	((u32*)buffer)[(0x40>>2)+1]	=  PartitionOffset>>2;

	//in
	((u32*)buffer)[0x00] = (u32)PHYSADDR((buffer+0x40));	//0x00
	((u32*)buffer)[0x01] = 0x20;					//0x04
	((u32*)buffer)[0x02] = 0;						//0x08
	((u32*)buffer)[0x03] = 0x2A4;					//0x0C 
	((u32*)buffer)[0x04] = 0;						//0x10
	((u32*)buffer)[0x05] = 0;						//0x14

	//out
	((u32*)buffer)[0x06] = PHYSADDR(tmd);			//0x18
	((u32*)buffer)[0x07] = 0x49E4;					//0x1C
	((u32*)buffer)[0x08] = PHYSADDR(buffer+0x60);	//0x20
	((u32*)buffer)[0x09] = 0x20;					//0x24

	DCFlushRange(buffer, 0x100);
	DCFlushRange(tmd, 0x49E4);

	s32 r = IOS_Ioctlv( di_fd, 0x8B, 3, 2, buffer);

	DCFlushRange(tmd, 0x49E4);

	free(buffer);

	return r;

}
int DVDLowClosePartition( void )
{
	memset(inbuf, 0, 0x20 );
	((u32*)inbuf)[0x00] = 0x8C<<24;
	return IOS_Ioctl( di_fd, 0x8C, inbuf, 0x20, 0, 0);
}
int DVDLowReadDiskID( void *dst )
{
	memset(inbuf, 0, 0x20 );
	memset((u8*)dst, 0, 0x20 );

	((u32*)inbuf)[0x00] = 0x70<<24;

	DCFlushRange(inbuf, 0x20);
	return IOS_Ioctl( di_fd, 0x70, inbuf, 0x20, (u8*)dst, 0x20);
}
int DVDClose( void )
{
	if( !di_fd )
		return -1;

	free( inbuf );
	free( outbuf );

	return IOS_Close( di_fd );
}
