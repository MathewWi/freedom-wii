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
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sdcard/wiisd_io.h>
#include <fat.h>

#include "misc.h"

u32 GeckoFound=0;

u32 SDCard_Init()
{
    __io_wiisd.startup();
	s32 r = fatMountSimple ( "sd", &__io_wiisd );
	return r;
}

void patch_b(u32 *source, u32 *dst)
{
	int diff = (dst - source) << 2;
	diff &= 0x03FFFFFC;
	*source = diff | 0x48000000;
}
void CheckForGecko( void )
{
	GeckoFound = usb_isgeckoalive( 1 );
}
void gprintf( const char *str, ... )
{
	if(!GeckoFound)
		return;

#ifndef GECKO_DEBUG
	return;
#else

	char astr[4096];

	va_list ap;
	va_start(ap,str);

	vsprintf( astr, str, ap );

	va_end(ap);

	usb_sendbuffer_safe( 1, astr, strlen(astr) );

#endif
}
static char ascii(char s)
{
  if(s < 0x20) return '.';
  if(s > 0x7E) return '.';
  return s;
}

void hexdump(void *d, int len)
{
  u8 *data;
  int i, off;
  data = (u8*)d;
  for (off=0; off<len; off += 16) {
    gprintf("%08x  ",off);
    for(i=0; i<16; i++)
      if((i+off)>=len) gprintf("   ");
      else gprintf("%02x ",data[off+i]);

    gprintf(" ");
    for(i=0; i<16; i++)
      if((i+off)>=len) gprintf(" ");
      else gprintf("%c",ascii(data[off+i]));
    gprintf("\n");
  }
}
