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
#include <string.h>


#define TYPE_RETURN 3
#define TYPE_NANDBOOT 4
#define TYPE_SHUTDOWNSYSTEM 5
#define RETURN_TO_MENU 0
#define RETURN_TO_SETTINGS 1
#define RETURN_TO_ARGS 2

typedef struct {
	u32 checksum;
	u8 flags;
	u8 type;
	u8 discstate;
	u8 returnto;
	u32 unknown[6];
} __attribute__((packed)) StateFlags;

s32 CheckBootState( void );
s32 ClearState( void );
