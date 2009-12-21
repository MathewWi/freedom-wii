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

#define VERSION	0x0001

//#define PARSE_DEBUG
//#define OGG_DEBUG
//#define CHANNEL_DEBUG
//#define CONFIG_DEBUG
#define GECKO_DEBUG


#define PHYSADDR(x) ((unsigned long *)(0x7FFFFFFF & ((unsigned long)(x))))
#define VIRTADDR(x) ((unsigned long *)(0x80000000 | ((unsigned long)(x))))

#define KB_SIZE		1024.0
#define MB_SIZE		1048576.0
#define GB_SIZE		1073741824.0

/* Macros */
#define round_up(x,n)	(-(-(x) & -(n)))

u32 SDCard_Init();
void gprintf( const char *str, ... );
void patch_b(u32 *source, u32 *dst);
void CheckForGecko( void );
void hexdump(void *d, int len);
