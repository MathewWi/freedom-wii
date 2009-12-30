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
#include <sys/dir.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <fat.h>
#include <malloc.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/errno.h>
#include <ogc/lwp_watchdog.h>

#include <sys/types.h>
#include <sys/errno.h>
#include <fcntl.h>


#include "certs_bin.h"
#include "devcerts_bin.h"
#include "su_tik.h"
#include "su_tmd.h"
#include "sys_tik.h"
#include "sys_tmd.h"

s32 ID_Sysmenu( void );
s32 ID_Root( void );
s32 ID_Title( u64 TitleID );
s32 ID_Channel( u64 TitleID );
