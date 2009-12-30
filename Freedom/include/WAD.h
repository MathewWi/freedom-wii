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
#include <gctypes.h>


struct WADHeader {
	unsigned int HeaderSize;
	unsigned int WadType;
	unsigned int CertChainSize;
	unsigned int Reserved;
	unsigned int TicketSize;
	unsigned int TMDSize;
	unsigned int DataSize;
	unsigned int FooterSize;
};

struct ContentHeader {
	u64		TitleID;
	u32		IconSize;
	u8		HeaderMD5[16];
	u8		IconMD5[16];
	u32		unknown;
	u64		TitleID2;
	u64		TitleID3;
};

u32 WADImport( char *name );
u32 WADImportContent( char *name );
u32 WADExportContent( u64 TitleID );
u32 WADTest( void );
