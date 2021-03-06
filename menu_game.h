/*
 * file menu_game.h - menus for setting up game parameters
 *
 * $Id: menu_game.h,v 1.3 2004/05/14 10:00:35 alfie Exp $
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
#ifndef _MENU_GAME_H
#define _MENU_GAME_H

#include "xblast.h"
#include "cfg_game.h"

/*
 * prototypes
 */
extern XBBool CreateLocalGameMenu (void *par);
extern XBBool CreateStartNetGameMenu (void *par);
extern XBBool CreateJoinNetGameMenu (void *par);

extern void setAutoCentral(XBBool set); // XBCC
extern XBBool CreateCentralGameMenu (void *par); // XBCC
extern XBBool CreateCentralJoinMenu (void *par); // XBCC

extern XBPlayerHost GetHostType (void);
extern void SetHostType (XBPlayerHost);

#endif
/*
 * end of file menu_game.h
 */
