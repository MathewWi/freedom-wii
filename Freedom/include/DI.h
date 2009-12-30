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
#include "misc.h"

enum DiscTypes { DISC_RVL, DISC_DOL };

#define DVD_COVER_OPEN	1
#define DVD_COVER_CLOSE	2

#define IOCTL_DI_READID		0x70
#define IOCTL_DI_READ		0x71
#define IOCTL_DI_WAITCVRCLOSE	0x79
#define IOCTL_DI_GETCOVER	0x88
#define IOCTL_DI_RESET		0x8A
#define IOCTL_DI_OPENPART	0x8B
#define IOCTL_DI_CLOSEPART	0x8C
#define IOCTL_DI_UNENCREAD	0x8D
#define IOCTL_DI_SEEK		0xAB
#define IOCTL_DI_STOPLASER	0xD2
#define IOCTL_DI_OFFSET		0xD9
#define IOCTL_DI_STOPMOTOR	0xE3


int DVDInit(void);
int DVDGetHandle( void );
int DVDLowGetCoverStatus( u32 *status );
int DVDLowRequestError( u32 *error );
int DVDLowRead( void *dst, u32 size, u32 offset);
int DVDLowUnencryptedRead( void *dst, u32 size, u32 offset);
int DVDLowReset( void );
int DVDLowStopMotor( void );
int DVDLowOpenPartition( u32 PartitionOffset, u8 *tmd );
int DVDLowClosePartition( void );
int DVDLowReadDiskID( void *dst );
int DVDClose( void );
