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

#include "NAND.h"


extern "C"
{
	#include "misc.h"
}

FSStats *NANDGetStats( void )
{
	u8 *mem = (u8*)memalign( 32, 0x140 );
	memset( mem, 0, 0x140 );
	s32 r = ISFS_GetStats(mem);
	if( r < 0 )
	{
		gprintf("NAND:GetFreeBlocks->ISFS_GetStats():%d\n", r);
		return NULL;
	}
		
	//for( u32 i=0; i<0x20; i+=4 )
	//	gprintf("%08X\t\t%d\t\t%d\n", *(u32*)(mem+i), *(u32*)(mem+i), *(u32*)(mem+i)*FSBLOCK_SIZE/1024/1024 );

	FSStats *stats = new FSStats;
	memcpy( stats, mem, 0x1C );

	free( mem );

	return stats;
}
