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

typedef struct
{
	void	*Pattern;
	u32		PatternLength;
	void	*Patch;
	u32		PatchLength;
} PatchInfo;

enum Patterns
{
	PATTERN_FWRITE,
	PATTERN_VIWII,
	PATTERN_KPADREAD,
	PATTERN_KPADREADOLD,
	PATTERN_PADREAD,
	
	PATTERN_COUNT
};

enum Patches
{
	PATCH_FWRITE,
	
	PATCH_COUNT
};

u32 FindPattern( void *Data, u32 Length, u32 Pattern );
u32 ApplyPatch( void *Data, u32 Length, u32 Pattern, u32 Patch );

