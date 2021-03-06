/*
 * file info.h - display info at level start
 *
 * $Id: info.h,v 1.6 2004/12/04 06:01:13 lodott Exp $
 *
 * Program XBLAST
 * (C) by Oliver Vogel (e-mail: m.vogel@ndh.net)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2; or (at your option)
 * any later version
 *
 * This program is distributed in the hope that it will be entertaining,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILTY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef _INFO_H
#define _INFO_H

#include "xblast.h"
#include "ini_file.h"

/*
 * prototypes
 */
extern void ResetInfo (void);
extern XBBool ParseLevelInfo (const DBSection *section, DBSection *warn);
extern void ClearInfo (void);
extern void AddPlayerInfo (const char *fmt, ...);
extern void AddLevelInfo (const char *fmt, ...);
extern void AddExtraInfo (const char *fmt, ...);
extern const char **GetPlayerInfo (int *pNum);
extern const char **GetLevelInfo (int *pNum);
extern const char **GetExtraInfo (int *pNum);
extern void SetMaxVictories(int nvictories);
extern unsigned GetMaxVictories();
extern unsigned GetGameModeInfo();
#endif
/*
 * end of file info.h
 */
