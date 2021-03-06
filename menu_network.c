/*
 * file menu_network.c - user interface for setting up networks games
 *
 * $Id: menu_network.c,v 1.40 2005/01/23 14:23:43 lodott Exp $
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
#include "menu_network.h"

#include "atom.h"
#include "menu.h"
#include "menu_game.h"
#include "menu_layout.h"
#include "mi_tool.h"
#include "server.h"
#include "central.h" /* XBCC */
#include "random.h" /* XBCC */
#include "str_util.h"
#include "chat.h"
#ifndef W32
#include <sys/time.h>
#endif

/*
 * shared data for all menus
 */

/*
 * local constants
 */
#define ANIME_LENGTH 4

/*
 * local variables
 */

/* server configuration */
static char        serverName[128];
static int         serverPort;
static char        serverGame[48];
static CFGGameHost cfgServer;
static CFGGameHost cfgServerhis[10];

/* host data */
static char        *hostName[MAX_HOSTS];
static unsigned     hostComplete[MAX_HOSTS];
static MENU_ID      hostItem[MAX_HOSTS];
/* player data */
static const char       *playerName[MAX_HOSTS][NUM_LOCAL_PLAYER];
static XBBool            playerValid[MAX_HOSTS][NUM_LOCAL_PLAYER];
static CFGPlayerGraphics playerGfx[MAX_HOSTS][NUM_LOCAL_PLAYER];
static const CFGPlayerGraphics *pPlayerGfx[NUM_LOCAL_PLAYER];
static BMSpriteAnimation        playerAnime[ANIME_LENGTH] = {
    SpriteStopDown, SpriteWinner3,  SpriteWinner2, SpriteWinner,
};
/* team data */
static int teamMode = 0;
static MENU_ID      teamItem[MAX_HOSTS][NUM_LOCAL_PLAYER];
/* network games */
static const XBNetworkGame *networkGame[NUM_SEARCH_ROWS];
static XBNetworkGame **networkGamehis;
static MENU_ID              networkItem[NUM_SEARCH_ROWS];
static XBAtom               networkAtom;
/* data for central */
char title[256];
char stat0[256];
char stat1[256];
char stat2[256];
char stat3[256];
char stat4[256];
XBBool autoCentral2=XBFalse;

/****************************************************
 * shared functions for client and server wait menu *
 ****************************************************/

static XBBool ButtonRefreshSearchCentral (void *par);

/*
 * free hostnames, only use after ServerInit or ClientInit
 */
static void
ClearHostNames (void)
{
  int i;
  for (i = 0; i < MAX_HOSTS; i ++) {
    if (NULL != hostName[i]) {
      free (hostName[i]);
      hostName[i] = NULL;
    }
  }
} /* ClearHostNames */

/*
 * send a host state request, client or server
 */
static void
SendHostStateReq (unsigned id, unsigned state)
{
  switch (Network_GetType()) {
  case XBNT_Server: Server_ReceiveHostStateReq(0, id, state); break;
  case XBNT_Client: Client_SendHostStateReq(id, state); break;
  default: Dbg_Out("failed to queue host state request, unsupported network type\n");
  }
} /* SendHostStateReq */

#ifdef UNNEEDED
/*
 * send a team state request, client or server
 */
static void
SendTeamStateReq (unsigned id, unsigned player, unsigned team)
{
  switch (Network_GetType()) {
  case XBNT_Server: Server_ReceiveTeamStateReq(0, id, player, team); break;
  case XBNT_Client: Client_SendTeamStateReq(id, player, team); break;
  default: Dbg_Out("failed to queue team state request, unsupported network type\n");
  }
} /* SendTeamStateReq */
#endif

#ifndef OLDMENUS
/* new menu code */

#ifdef REQUESTS
/* detailed requests info in menu, experimental request handling */

/*
 * team change handler - handle local changes of team states
 */
static XBBool
TeamChangeLocal (unsigned id, unsigned player, XBTeamState *cur) {
  assert(id < MAX_HOSTS);
  assert(player < NUM_LOCAL_PLAYER);
  /* TODO: filtering, some host states might not be sensible */
  switch (Network_GetType()) {
  case XBNT_Server:
    *cur = (*cur == NUM_XBTS-1) ? XBTS_None : *cur+1;
    Server_ReceiveTeamStateReq(0,id,player,*cur);
    return XBTrue;
  case XBNT_Client:
    *cur = (*cur == NUM_XBTS-1) ? XBTS_None : *cur+1;
    Client_SendTeamStateReq(id,player,*cur);
    return XBTrue;
  default:
    Dbg_Out("local team change failed, unsupported network type\n");
    return XBFalse;
  }
} /* TeamChangeLocal */

/*
 * team update handler - handle external changes of team states
 */
static XBBool
TeamStateUpdate (unsigned id, unsigned player, XBTeamState *cur, XBTeamState *req) {
  XBHostState newteam;
  XBHostState *newreq;
  unsigned i;
  XBBool chg = XBFalse;
  assert(id < MAX_HOSTS);
  assert(player < NUM_LOCAL_PLAYER);
  newteam = Network_GetTeamState(id,player);
  newreq = Network_GetTeamStateReq(id,player);
  if (*cur != newteam) {
    /* team change, set team colors */
    *cur = newteam;
    switch (newteam) {
    case XBTS_Red:
      playerGfx[id][player].body = COLOR_RED;
      playerGfx[id][player].handsFeet = COLOR_RED;
      break;
    case XBTS_Green:
      playerGfx[id][player].body = COLOR_GREEN;
      playerGfx[id][player].handsFeet = COLOR_GREEN;
      break;
    case XBTS_Blue:
      playerGfx[id][player].body = COLOR_BLUE;
      playerGfx[id][player].handsFeet = COLOR_BLUE;
      break;
    case XBTS_None:
      playerGfx[id][player].body = playerGfx[id][player].bodySave;
      playerGfx[id][player].handsFeet = playerGfx[id][player].handsFeetSave;
      break;
    default:
      break;
    }
    chg = XBTrue;
  }
  for (i=0; i<MAX_HOSTS; i++) {
    if (req[i] != newreq[i]) {
      req[i] = newreq[i];
      chg = XBTrue;
    }
  }
  return chg;
} /* TeamStateUpdate */

/*
 * handle host state changes - local changes
 */
static XBBool
HostChangeLocal (unsigned id, XBHostState *cur) {
  assert (id < MAX_HOSTS);
  switch (Network_GetType()) {
  case XBNT_Server:
    switch (*cur) {
    case XBHS_None: return XBFalse;
    case XBHS_Wait: *cur = XBHS_Out; break;
    case XBHS_Server: *cur = XBHS_Ready; break;
    case XBHS_In: *cur = (id==0) ? XBHS_Server : XBHS_Out; break;
    case XBHS_Out: *cur = (id==0) ? XBHS_Server : XBHS_In; break;
    case XBHS_Ready: *cur = (id==0) ? XBHS_Server : XBHS_Out; break;
    default: break;
    }
    Server_ReceiveHostStateReq(0,id,*cur);
    return XBTrue;
  case XBNT_Client:
    switch (*cur) {
    case XBHS_None: return XBFalse;
    case XBHS_Wait: *cur = XBHS_Out; break;
    case XBHS_Server: *cur = XBHS_Out; break;
    case XBHS_In: *cur = (id==Client_ID()) ? XBHS_Ready : XBHS_Out; break;
    case XBHS_Out: *cur = XBHS_In; break;
    case XBHS_Ready: *cur = (id==Client_ID()) ? XBHS_In : XBHS_Out; break;
    default: break;
    }
    Client_SendHostStateReq(id,*cur);
    return XBTrue;
  default:
    Dbg_Out("local host change failed, unsupported network type\n");
    return XBFalse;
  }
} /* HostChangeLocal */

/*
 * handle host state update requests
 */
static XBBool
HostStateUpdate (unsigned id, XBHostState *cur, XBHostState *req, int *ping) {
  XBHostState newstate;
  XBHostState *newreq;
  int newping = -1;
  unsigned i;
  XBBool chg = XBFalse;
  assert (id < MAX_HOSTS);
  newping = Network_GetPingTime(id);
  newstate = Network_GetHostState(id);
  newreq = Network_GetHostStateReq(id);
  /* check state change */
  if (*cur != newstate) {
    *cur = newstate;
    chg = XBTrue;
  }
  /* check ping change */
  if (newping != *ping) {
    *ping = newping;
    chg = XBTrue;
  }
  /* check requests change */
  for (i=0; i<MAX_HOSTS; i++) {
    if (req[i] != newreq[i]) {
      req[i] = newreq[i];
      chg = XBTrue;
    }
  }
  return chg;
} /* HostStateUpdate  */

#else
/* classic display and request handling */

/*
 * team change handler - handle local changes of team states
 */
static XBBool
TeamChangeLocal (unsigned id, unsigned player, XBTeamState *cur) {
  assert(id < MAX_HOSTS);
  assert(player < NUM_LOCAL_PLAYER);
  /* TODO: filtering, some host states might not be sensible */
  switch (Network_GetType()) {
  case XBNT_Server:
    *cur = (*cur == NUM_XBTS-1) ? XBTS_None : *cur+1;
    Server_ReceiveTeamStateReq(0,id,player,*cur);
    return XBTrue;
  default:
    return XBFalse;
  }
} /* TeamChangeLocal */

/*
 * team update handler - handle external changes of team states
 */
static XBBool
TeamStateUpdate (unsigned id, unsigned player, XBTeamState *cur, XBTeamState *req) {
  XBTeamState newteam;
  XBTeamState *newreq;
  unsigned i;
  XBBool chg = XBFalse;
  assert(id < MAX_HOSTS);
  assert(player < NUM_LOCAL_PLAYER);
  newteam = Network_GetTeamState(id,player);
  newreq = Network_GetTeamStateReq(id,player);
  /* set team colors for player if teamchange */
  if (*cur != newteam) {
    *cur = newteam;
    switch (newteam) {
    case XBTS_Red:
      playerGfx[id][player].body = COLOR_RED;
      playerGfx[id][player].handsFeet = COLOR_RED;
      break;
    case XBTS_Green:
      playerGfx[id][player].body = COLOR_GREEN;
      playerGfx[id][player].handsFeet = COLOR_GREEN;
      break;
    case XBTS_Blue:
      playerGfx[id][player].body = COLOR_BLUE;
      playerGfx[id][player].handsFeet = COLOR_BLUE;
      break;
    case XBTS_None:
      playerGfx[id][player].body = playerGfx[id][player].bodySave;
      playerGfx[id][player].handsFeet = playerGfx[id][player].handsFeetSave;
      break;
    default:
      break;
    }
    chg = XBTrue;
  }
  /* update request data if necessary */
  for (i=0; i<MAX_HOSTS; i++) {
    if (req[i] != newreq[i]) {
      req[i] = newreq[i];
	chg = XBTrue;
    }
  }
  return chg;
} /* TeamStateUpdate */

/*
 * handle host state changes - local changes
 */
static XBBool
HostChangeLocal (unsigned id, XBHostState *cur) {
  assert (id < MAX_HOSTS);
  switch (Network_GetType()) {
  case XBNT_Server:
    /* server menu */
    switch (Network_GetHostState(id)) {
    case XBHS_Server:
      /* request for host marked as server*/
      assert(id==0);
      switch (*cur) {
      case XBHS_Ready: *cur = XBHS_None; break;
      default: *cur = XBHS_Ready; break;
      }
      break;
    case XBHS_In:
      /* request for host marked in */
      assert(id>0);
      switch (*cur) {
      case XBHS_Out: *cur = XBHS_None; break;
      default: *cur = XBHS_Out; break;
      }
      break;
    case XBHS_Ready:
      /* request for host marked ready */
      switch (*cur) {
      case XBHS_Server: *cur = XBHS_None; break;
      case XBHS_Out: *cur = XBHS_None; break;
      default: *cur = (id==0) ? XBHS_Server : XBHS_Out; break;
      }
      break;
    case XBHS_Out:
      /* request for host marked out */
      assert(id>0);
      switch (*cur) {
      case XBHS_In: *cur = XBHS_None; break;
      default: *cur = (hostComplete[id]==0) ? XBHS_In : XBHS_Wait; break;
      }
      break;
    case XBHS_Wait:
      /* request for host marked waiting */
      assert(id>0);
      switch (*cur) {
      case XBHS_Out: *cur = XBHS_None; break;
      default: *cur = XBHS_Out; break;
      }
      break;
    default: return XBFalse;
    }
    Server_ReceiveHostStateReq(0,id,*cur);
    return XBTrue;
  case XBNT_Client:
    if (id==Client_ID()) {
      /* request for local host item */
      switch (Network_GetHostState(id)) {
      case XBHS_In:
	assert(id>0);
	switch (*cur) {
	case XBHS_Ready: *cur = XBHS_None; break;
	default: *cur = XBHS_Ready; break;
	}
	break;
      case XBHS_Ready:
	switch (*cur) {
	case XBHS_In: *cur = XBHS_None; break;
	default: *cur = XBHS_In; break;
	}
	break;
      case XBHS_Out:
	switch (*cur) {
	case XBHS_In: *cur = XBHS_Ready; break;
	case XBHS_Ready: *cur = XBHS_None; break;
	default: *cur = XBHS_In; break;
	}
	break;
      default: return XBFalse;
      }
      Client_SendHostStateReq(id,*cur);
      return XBTrue;
    }
    return XBFalse;
  default:
    Dbg_Out("local host change failed, unsupported network type\n");
    return XBFalse;
  }
} /* HostChangeLocal */

/*
 * handle host state update requests
 */
static XBBool
HostStateUpdate (unsigned id, XBHostState *cur, XBHostState *req, int *ping) {
  XBHostState newstate;
  XBHostState *newreq;
  int newping = -1;
  unsigned i;
  XBBool chg = XBFalse;
  assert (id < MAX_HOSTS);
  newping = Network_GetPingTime(id);
  newstate = Network_GetHostState(id);
  newreq = Network_GetHostStateReq(id);
  /* check state change */
  if (*cur != newstate) {
    *cur = newstate;
    chg = XBTrue;
  }
  /* check ping change */
  if (newping != *ping) {
    *ping = newping;
    chg = XBTrue;
  }
  /* check requests change if necessary */
  for (i=0; i<MAX_HOSTS; i++) {
    if (req[i] != newreq[i]) {
      req[i] = newreq[i];
      chg = XBTrue;
    }
  }
  return chg;
} /* HostStateUpdate  */

#endif

/*
 * count valid players for a specific host
 */
static unsigned
CountHostPlayers(unsigned id)
{
  unsigned p,cnt=0;
  assert(id < MAX_HOSTS);
  for (p=0; p<NUM_LOCAL_PLAYER; p++) {
    if (playerValid[id][p]) {
      cnt+=1;
    }
  }
  return cnt;
} /* CountPlayers */

/*
 * count valid players
 */
static unsigned
CountPlayers()
{
  unsigned i,cnt=0;
  for (i=0; i<MAX_HOSTS; i++) {
    cnt += CountHostPlayers(i);
  }
  return cnt;
} /* CountPlayers */

/*
 * return if host is in
 */
static XBBool
HostIsIn(unsigned id)
{
  switch ( Network_GetHostState(id) ) {
  case XBHS_In:
  case XBHS_Ready: return XBTrue;
  default: return XBFalse;
  }
} /* HostIsIn */

/*
 * update team mask for team counting
 */
static void
TeamMaskUpdate(unsigned *mask, unsigned *pl, unsigned *tm, XBTeamState team)
{
  if (team == XBTS_None) {
    *pl = *pl +1;
    return;
  }
  if ( ! (*mask & (1<<team)) ) {
    /* count new team if team color, not in mask */
    *tm = *tm + 1;
    *mask = *mask | (1<<team);
  }
} /* TeamMaskUpdate */

#ifdef UNNEEDED
/*
 * count accepted teams by state only
 */
static unsigned
CountTeamsByState()
{
  unsigned id, pl, p, t, mask;
  mask = 0x00;
  for (id = 0; id < MAX_HOSTS; id ++) {
    if (HostIsIn(id)) {
      for (pl = 0; pl < NUM_LOCAL_PLAYER; pl ++) {
	TeamMaskUpdate(&mask, &p, &t, Network_GetTeamState(id,pl));
      }
    }
  }
  assert( t==0 || p==0);
  return(t+p);
} /* CountTeamsByState*/
#endif

/*
 * create player graphics in menu for client and server
 */
static void
CreatePlayerItems (void)
{
  int i;
  for (i = 0; i < NUM_LOCAL_PLAYER; i ++) {
    MenuAddPlayer (PLAYER_LEFT (i, NUM_LOCAL_PLAYER), PLAYER_TOP, PLAYER_WIDTH, i, pPlayerGfx + i, -ANIME_LENGTH, playerAnime);
  }
} /* CreatePlayerItems */

/*
 * team focus handler - focus changed to id(player)
 */
static void
TeamFocus (unsigned id, unsigned player) {
  assert (id < MAX_HOSTS);
  assert (player < NUM_LOCAL_PLAYER);
  /* show player sprite */
  memset(pPlayerGfx, 0, sizeof(pPlayerGfx));
  if (playerValid[id][player]) {
    pPlayerGfx[player] = &playerGfx[id][player];
  } else {
    Dbg_Out("focus on team item %u(%u), no entry!!\n",id,player);
    pPlayerGfx[player] = NULL;
  }
} /* TeamFocus */

#ifdef SMPF

/*
 * create SMPF team items in menu
 */
static void
CreateTeamItems ()
{
  int client = 0;
  int c = MENU_TOP;
  int r = MENU_TOP;
  int p;
  r -= CELL_H;
  for (client = 0; client < MAX_PLAYER; client++) {
    if (client % 3 == 0) {
      c = 5 * CELL_W / 2;
      r += CELL_H;
    } else {
      c += 4*CELL_W;
    }
    for (p=0; p<2; p++) {
      teamItem[client][p] = MenuAddTeam (c+1*CELL_W, r+(2+p)*CELL_H/2, 2*CELL_W, client, p, TeamFocus, TeamChangeLocal, TeamStateUpdate);
      if (!playerValid[client][p]) {
	MenuSetActive (teamItem[client][p], XBFalse);
      }
    }
  }
} /* CreateTeamItems */

/*
 * create SMPF host items in menu
 */
static void
CreateHostItems (XBBool server)
{
  int client = 0;
  int c      = MENU_TOP;
  int r      = MENU_TOP;
  r -= CELL_H;
  for (client = 0; client < MAX_PLAYER; client++) {
    if (client % 3 == 0) {
      c = 5 * CELL_W / 2;
      r += CELL_H;
    } else {
      c += 4*CELL_W;
    }
    MenuAddTag (c - 2*CELL_W/2, r + 2*CELL_H/2, 2*CELL_W, &playerName[client][0]);
    MenuAddTag (c - 2*CELL_W/2, r + 3*CELL_H/2, 2*CELL_W, &playerName[client][1]);
  }
  CreateTeamItems();
} /* CreateHostItems */

#else

/*
 * create team items in menu
 */
static void
CreateTeamItems ()
{
  int x, y;
  int client = 0;
  int p,c,r      = MENU_TOP;
  for (y = 0; y < 3; y ++) {
    c = 3 * CELL_W / 2;
    for (x = 0; x < 2; x ++) {
      /* host */
      for (p=0; p<2; p++) {
	teamItem[client][p] = MenuAddTeam (c+2*CELL_W, r+(2+p)*CELL_H/2, 3*CELL_W, client, p, TeamFocus, TeamChangeLocal, TeamStateUpdate);
	if (!playerValid[client][p]) {
	  MenuSetActive (teamItem[client][p], XBFalse);
	}
      }
      client ++;
      c += 6*CELL_W;
    }
    r += 2*CELL_H;
  }
} /* CreateTeamItems */

/*
 * focus changed to host item
 */
static void
HostFocus(unsigned id)
{
  /* show players on host */
  unsigned player;
  unsigned cnt = 0;
  assert (id < MAX_HOSTS);
  for (player = 0; player < NUM_LOCAL_PLAYER; player ++) {
    if (playerValid[id][player]) {
      pPlayerGfx[player] = &playerGfx[id][player];
      cnt ++;
    } else {
      pPlayerGfx[player] = NULL;
    }
  }
  if (cnt == 0) {
    Dbg_Out("focus on host item %u, %u players!\n", id, cnt);
  }
} /* HostFocus */

/*
 * create host items in menu
 */
static void
CreateHostItems (XBBool server)
{
  int x, y;
  unsigned client = 0;
  int c, r      = MENU_TOP;
  for (y = 0; y < MAX_PLAYER / 2; y ++) {
    c = 3 * CELL_W / 2;
    for (x = 0; x < 2; x ++) {
      hostItem[client] = MenuAddHost(c+1*CELL_W,r, 4*CELL_W, client, (const char **) hostName, HostFocus, HostChangeLocal, HostStateUpdate);
      if (CountHostPlayers(client) == 0) {
	MenuSetActive(hostItem[client],XBFalse);
      }
      MenuAddTag (c, r + 2*CELL_H/2, 2*CELL_W, &playerName[client][0]);
      MenuAddTag (c, r + 3*CELL_H/2, 2*CELL_W, &playerName[client][1]);
      client ++;
      c += 6*CELL_W;
    }
    r += 2*CELL_H;
  }
  CreateTeamItems ();
} /* CreateHostItems */

#endif

/*
 * handle network event "game config", client or server
 */
static void
HandleGameConfig (unsigned clientID)
{
  int i;
  CFGGamePlayers cfgPlayers;
  assert (clientID < MAX_HOSTS);
  Dbg_Out("host #%u sent game config\n", clientID);
  /* free old host name, if necessary */
  if (NULL != hostName[clientID]) {
    free (hostName[clientID]);
  }
  /* set host name from Remote Database */
  hostName[clientID]     = DupString (GetHostName (CT_Remote, atomArrayHost0[clientID]));
  /* retrieve config data */
  if (RetrieveGamePlayers (CT_Remote, atomArrayHost0[clientID], &cfgPlayers) ) {
    Dbg_Out("expecting %u players at host #%u\n", cfgPlayers.num, clientID);
    /* set up completeness check */
    hostComplete[clientID] = (1 << 0);
    for (i = 0; i < cfgPlayers.num && i < NUM_LOCAL_PLAYER; i ++) {
      if (ATOM_INVALID == cfgPlayers.player[i]) {
	break;
      }
      hostComplete[clientID] |= (1 << (i+1));
    }
    SendHostStateReq(clientID, XBHS_Wait);
    MenuSetActive(hostItem[clientID],XBTrue);
  } else {
    Dbg_Out("game config error for host %u\n", clientID);
    SendHostStateReq(clientID, XBHS_None);
  }
} /* HandleGameConfig */

/*
 * handle network event "player config", client or server
 */
static void
HandlePlayerConfig (unsigned clientID, int player)
{
  XBAtom playerAtom;

  assert (clientID < MAX_HOSTS);
  assert (player >= 0);
  assert (player < NUM_LOCAL_PLAYER);
  Dbg_Out("receiving player config for %u(%u)\n",clientID,player);
  /* get player data */
  playerAtom                    = Network_GetPlayer (clientID, player);
  playerName[clientID][player]  = GetPlayerName (CT_Remote, playerAtom);
  playerValid[clientID][player] = XBTrue;
  RetrievePlayerGraphics (CT_Remote, playerAtom, COLOR_INVALID, &playerGfx[clientID][player]);
  MenuSetActive (teamItem[clientID][player], XBTrue);
  /* check completeness and ping */
  hostComplete[clientID] &= ~ (1 << (1+player) );
  if (0 == hostComplete[clientID]) {
    SendHostStateReq(clientID, XBHS_In);
  }
} /* HandlePlayerConfig */

/*
 * handle ping event
 */
static void
HandlePing (unsigned clientID) {
  if (1 == hostComplete[clientID]) {
    SendHostStateReq(clientID, XBHS_In);
  }
  hostComplete[clientID] &= ~ (1 << 0);
} /* HandlePing */

/*
 * handle network events "disconnected", "error", client or server
 */
static void
HandleShutdown (unsigned clientID)
{
  int j;
  assert (clientID < MAX_HOSTS);
  /* clear host entry */
  if (NULL != hostName[clientID]) {
    free (hostName[clientID]);
    hostName[clientID] = NULL;
  }
  /* clear player data */
  memset (playerValid[clientID], 0, sizeof (playerValid[clientID]));
  memset (playerName[clientID],  0, sizeof (playerName[clientID]));
  /* disable host button */
  MenuSetActive(hostItem[clientID],XBFalse);
  /* disable team button */
  for (j = 0; j < NUM_LOCAL_PLAYER; j ++) {
    MenuSetActive (teamItem[clientID][j], XBFalse);
  }
  /* exit menu if server has disconnected */
  if (0 == clientID) {
    Client_Disconnect ();
    MenuExecFunc (CreateMainMenu, NULL);
  }
} /* HandleShutdown */

/* prototypes for start button handlers */
static XBBool ButtonClientStart (void *par);
static XBBool ButtonServerStart (void *par);

/*
 * Handle network event "start game"
 */
static void
HandleStartGame (unsigned clientID)
{
  assert (clientID < MAX_HOSTS);
  if (Network_GetType() == XBNT_Server) {
    MenuExecFunc (ButtonServerStart, NULL);
  } else {
    SetHostType (XBPH_Client1 + clientID - 1);
    MenuExecFunc (ButtonClientStart, NULL);
  }
} /* HandleStartGame */

/********************
 * server wait menu *
 ********************/

/*
 * polling for server wait
 */
static void
PollServerWaitMenu (void *par)
{
  XBNetworkEvent msg;
  unsigned       clientID;
  static char skip=1;
  XBAtom *atom = par;
  assert (atom != NULL);

  /* check network events */
  while (XBNW_None != (msg = Network_GetEvent (&clientID) ) ) {
    switch (msg) {
    case XBNW_Accepted:
      /* client has connected */
      break;
    case XBNW_GameConfig:        HandleGameConfig (clientID); break;
      /* game config received */
    case XBNW_RightPlayerConfig: HandlePlayerConfig (clientID, 0); break;
      /* player data received */
    case XBNW_LeftPlayerConfig:  HandlePlayerConfig (clientID, 1); break;
      /* player data received */
    case XBNW_Joy1PlayerConfig:  HandlePlayerConfig (clientID, 2); break;
      /* player data received */
    case XBNW_Joy2PlayerConfig:  HandlePlayerConfig (clientID, 3); break;
      /* player data received */
    case XBNW_PingReceived:      HandlePing (clientID); break;
      /* received a ping for client */
    case XBNW_Disconnected:      HandleShutdown (clientID); break;
      /* one host has disconnected */
    case XBNW_Error:             HandleShutdown (clientID); break;
      /* error in connection to host */
    case XBNW_StartGame:         HandleStartGame (clientID); break;
      /* start game has been triggered by requests */
    default:
      /* anything else */
      break;
    }
  }
  /* retransmit newgame with current player number */
  if (cfgServer.central && (skip++ ==0)) {
    fprintf(stderr,"players=%u\n",CountPlayers());
    Server_RestartNewGame(CountPlayers() & 0xFF,"");
  }
} /* PollNetwork */

/*
 * init display variables for server
 */
static void
ServerInit()
{
  unsigned player;
  CFGGamePlayers cfgGame;
  CFGGameSetup   cfgSetup;
  /* clear all data */
  memset (hostName,     0, sizeof (hostName));
  memset (hostComplete, 0, sizeof (hostComplete));
  memset (playerValid,  0, sizeof (playerValid));
  memset (playerGfx,    0, sizeof (playerGfx));
  memset (playerName,   0, sizeof (playerName));
  /* determine local name for server */
  hostName[0] = DupString ("localhost");
  /* determine local player */
  if (RetrieveGamePlayers (CT_Local, atomServer, &cfgGame) ) {
    Dbg_Out("found %u local players\n",cfgGame.num);
    assert(cfgGame.num < NUM_LOCAL_PLAYER);
    for (player = 0; player < cfgGame.num; player ++) {
      assert(ATOM_INVALID != cfgGame.player[player]);
      RetrievePlayerGraphics (CT_Local, cfgGame.player[player], COLOR_INVALID, &playerGfx[0][player]);
      playerValid[0][player] = XBTrue;
      playerName[0][player] = GetPlayerName (CT_Local, cfgGame.player[player]);
    }
  } else {
    Dbg_Out("no local players on server!\n");
  }

  /* determine initial teammode */
  RetrieveGameSetup (CT_Remote, atomLocal, &cfgSetup);
} /* ServerInit */

/*
 * start button activated
 */
static XBBool
ButtonServerStart (void *par)
{
  unsigned       id, pl, p, s, t, mask;
  XBPlayerHost   host;
  XBAtom         gameAtom;
  CFGGamePlayers cfgAllPlayers, cfgHostPlayers;
  CFGGameSetup   cfgSetup;
  XBTeamState    team;

  /* count by player configs */
  p = 0;
  s = 0;
  t = 0;
  mask = 0x00;
  for (id = 0, host = XBPH_Server; id < MAX_HOSTS; id ++, host ++) {
    /* get game config atom */
    if (id == 0) {
      gameAtom = atomLocal;
    } else if (HostIsIn(id)) {
      gameAtom = atomArrayHost0[id];
    } else {
      continue;
    }
    /* now get player data for host */
    if (! RetrieveGamePlayers (CT_Remote, gameAtom, &cfgHostPlayers) ) {
      Server_ReceiveHostStateReq(0,id,XBHS_Out);
      continue;
    }
    /* add each player */
    for (pl = 0; pl < cfgHostPlayers.num; pl ++) {
      team = Network_GetTeamState(id,pl);
      Dbg_Out("adding host %u, player %u, team %u\n",id,pl,team);
      cfgAllPlayers.player[p] = Network_GetPlayer (id, pl);
      cfgAllPlayers.control[p] = cfgHostPlayers.control[pl];
      cfgAllPlayers.playerID[p]= cfgHostPlayers.playerID[pl];
      cfgAllPlayers.host[p]    = host;
      cfgAllPlayers.team[p]    = team;
      TeamMaskUpdate(&mask, &s, &t, team);
      if (cfgAllPlayers.player[p]==ATOM_INVALID) {
	Dbg_Out("invalid atom for %u(%u), total %u\n",id,pl,p);
      }
      p++;
    }
  }
  Dbg_Out("%u players, %u chaos, %u teams\n",p, s, t);
  assert (s==0 || t==0);
  if (s+t <= 1) {
    GUI_ErrorMessage ("Need at least two teams/players!");
    return XBFalse;
  }
  cfgAllPlayers.num = p;
  /* store players in Remote/host0 database */
  StoreGamePlayers (CT_Remote, atomArrayHost0[0], &cfgAllPlayers);
  /* store server game setup in Remote/host0 database */
  if (RetrieveGameSetup (CT_Remote, atomLocal, &cfgSetup)) {
    cfgSetup.teamMode = (t>0) ? XBTM_Team : XBTM_None;
    StoreGameSetup (CT_Remote, atomArrayHost0[0], &cfgSetup);
  }
  /* close port for listening */
  Server_StopListen ();
  /* start/disconnect hosts */
  for (id = 1; id < MAX_HOSTS; id ++) {
    if (HostIsIn(id)) {
      Server_SendStart (id);
    } else {
      Server_SendDisconnect (id);
    }
  }
  /* clean up */
  ClearHostNames ();
  MenuClear ();
  return XBTrue;
} /* ButtonServerStart */

/*
 * disconnect button activated
 */
static XBBool
ButtonServerDisconnect (void *par)
{
  Dbg_Out("stopping server\n");
  Server_StopListen ();
  Dbg_Out("disconnecting clients\n");
  Server_SendDisconnectAll ();
  Dbg_Out("cleaning up\n");
  ClearHostNames();
  return CreateServerMenu (par);
} /* ButtonServerDisconnect */

/*
 * kickout button activated
 */
static XBBool
ButtonServerKickOut (void *par)
{
  int i;
  /* disconnect unwanted clients  */
  for (i = 1; i < MAX_HOSTS; i ++) {
    if (Server_GetHostState(i) == (unsigned) XBHS_Out) {
      Dbg_Out("kicking host %u\n",i);
      Server_SendDisconnect (i);
    }
  }
  return XBFalse;
} /* ButtonServerKickOut */

/*
 * build server wait menu
 */
static XBBool
CreateServerWaitMenu (void *par)
{
  XBAtom *atom = par;

  assert (atom != NULL);
  Dbg_Out("---- creating server wait menu ---\n");
  /* init server data */
  ServerInit ();
  /* build menu */
  MenuClear ();
  /* poll function */
  MenuAddCyclic (PollServerWaitMenu, par);
  /* build player shapes for menu */
  CreatePlayerItems ();
  /* Title */
  MenuAddLabel (TITLE_LEFT, TITLE_TOP, TITLE_WIDTH, "Waiting for Clients");
  MenuAddLabel1 (10, 79, TITLE_WIDTH+60, "Chat with lamer Rado and others in http://irc.xblast-center.com/");
  /* build host slots */
  CreateHostItems (XBTrue);
  /* Buttons */
  MenuSetAbort   (MenuAddHButton ( 3 * CELL_W/2, MENU_BOTTOM, 4*CELL_W, "Disconnect", ButtonServerDisconnect, par) );
  (void)          MenuAddHButton (11 * CELL_W/2, MENU_BOTTOM, 4*CELL_W, "Kick out",   ButtonServerKickOut,    par);
  MenuSetDefault (MenuAddHButton (19 * CELL_W/2, MENU_BOTTOM, 4*CELL_W, "Start",      ButtonServerStart,      NULL) );
  /* directional item linking */
  MenuSetLinks ();
  /* that's all*/
  return XBFalse;
} /* CreateServerWaitMenu */

/********************
 * client wait menu *
 ********************/

/*
 * polling for server wait
 */
static void
PollClientWaitMenu (void *par)
{
  XBNetworkEvent msg;
  unsigned       clientID;
  static char skip=1;
  XBAtom *atom = par;
  assert (atom != NULL);

  /* check network events */
  while (XBNW_None != (msg = Network_GetEvent (&clientID) ) ) {
    switch (msg) {
    case XBNW_GameConfig:        HandleGameConfig (clientID);	  break;
      /* game config received */
    case XBNW_RightPlayerConfig: HandlePlayerConfig (clientID, 0);	  break;
      /* player data received */
    case XBNW_LeftPlayerConfig:  HandlePlayerConfig (clientID, 1);	  break;
      /* player data received */
    case XBNW_Joy1PlayerConfig:  HandlePlayerConfig (clientID, 2);	  break;
      /* player data received */
    case XBNW_Joy2PlayerConfig:  HandlePlayerConfig (clientID, 3);	  break;
      /* player data received */
    case XBNW_Disconnected:      HandleShutdown (clientID);	  break;
      /* one host has disconnected */
    case XBNW_Error:             HandleShutdown (clientID);	  break;
      /* received start game */
    case XBNW_StartGame:         HandleStartGame (clientID);	  break;
      /* error in connection to host */
    default:
      /* anything else */
      break;
    }
  }
  /* retransmit newgame with current player number */
  if (cfgServer.central && (skip++ ==0)) {
    fprintf(stderr,"players=%u\n",CountPlayers());
    Server_RestartNewGame(CountPlayers() & 0xFF,"");
  }
} /* PollNetwork */

/*
 * init display variables for client
 */
static void
ClientInit()
{
  /* clear all data */
  memset (hostName,     0, sizeof (hostName));
  memset (hostComplete, 0, sizeof (hostComplete));
  memset (playerValid,  0, sizeof (playerValid));
  memset (playerGfx,    0, sizeof (playerGfx));
  memset (playerName,   0, sizeof (playerName));
} /* ClientInit */

/*
 * start network game as client
 */
static XBBool
ButtonClientStart (void *par)
{
  ClearHostNames ();
  /* clean up */
  MenuClear ();
  return XBTrue;
} /* ButtonClientStart */

/*
 * disconnect button activated
 */
static XBBool
ButtonClientDisconnect (void *par)
{
  Dbg_Out("disconnecting from server\n");
  Client_Disconnect ();
  /* back to connection menu */
  return CreateJoinNetGameMenu (par);
} /* ButtonDisconnect */

/*
 * build client wait menu
 */
static XBBool
CreateClientWaitMenu (void *par)
{
  XBAtom *atom = par;
  assert (atom != NULL);
  /* needed inits */
  ClientInit ();
  /* build menu */
  MenuClear ();
  /* poll function */
  MenuAddCyclic (PollClientWaitMenu, par);
  /* players */
  CreatePlayerItems ();
  /* Title */
  MenuAddLabel (TITLE_LEFT, TITLE_TOP, TITLE_WIDTH, "Game Setup by Server");
  /* hosts */
  teamMode=0;
  CreateHostItems (XBFalse);
  MenuAddLabel1 (10, 79, TITLE_WIDTH+60, "Chat with lamer Rado and others in http://irc.xblast-center.com/");
  /* Buttons */
  MenuSetAbort (MenuAddHButton (3*CELL_W/2, MENU_BOTTOM, 4*CELL_W, "Disconnect", ButtonClientDisconnect, par) );
  /* --- */
  MenuSetLinks ();
  /* that's all*/
  return XBFalse;
} /* CreateClientWaitMenu */


#else
/* old menu code don't use */

/* host data */
static XBHostState  hostState[MAX_HOSTS];
static int          hostPing[MAX_HOSTS];

/* team data */
static XBTeamState  teamState[MAX_HOSTS][NUM_LOCAL_PLAYER];

/* focus data */
static MENU_ID  lastFocus = 0;
static unsigned lastHost  = MAX_HOSTS;

/* network games */
static const XBNetworkGame *networkGame[NUM_SEARCH_ROWS];
static XBNetworkGame **networkGamehis;
static MENU_ID              networkItem[NUM_SEARCH_ROWS];
static XBAtom               networkAtom;

/* all koen's shit for teams */
#define largeNUMBER 254

static XBTeamState  oldTeamState[MAX_HOSTS][NUM_LOCAL_PLAYER];
static unsigned  teamID=-1, clientTeam=-1;

/* names of all possible players */
static XBHostState  oldHostState[MAX_HOSTS];

/******************************
 * shared stuff client/server *
 ******************************/

/*
 * count teams
 */
static int
CountTeams ()
{
  int i, j, k, reinco;

  k=0;
  if (teamMode) {
    reinco=0;
    for (i = 0; i < MAX_HOSTS; i ++) {
      for (j = 0; j < NUM_LOCAL_PLAYER; j ++) {
	if (teamState[i][j] >= XBTS_Red) {
	  if(!(reinco&(1 << teamState[i][j]))) {
	    reinco|=1 << teamState[i][j];
	    k++;
	  }
	}
      }
    }
    if (k <= 1) {
      return(-1);
    }
  }
  return(k);
} /* CountTeams */

#ifdef SMPF
/*
 * create SMPF team items in menu
 */
static void
CreateTeamItems ()
{
  int client = 0;
  int c = MENU_TOP;
  int r = MENU_TOP;
  r -= CELL_H;
  for (client = 0; client < MAX_PLAYER; client++) {
    if (client % 3 == 0) {
      c = 5 * CELL_W / 2;
      r += CELL_H;
    } else {
      c += 4*CELL_W;
    }
    if (teamMode) {
      teamItem[client][0] = MenuAddServerTeam (c+1*CELL_W, r+2*CELL_H/2, 2*CELL_W, &teamState[client][0]);
      teamItem[client][1] = MenuAddServerTeam (c+1*CELL_W, r+3*CELL_H/2, 2*CELL_W, &teamState[client][1]);
      MenuSetActive (teamItem[client][0], XBFalse);
      MenuSetActive (teamItem[client][1], XBFalse);
    }
  }
} /* CreateTeamItems */

/*
 * create SMPF host items in menu
 */
static void
CreateHostItems (XBBool server)
{
  int client = 0;
  int c      = MENU_TOP;
  int r      = MENU_TOP;

  r -= CELL_H;
  for (client = 0; client < MAX_PLAYER; client++) {
    if (client % 3 == 0) {
      c = 5 * CELL_W / 2;
      r += CELL_H;
    } else {
      c += 4*CELL_W;
    }
    if (0 == client) {
      if (teamMode && server) {
	teamItem[client][0] = MenuAddServerTeam (c+1*CELL_W, r+2*CELL_H/2, 2*CELL_W, &teamState[client][0]);
	teamItem[client][1] = MenuAddServerTeam (c+1*CELL_W, r+3*CELL_H/2, 2*CELL_W, &teamState[client][1]);
	if (!playerValid[0][1]) { MenuSetActive (teamItem[client][1], XBFalse); }
      }
    } else {
      if(teamMode && server) {
	teamItem[client][0] = MenuAddServerTeam (c+1*CELL_W, r+2*CELL_H/2, 2*CELL_W, &teamState[client][0]);
	teamItem[client][1] = MenuAddServerTeam (c+1*CELL_W, r+3*CELL_H/2, 2*CELL_W, &teamState[client][1]);
      }
    }
    MenuAddTag (c - 2*CELL_W/2, r + 2*CELL_H/2, 2*CELL_W, &playerName[client][0]);
    MenuAddTag (c - 2*CELL_W/2, r + 3*CELL_H/2, 2*CELL_W, &playerName[client][1]);
  }
} /* CreateHostItems */

#else

/*
 * create standard team items in menu
 */
static void
CreateTeamItems ()
{
  int x,y;
  int client = 0;
  int c,r      = MENU_TOP;
  for (y = 0; y < 3; y ++) {
    c = 5 * CELL_W / 2;
    for (x = 0; x < 2; x ++) {
      /* host */
      if (teamMode) {
	teamItem[client][0] = MenuAddPeerTeam (c+2*CELL_W, r+2*CELL_H/2, 2*CELL_W, &teamState[client][0]);
	teamItem[client][1] = MenuAddPeerTeam (c+2*CELL_W, r+3*CELL_H/2, 2*CELL_W, &teamState[client][1]);
	MenuSetActive (teamItem[client][0], XBFalse);
	MenuSetActive (teamItem[client][1], XBFalse);
      }
      client ++;
      c += 6*CELL_W;
    }
    r += 2*CELL_H;
  }
} /* CreateTeamItems */

/*
 * create standard host items in menu
 */
static void
CreateHostItems (XBBool server)
{
  int x,y;
  int player;
  int client = 0;
  int c, r      = MENU_TOP;


  Dbg_Out("====================\n");
  for (y = 0; y < MAX_PLAYER / 2; y ++) {
    c = 5 * CELL_W / 2;
    for (x = 0; x < 2; x ++) {
      /* host */
      if (0 == client) {
	hostItem[client] = MenuAddServer (c, r, 4*CELL_W, (const char **) hostName + client);
	if(teamMode && server) {
	  teamItem[client][0] = MenuAddServerTeam (c+2*CELL_W, r+2*CELL_H/2, 2*CELL_W, &teamState[client][0]);
	  teamItem[client][1] = MenuAddServerTeam (c+2*CELL_W, r+3*CELL_H/2, 2*CELL_W, &teamState[client][1]);
	  if (!playerValid[0][1]) { MenuSetActive (teamItem[client][1], XBFalse); }
	}
      } else {
	if (server) {
	  hostItem[client] = MenuAddClient (c, r, 4*CELL_W, (const char **) hostName + client, hostState + client, hostPing + client);
	  if(teamMode && server) {
	    teamItem[client][0] = MenuAddServerTeam (c+2*CELL_W, r+2*CELL_H/2, 2*CELL_W, &teamState[client][0]);
	    teamItem[client][1] = MenuAddServerTeam (c+2*CELL_W, r+3*CELL_H/2, 2*CELL_W, &teamState[client][1]);
	  }
	} else {
	  hostItem[client] = MenuAddPeer (c, r, 4*CELL_W, (const char **) hostName + client, hostState + client, hostPing + client);
	  if(teamMode && server) {
	    teamItem[client][0] = MenuAddPeerTeam (c+2*CELL_W, r+2*CELL_H/2, 2*CELL_W, &teamState[client][0]);
	    teamItem[client][1] = MenuAddPeerTeam (c+2*CELL_W, r+3*CELL_H/2, 2*CELL_W, &teamState[client][1]);
	  }
	}
	if (teamMode && server) {
	  MenuSetActive (teamItem[client][0], XBFalse);
	  MenuSetActive (teamItem[client][1], XBFalse);
	}
	MenuSetActive (hostItem[client], XBFalse);
      }

      /* players */
      player = 0;
      MenuAddTag (c - 2*CELL_W/2, r + 2*CELL_H/2, 2*CELL_W, &playerName[client][player ++]);
      MenuAddTag (c - 2*CELL_W/2, r + 3*CELL_H/2, 2*CELL_W, &playerName[client][player ++]);
      if(0) {
	MenuAddTag (c + 1*CELL_W, r + 2*CELL_H/2, 2*CELL_W, &playerName[client][player ++]);
	MenuAddTag (c + 1*CELL_W, r + 3*CELL_H/2, 2*CELL_W, &playerName[client][player ++]);
	MenuAddTag (c + 3*CELL_W, r + 2*CELL_H/2, 2*CELL_W, &playerName[client][player ++]);
	MenuAddTag (c + 3*CELL_W, r + 3*CELL_H/2, 2*CELL_W, &playerName[client][player ++]);
      } else {
	player+=4;
      }
      client ++;
      c += 6*CELL_W;
    }
    r += 2*CELL_H;
  }
  Dbg_Out("====================\n");

} /* CreateHostItems */

#endif

/*
 * start network game as client
 */
static XBBool
ButtonClientStart (void *par)
{
  ClearHostNames ();
  /* clean up */
  MenuClear ();
  return XBTrue;
} /* ButtonClientStart */

/*
 * handle network event "player config"
 */
static void
HandlePlayerConfig (unsigned clientID, int player)
{
  int i,j;
  XBAtom playerAtom;

  assert (clientID < MAX_HOSTS);
  assert (player >= 0);
  assert (player < NUM_LOCAL_PLAYER);
  /* get player graphics */
  playerAtom                    = Network_GetPlayer (clientID, player);
  playerName[clientID][player]  = GetPlayerName (CT_Remote, playerAtom);
  playerValid[clientID][player] = XBTrue;
  RetrievePlayerGraphics (CT_Remote, playerAtom, COLOR_INVALID, &playerGfx[clientID][player]);
  /* todo: check completeness */
  hostComplete[clientID] &= ~ (1 << player);
  Dbg_Out ("HandlePlayerConfig: %u %02u\n", clientID, hostComplete[clientID]);
  if (0 == hostComplete[clientID]) {
    hostState[clientID] = XBHS_In;
#ifndef SMPF
    MenuSetActive (hostItem[clientID], XBTrue);
#endif
  }
  if (teamMode) {
    teamState[clientID][player] = XBTS_Red;
    MenuSetActive (teamItem[clientID][player], XBTrue);
    for (i = 0; i < MAX_HOSTS; i ++) {
      for (j = 0; j < NUM_LOCAL_PLAYER; j ++) {
	Server_SendTeamState(i, j, (unsigned) teamState[i][j]);
      }
    }
  }
} /* HandlePlayerConfig */

/*
 * handle network event "game config"
 */
static void
HandleGameConfig (unsigned clientID)
{
  int i;
  CFGGamePlayers cfgPlayers;

  assert (clientID < MAX_HOSTS);
  /* get host name */
  if (NULL != hostName[clientID]) {
    free (hostName[clientID]);
  }
  hostName[clientID]     = DupString (GetHostName (CT_Remote, atomArrayHost0[clientID]));
  hostState[clientID]    = XBHS_Wait;
  hostComplete[clientID] = 0;


  /* start check for completion */
  if (RetrieveGamePlayers (CT_Remote, atomArrayHost0[clientID], &cfgPlayers) ) {
    for (i = 0; i < cfgPlayers.num && i < NUM_LOCAL_PLAYER; i ++) {
      if (ATOM_INVALID == cfgPlayers.player[i]) {
	break;
      }
      hostComplete[clientID] |= (1 << i);
    }
  }
  Dbg_Out ("HandleGameConfig: %u %02X\n", clientID, hostComplete[clientID]);
  /* resend hoststates for other clients */
  memset (oldHostState, 0, sizeof (oldHostState));
} /* HandleGameConfig */

/*
 * handle network event "disconnected"
 */
static void
HandleShutdown (unsigned clientID)
{
  int j;
  assert (clientID < MAX_HOSTS);
  /* cleat host entry */
  if (NULL != hostName[clientID]) {
    free (hostName[clientID]);
    hostName[clientID] = NULL;
  }
  hostState[clientID] = XBHS_None;
  for (j = 0; j < NUM_LOCAL_PLAYER; j ++) {
    teamState[clientID][j] = XBTS_None;
  }
  hostPing[clientID]  = -1;
  /* clear player data */
  memset (playerValid[clientID], 0, sizeof (playerValid[clientID]));
  memset (playerName[clientID],  0, sizeof (playerName[clientID]));
  /* disable button */
#ifndef SMPF
  MenuSetActive (hostItem[clientID], XBFalse);
#endif
  for (j = 0; j < NUM_LOCAL_PLAYER; j ++) {
    MenuSetActive (teamItem[clientID][j], XBFalse);
  }
  /* exit menu if server has disconnected */
  if (0 == clientID) {
    Client_Disconnect ();
    MenuExecFunc (CreateMainMenu, NULL);
  }
} /* HandleShutdown */

/*
 * Handle network event "start game"
 */
static void
HandleStartGame (unsigned clientID)
{
  assert (clientID < MAX_HOSTS);

  SetHostType (XBPH_Client1 + clientID - 1);
  MenuExecFunc (ButtonClientStart, NULL);
} /* HandleStartGame */

/*
 * Handle network events "host state"
 */
static void
HandleHostState (unsigned clientID, XBBool isServer)
{
  unsigned state;
  assert (clientID < MAX_HOSTS);
  if (isServer) {
    state = Server_GetHostState(clientID);
  } else {
    state = Client_GetHostState(clientID);
  }
  hostState[clientID] = (XBHostState) state;
  Dbg_Out ("server changed host %u to %u\n", clientID, state);
} /* HandleHostState */

/*
 * Handle network events "team state"
 */
static void
HandleTeamChange (unsigned clientID, unsigned teamID)
{
  assert (clientID < MAX_HOSTS*NUM_LOCAL_PLAYER);
  teamState[clientID / NUM_LOCAL_PLAYER][clientID % NUM_LOCAL_PLAYER] = teamID;
  Dbg_Out ("server changed team %u (host %u, player %u) state to %u\n", clientID,
	   (clientID / NUM_LOCAL_PLAYER),(clientID % NUM_LOCAL_PLAYER), teamID);
} /* HandleTeamChange */

/*
 * handle network event "ping received"
 */
static void
HandlePingReceived (unsigned clientID, XBBool isServer)
{
  assert (clientID < MAX_HOSTS);
  if (isServer) {
    hostPing[clientID] = Server_GetPingTime (clientID);
  } else {
    hostPing[clientID] = Client_GetPingTime (clientID);
  }
} /* HandlePingReceived */

/*
 * Handle any networks events
 */
static void
HandleNetworkEvents (XBBool isServer)
{
  XBNetworkEvent msg;
  unsigned       clientID;

  while (XBNW_None != (msg = Network_GetEvent (&clientID) ) ) {
    switch (msg) {
      /* player data received */
    case XBNW_RightPlayerConfig: HandlePlayerConfig (clientID, 0);	  break;
    case XBNW_LeftPlayerConfig:  HandlePlayerConfig (clientID, 1);	  break;
    case XBNW_Joy1PlayerConfig:  HandlePlayerConfig (clientID, 2);	  break;
    case XBNW_Joy2PlayerConfig:  HandlePlayerConfig (clientID, 3);	  break;
      /* game config received */
    case XBNW_GameConfig:        HandleGameConfig (clientID);	  break;
      /* one host has disconnected */
    case XBNW_Disconnected:      HandleShutdown (clientID);	  break;
      /* error in connection to host */
    case XBNW_Error:             HandleShutdown (clientID);	  break;
      /* ping received from client */
    case XBNW_PingReceived:      HandlePingReceived (clientID, isServer); break;
      /* server wants to start game */
    case XBNW_StartGame:         HandleStartGame (clientID);              break;
      /* server has changed host state */
    case XBNW_HostChange:
      HandleHostState (clientID, isServer);
      break;
    case XBNW_TeamChange:
      if (Network_GetType() == XBNT_Server) {
	return;
      }
      //Dbg_Out("--- Team %u %u %u---\n", clientID, clientTeam, teamID);
      if(teamMode==0) {
	teamMode=1;
	CreateTeamItems();
      }
      if (!isServer) {
	if(clientTeam>largeNUMBER) {
	  if(teamID<largeNUMBER) {
	    HandleTeamChange (clientID, teamID);
	    clientTeam=-1;
	    teamID=-1;
	  } else {
	    clientTeam=clientID;
	  }
	} else {
	    // error
	  Dbg_Out("--- Team set error ---\n");
	}
      }
      break;
    case XBNW_TeamChangeData:
      //Dbg_Out("--- Team data %u %u %u ---\n", clientID, clientTeam, teamID);
      if (Network_GetType() == XBNT_Server) {
	return;
      }
      if(teamMode==0) {
	teamMode=1;
	CreateTeamItems();
      }
      if(clientID<largeNUMBER) {
	if (!isServer) {
	  if(teamID>largeNUMBER) {
	    if(clientTeam<largeNUMBER) {
	      HandleTeamChange (clientTeam, clientID);
	      clientTeam=-1;
	      teamID=-1;
	    } else {
	      teamID=clientID;
	    }
	  } else {
	    // error
	    Dbg_Out("--- Team data set error ---\n");
	  }
	}
      }
      break;
      /* anything else */
    default:
      break;
    }
  }
} /* HandleNetworkEvents */

/*
 *
 */
static void
UpdateFocus (void)
{
  MENU_ID  newFocus;
  unsigned newHost;
  int      player,j;

  newFocus = MenuGetFocus ();
  if (lastFocus != newFocus) {
    if (0 != newFocus) {
      /* which host */
      for (newHost = 0; newHost < MAX_HOSTS; newHost ++) {
#ifndef SMPF
	if (hostItem[newHost] == newFocus) {
	  if (newHost != lastHost) {
	    for (player = 0; player < NUM_LOCAL_PLAYER; player ++) {
	      if (playerValid[newHost][player]) {
		pPlayerGfx[player] = &playerGfx[newHost][player];
	      } else {
		pPlayerGfx[player] = NULL;
	      }
	    }
	    lastHost = newHost;
	  }
	  break;
	}
#endif
	/* Added by KOEN team focus: does not work don't know why not! I Fixed IT!! */
	for(j=0; j< NUM_LOCAL_PLAYER; j++) {
	  if (teamItem[newHost][j] == newFocus) {
	    if (newHost != lastHost) {
	      for (player = 0; player < NUM_LOCAL_PLAYER; player ++) {
		if (playerValid[newHost][player]) {
		  pPlayerGfx[player] = &playerGfx[newHost][player];
		} else {
		  pPlayerGfx[player] = NULL;
		}
	      }
	      lastHost = newHost;
	    }
	    j=-1;
	    break;
	  }
	}
	if(j<0) { break; }
      }
    }
    lastFocus = newFocus;
  }
} /* UpdateFocus */

/*
 * send changed team states on server to clients
 */
static void
CheckTeamStates (void)
{
  unsigned i,j;

  /* Added by KOEN make seperate sub?? has to be done first */
  if(teamMode) {
    for (i = 0; i < MAX_HOSTS; i ++) {
      for(j=0; j< NUM_LOCAL_PLAYER; j++) {
	if (oldTeamState[i][j] != teamState[i][j]) {
	  Server_SendTeamState(i, j, (unsigned) teamState[i][j]);
	  oldTeamState[i][j] = teamState[i][j];
	}
      }
    }
  }
} /* CheckTeamStates */

/*
 * send changed hosts states on server to clients
 */
static void
CheckHostStates (void)
{
  unsigned i;
  for (i = 0; i < MAX_HOSTS; i ++) {
    if (oldHostState[i] != hostState[i]) {
      /* inform clients about state change */
      Server_SendHostState (i, (unsigned) hostState[i]);
      oldHostState[i] = hostState[i];
    }
  }

} /* CheckHostStates (void) */

/*
 * check for chat lines
 */
static void
CheckChat()
{
  XBChat *chat;
  chat = Chat_Pop();
  if ( chat != NULL ) {
    Dbg_Out("+++GUI+++ %s\n",chat->txt);
  }
} /* CheckChat */

/* count players who are in or ready*/
static int
CountPlayers()
{
  static unsigned i,j;
  XBPlayerHost   host;
  CFGGamePlayers cfgHostPlayers;
  XBAtom         gameAtom;

  j = 0;
  for (i = 0, host = XBPH_Server; i < MAX_HOSTS; i++, host++) {
    if (i == 0) {
      gameAtom = atomLocal;
    } else if (hostState[i] == XBHS_In) {
	gameAtom = atomArrayHost0[i];
    } else if (hostState[i] == XBHS_Ready) {
      gameAtom = atomArrayHost0[i];
    } else {
      continue;
    }
    if (! RetrieveGamePlayers (CT_Remote, gameAtom, &cfgHostPlayers) ) {
      continue;
    }
    j += cfgHostPlayers.num;
  }
  return j;
} /* count current players */

/*
 * deal with network events and other stuff, that has to be checked regularly
 */
static void
PollNetwork (void *par)
{
  XBBool isServer;
  static char skip=0;
  XBAtom *atom = par;
  assert (atom != NULL);

  isServer = (*atom == atomServer);
  /* check network events */
  HandleNetworkEvents (isServer);
  /* show players according to selected host */
  UpdateFocus ();
  /* check host states if server */
  if (isServer) {
    CheckHostStates ();
    CheckTeamStates ();
  }
  /* check if chat lines have come in */
  CheckChat();
  /* Kuni/Rado Patch : retransmit newgame with current player number */
  if (cfgServer.central && (skip++ ==0)) {
    fprintf(stderr,"players=%u\n",CountPlayers());
    Server_RestartNewGame(CountPlayers() & 0xFF,"");
  }
  /* end of Kuni/Rado patch */
} /* PollNetwork */

/*
 * local initializations
 */
static void
LocalInit (void)
{
  int i;
  memset (hostName,     0, sizeof (hostName));
  memset (hostState,    0, sizeof (hostState));
  memset (oldHostState, 0, sizeof (oldHostState));
  memset (hostComplete, 0, sizeof (hostComplete));
  memset (playerName,   0, sizeof (playerName));
  memset (playerGfx,    0, sizeof (playerGfx));
  memset (playerValid,  0, sizeof (playerValid));
  memset (hostPing,    -1, sizeof (hostPing));
  memset (teamState,    0, sizeof (teamState));
  memset (oldTeamState, 0, sizeof (oldTeamState));
  lastFocus   = 0;
  teamMode    = 0;
  lastHost    = MAX_HOSTS;
  for(i=0; i<NUM_LOCAL_PLAYER; i++) {
    pPlayerGfx[i]=NULL;
  }
} /* LocalInit */

/*
 * create player graphics in menu for client and server
 */
static void
CreatePlayerItems (void)
{
  int i;
  for (i = 0; i < NUM_LOCAL_PLAYER; i ++) {
    MenuAddPlayer (PLAYER_LEFT (i, NUM_LOCAL_PLAYER), PLAYER_TOP, PLAYER_WIDTH, i, pPlayerGfx + i, -ANIME_LENGTH, playerAnime);
  }
} /* CreatePlayerItems */

/********************
 * server wait menu *
 ********************/

/*
 * init display variables for server
 */
static void
ServerInit()
{
  unsigned player;
  unsigned i;
  CFGGamePlayers cfgGame;
  CFGGameSetup   cfgSetup;
  CFGGamePlayers cfgPlayers;
  /* clear all data */
  LocalInit();
  /* determine local name for server */
  hostName[0] = DupString ("localhost");
  /* determine local player */
  if (RetrieveGamePlayers (CT_Local, atomServer, &cfgGame) ) {
    Dbg_Out("found %u local players\n",cfgGame.num);
    for (player = 0; player < cfgGame.num; player ++) {
      playerValid[0][player] = XBTrue;
      RetrievePlayerGraphics (CT_Local, cfgGame.player[player], COLOR_INVALID, &playerGfx[0][player]);
      playerName[0][player] = GetPlayerName (CT_Local, cfgGame.player[player]);
    }
  } else {
    Dbg_Out("no local players on server!\n");
  }
  /* determine initial teammode */
  RetrieveGameSetup (CT_Remote, atomLocal, &cfgSetup);
  teamMode = (XBPT_None != cfgSetup.teamMode);
  if (teamMode) {
    Dbg_Out("setting teams for local players\n");
    RetrieveGamePlayers (CT_Remote, atomLocal, &cfgPlayers);
    for (i = 0; i < cfgPlayers.num && i < NUM_LOCAL_PLAYER; i ++) {
      teamState[0][i]=XBTS_Red;
    }
  }
} /* ServerInit */

/*
 * start button activated
 */
static XBBool
ButtonServerStart (void *par)
{
  int            i, j, k;
  XBPlayerHost   host;
  XBAtom         gameAtom;
  CFGGamePlayers cfgAllPlayers, cfgHostPlayers;
  CFGGameSetup   cfgSetup;

  /* check team count */
  j = CountTeams();
  if (j<0) {
    GUI_ErrorMessage ("Only one team!");
    return XBFalse;
  }
  /* retrieve all player configs */
  j = 0;
  for (i = 0, host = XBPH_Server; i < MAX_HOSTS; i ++, host ++) {
    if (i == 0) {
      gameAtom = atomLocal;
    } else if (hostState[i] == XBHS_In) {
      gameAtom = atomArrayHost0[i];
    } else if (hostState[i] == XBHS_Ready) {
      gameAtom = atomArrayHost0[i];
    } else {
      continue;
    }
    if (! RetrieveGamePlayers (CT_Remote, gameAtom, &cfgHostPlayers) ) {
      continue;
    }
    for (k = 0; k < cfgHostPlayers.num; k ++) {
      cfgAllPlayers.player[j]  = Network_GetPlayer (i, k);
      cfgAllPlayers.control[j] = cfgHostPlayers.control[k];
      cfgAllPlayers.playerID[j]= cfgHostPlayers.playerID[k];
      cfgAllPlayers.host[j]    = host;
      cfgAllPlayers.team[j]    = (teamMode?teamState[i][k]:XBPT_None);
      j ++;
    }
  }
  cfgAllPlayers.num = j;
  /* and store it */

  StoreGamePlayers (CT_Remote, atomArrayHost0[0], &cfgAllPlayers);
  /* now the other configs */
  if (RetrieveGameSetup (CT_Remote, atomLocal, &cfgSetup)) {
    StoreGameSetup (CT_Remote, atomArrayHost0[0], &cfgSetup);
  }
  /* close port for listening */
  Server_StopListen ();
  /* send data to clients */
  if(teamMode) {
    for (i = 0; i < MAX_HOSTS; i ++) {
      for (j = 0; j < NUM_LOCAL_PLAYER; j ++) {
	Server_SendTeamState(i, j, (unsigned) teamState[i][j]);
      }
    }
  }

  for (i = 1; i < MAX_HOSTS; i ++) {
    switch (hostState[i]) {
    case XBHS_In:
      Server_SendStart (i);
      break;
    case XBHS_Ready:
      Server_SendStart (i);
      break;
    default:
      Server_SendDisconnect (i);
      break;
    }
  }
  /* clean up */
  ClearHostNames ();
  MenuClear ();
  return XBTrue;
} /* ButtonServerStart */

/*
 * disconnect button activated
 */
static XBBool
ButtonServerDisconnect (void *par)
{
  int i;
  /* close port for listening */
  Server_StopListen ();
  /* finish communication with clients */
  Server_SendDisconnectAll ();
  /* clean up */
  for (i = 1; i < MAX_HOSTS; i ++) {
    if (NULL != hostName[i]) {
      free (hostName[i]);
    }
  }
  /* back to connection menu */
  return CreateServerMenu (par);
} /* ButtonServerDisconnect */

/*
 * kickout button activated
 */
static XBBool
ButtonServerKickOut (void *par)
{
  int i;

  /* disconnect unwanted clients  */
  for (i = 1; i < MAX_HOSTS; i ++) {
    if (hostState[i] == XBHS_Out) {
      /* disconnect this client */
      Server_SendDisconnect (i);
      /* simluate disconnect from client */
      /* Server_ReceiveDisconnect (i); unneeded, SendDisconnect makes client appear as disconnected */
    }
  }
  /* back to connection menu */
  return XBFalse;
} /* ButtonServerDisconnect */

/*
 * build server wait menu
 */
static XBBool
CreateServerWaitMenu (void *par)
{
  XBAtom *atom = par;

  assert (atom != NULL);
  Dbg_Out("---- creating server wait menu ---\n");
  /* init server data */
  ServerInit ();
  /* build menu */
  MenuClear ();
  /* poll function for network events */
  MenuAddCyclic (PollNetwork, par);
  /* build player shapes for menu */
  CreatePlayerItems ();
  /* Title */
  MenuAddLabel (TITLE_LEFT, TITLE_TOP, TITLE_WIDTH, "Waiting for Clients");
  MenuAddLabel1 (10, 79, TITLE_WIDTH+60, "Chat with lamer Rado and others in http://irc.xblast-center.com/");
  /* build host slots */
  CreateHostItems (XBTrue);
  /* Buttons */
  MenuSetAbort   (MenuAddHButton ( 3 * CELL_W/2, MENU_BOTTOM, 4*CELL_W, "Disconnect", ButtonServerDisconnect, par) );
  (void)          MenuAddHButton (11 * CELL_W/2, MENU_BOTTOM, 4*CELL_W, "Kick out",   ButtonServerKickOut,    par);
  MenuSetDefault (MenuAddHButton (19 * CELL_W/2, MENU_BOTTOM, 4*CELL_W, "Start",      ButtonServerStart,      NULL) );
  /* directional item linking */
  MenuSetLinks ();
  /* that's all*/
  return XBFalse;
} /* CreateServerWaitMenu */

/********************
 * client wait menu *
 ********************/

/*
 * disconnect button activated
 */
static XBBool
ButtonClientDisconnect (void *par)
{
  Dbg_Out("disconnecting from server\n");
  Client_Disconnect ();
  /* back to connection menu */
  return CreateJoinNetGameMenu (par);
} /* ButtonDisconnect */

/*
 * toggle in/ready host state, inform server
 */
static XBBool
ButtonClientToggle (void *par)
{
  unsigned id = Client_ID();
  fprintf(stderr,"+++ toggle id %u\n",id);
  if (id < MAX_HOSTS) {
    if (hostState[id] == XBHS_In) {
      Client_SendHostState(XBHS_Ready);
    } else if (hostState[id] == XBHS_Ready) {
      Client_SendHostState(XBHS_In);
    } else {
      return XBFalse;
    }
    return XBTrue;
  }
  return XBFalse;
} /* ButtonClientToggle */

/*
 * build client wait menu
 */
static XBBool
CreateClientWaitMenu (void *par)
{
  XBAtom *atom = par;
  assert (atom != NULL);
  /* needed inits */
  LocalInit ();
  /* build menu */
  MenuClear ();
  /* poll function */
  MenuAddCyclic (PollNetwork, par);
  /* players */
  CreatePlayerItems ();
  /* Title */
  MenuAddLabel (TITLE_LEFT, TITLE_TOP, TITLE_WIDTH, "Game Setup by Server");
  /* hosts */
  teamMode=0;
  CreateHostItems (XBFalse);
 MenuAddLabel1 (10, 79, TITLE_WIDTH+60, "Chat with lamer Rado and others in http://irc.xblast-center.com/");
  /* Buttons */
  MenuSetAbort (MenuAddHButton (3*CELL_W/2, MENU_BOTTOM, 4*CELL_W, "Disconnect", ButtonClientDisconnect, par) );
  MenuSetDefault (MenuAddHButton (19*CELL_W/2, MENU_BOTTOM, 4*CELL_W, "Toggle", ButtonClientToggle, par) );
  /* --- */
  MenuSetLinks ();
  /* that's all*/
  return XBFalse;
} /* CreateClientWaitMenu */

#endif

/*********************
 * server setup menu *
 *********************/

/*
 * start the server
 */
XBBool
ButtonServerListen (void *par)
{
  XBAtom *atom = par;

  assert (atom != NULL);
  /* set server connection */
  cfgServer.name = NULL;
  cfgServer.port = serverPort;
  cfgServer.game = serverGame;
  /* store in database */
  StoreGameHost (CT_Local, *atom, &cfgServer);
  /* get game setup */
  /* now try connection */
  if (! Server_StartListen (&cfgServer) ) {
    /* connection failed */
    GUI_ErrorMessage ("Failed to listen on port %hu", serverPort);
    return CreateServerMenu (par);
  }
  return CreateServerWaitMenu (par);
} /* ButtonServerListen */

/*
 * back button activated
 */
XBBool
ButtonServerBack (void *par)
{
  return CreateStartNetGameMenu (par);
} /* ButtonServerBack*/

/*
 * setup xblast server
 */
XBBool
CreateServerMenu (void *par)
{
  XBAtom *atom = par;

  assert (atom != NULL);
  /* get old server from database */
  if (! RetrieveGameHost (CT_Local, *atom, &cfgServer)) {
    serverPort    = 16168;
    serverGame[0] = 0;
  } else {
    serverPort = cfgServer.port;
    strncpy (serverGame, cfgServer.game, sizeof (serverGame));
    serverGame[sizeof (serverGame) - 1] = 0;
  }
  /* build menu */
  MenuClear ();
  /* Title */
  MenuAddLabel (TITLE_LEFT, TITLE_TOP, TITLE_WIDTH, "Setup Server");
  /* parameters */
  MenuAddInteger   (DLG_LEFT, MENU_ROW (0), DLG_WIDTH, "TCP-Port:",               2*CELL_W, &serverPort, 4096, 65535);
  MenuAddComboBool (DLG_LEFT, MENU_ROW (1), DLG_WIDTH, "Use fixed UDP-ports",     2*CELL_W, &cfgServer.fixedUdpPort);
  MenuAddComboBool (DLG_LEFT, MENU_ROW (2), DLG_WIDTH, "Allow clients using NAT", 2*CELL_W, &cfgServer.allowNat);
  MenuAddComboBool (DLG_LEFT, MENU_ROW (3), DLG_WIDTH, "Visible in LAN:",         2*CELL_W, &cfgServer.browseLan);
  MenuAddComboBool (DLG_LEFT, MENU_ROW (4), DLG_WIDTH, "Visible in central:",     2*CELL_W, &cfgServer.central); // XBCC
  MenuAddString    (DLG_LEFT, MENU_ROW (5), DLG_WIDTH, "Name:",                   4*CELL_W, serverGame, sizeof (serverGame));
  MenuAddComboBool    (DLG_LEFT, MENU_ROW (6), DLG_WIDTH, "Beep for new clients:",                   2*CELL_W,&cfgServer.beep );
  /* Buttons */
  MenuSetAbort   (MenuAddHButton ( 5 * CELL_W/2, MENU_BOTTOM, 4*CELL_W, "Back",     ButtonServerBack, par) );
  MenuSetDefault (MenuAddHButton (17 * CELL_W/2, MENU_BOTTOM, 4*CELL_W, "Continue", ButtonServerListen,     par) );
  /* --- */
  MenuSetLinks ();
  /* that's all*/
  return XBFalse;
} /* CreateServerMenu */


/*********************
 * central wait menu *
 *********************/

/* forward declaration */
static XBBool CreateCentralWaitMenu (void *par);

/*
 * stop button activated
 */
static XBBool
ButtonCentralStop (void *par)
{
  /* finish communication to server */
  Central_StopListen ();
  /* finish communication with clients */
  Central_SendDisconnectAll ();
  /* back to connection menu */
  return CreateCentralMenu (par);
} /* ButtonDisconnect */

/*
 * update button activated
 */
static XBBool
ButtonCentralUpdate (void *par)
{
  return CreateCentralWaitMenu (par);
} /* ButtonCentralUpdate */

/*
 * poll function for central
 */
static void
PollCentral (void *par)
{
  static char i=0;
  XBAtom *atom = par;
  assert (atom != NULL);
  /* clear old games every 256th time */
  if (i++ == 0) {
    C2B_ClearOldGames();
  }
} /* PollCentral */

/*
 * build central wait menu
 */
static XBBool
CreateCentralWaitMenu (void *par)
{
  XBAtom *atom = par;
  int nPlayers = 0, nGames = 0, nGamesPlayed = 0, nTotalGames = 0, nLogins = 0;
  Central_GetStatistics(&nPlayers, &nGames, &nGamesPlayed, &nTotalGames, &nLogins);
  assert (atom != NULL);
  MenuClear ();
  /* poll function */
  MenuAddCyclic (PollCentral, par); // func has to be void mi_tool.h:46
  /* Title */
  sprintf(title,"Central: %s", serverGame);
  MenuAddLabel (TITLE_LEFT, TITLE_TOP, TITLE_WIDTH, title);

  sprintf(stat0,"Number of known players: %i", nPlayers);
  MenuAddLabel (5*CELL_W/2, MENU_ROW (1), 10*CELL_W, stat0);

  sprintf(stat1,"Number of open games: %i", nGames);
  MenuAddLabel (5*CELL_W/2, MENU_ROW (2), 10*CELL_W, stat1);

  sprintf(stat2,"Number of played games: %i", nGamesPlayed);
  MenuAddLabel (5*CELL_W/2, MENU_ROW (3), 10*CELL_W, stat2);

  sprintf(stat3,"Total number of games played: %i", nTotalGames);
  MenuAddLabel (5*CELL_W/2, MENU_ROW (4), 10*CELL_W, stat3);

  sprintf(stat4,"Total number of logins: %i", nLogins);
  MenuAddLabel (5*CELL_W/2, MENU_ROW (5), 10*CELL_W, stat4);
  /* Buttons */
  MenuSetDefault (MenuAddHButton (16*CELL_W/2, MENU_BOTTOM, 5*CELL_W, "Update now", ButtonCentralUpdate, par) );
  MenuSetAbort (MenuAddHButton (4*CELL_W/2, MENU_BOTTOM, 5*CELL_W, "Stop", ButtonCentralStop, par) );
  /* --- */
  MenuSetLinks ();
  /* that's all*/
  return XBFalse;
} /* CreateClientWaitMenu */

/**********************
 * setup central menu *
 **********************/

/*
 * setup central connection XBCC
 * used in menu_game.c, why ??
 */
void setAutoCentral2(XBBool set) {
  autoCentral2=set;
} /* setAutoCentral2 */

/*
 * start central button activated
 */
XBBool
ButtonCentralListen (void *par)
{
  XBAtom *atom = par;

  assert (atom != NULL);
  /* set server connection */
  cfgServer.name = NULL;
  cfgServer.port = serverPort;
  cfgServer.game = serverGame;
  /* store in database */
  StoreGameHost (CT_Local, *atom, &cfgServer);
  /* get game setup */
  /* now try connection */
  if (! Central_StartListen (&cfgServer) ) {
    /* connection failed */
    GUI_ErrorMessage ("Failed to listen on port %hu", serverPort);
    return CreateCentralMenu (par);
  }
  return CreateCentralWaitMenu (par);
} /* ButtonServerListen */

/*
 * back button activated
 */
static XBBool
ButtonMainMenu (void *par)
{
  /* call main menu */
  return CreateMainMenu (par);
} /* ButtonMainMenu */

/*
 * build setup central menu
 */
XBBool
CreateCentralMenu (void *par)
{
  XBAtom *atom = par;

  assert (atom != NULL);
  /* get old server from database */
  if (! RetrieveGameHost (CT_Local, *atom, &cfgServer)) {
    serverGame[0] = 0;
    serverPort    = 16168;
  } else {
    strncpy (serverGame, cfgServer.game, sizeof (serverGame));
    serverGame[sizeof (serverGame) - 1] = 0;
    serverPort = cfgServer.port;
  }
  if (autoCentral2) {
    return ButtonCentralListen(par);
  } else {
    /* build menu */
    MenuClear ();
    /* Title */
    MenuAddLabel (TITLE_LEFT, TITLE_TOP, TITLE_WIDTH, "Create a central");
    /* parameters */
    MenuAddString  (DLG_LEFT, MENU_ROW (0), DLG_WIDTH, "Central name:", 4*CELL_W, serverGame, sizeof (serverGame));
    MenuAddInteger (DLG_LEFT, MENU_ROW (1), DLG_WIDTH, "TCP-Port:", 4*CELL_W, &serverPort, 4096, 65535);
    /* Buttons */
    MenuSetAbort   (MenuAddHButton ( 5 * CELL_W/2, MENU_BOTTOM, 4*CELL_W, "Back",    ButtonMainMenu, par) );
    MenuSetDefault (MenuAddHButton (17 * CELL_W/2, MENU_BOTTOM, 4*CELL_W, "Start",   ButtonCentralListen,   par) );
    /* --- */
    MenuSetLinks ();
    /* that's all*/
    return XBFalse;
  }
} /* CreateCentralMenu */

/*******************
 * ip history menu *
 *******************/

XBBool CreateHistoryNetGameMenu (void *par);

/*
 * history entry activated
 */
static XBBool
ButtonStartLanGamehis (void *par)
{
  const XBNetworkGame **ptr = par;
  XBBool connect;
  assert (NULL != ptr);
  if (NULL == *ptr) {
    return XBFalse;
  }
  /* get server connection */
  cfgServer.name = (*ptr)->host;
  cfgServer.port = (*ptr)->port;
  /* store in database */
  StoreGameHost (CT_Local, networkAtom, &cfgServer);
  /* now try connection */
  connect = Client_Connect (&cfgServer);
  /* clean up */
  Client_StopQuery ();
  memset (networkGame, 0, sizeof (networkGame));
  if (! connect) {
    /* connection failed */
    GUI_ErrorMessage ("Connection to %s:%hu failed", serverName, serverPort);
    return CreateHistoryNetGameMenu (&networkAtom);
  }
  /*
    free networkgamehis, should be implemented!!!
    somehow i couldnt...
for(i=0;i<NUM_SEARCH_ROWS;i++){
    free((*networkGamehis[i]).host);
   free(networkGamehis[i]);
 }
 free(networkGamehis);*/

  return CreateClientWaitMenu (&networkAtom);
} /* ButtonStartLanGamehis */

/*
 * back button activated
 */
static XBBool
ButtonBackLanGamehis ( void *par)
{
  return CreateJoinNetGameMenu (par);
} /* ButtonBackLanGamehis */

/*
 * build history menu
 */
XBBool
CreateHistoryNetGameMenu (void *par)
{
  XBAtom *atom = par;
  int i;
  assert (NULL != atom);
  networkAtom = *atom;
  /* setup */
  if (RetrieveIpHistory( cfgServerhis,networkAtom) == XBFalse) {
    return XBFalse;
  }
  networkGamehis = malloc( sizeof (*networkGame)*NUM_SEARCH_ROWS);
  for (i=0; i<NUM_SEARCH_ROWS; i++) {
    (networkGamehis[i]) = malloc(sizeof(networkGame));
    if (i<10) {
      (*networkGamehis[i]).port=cfgServerhis[i].port;
      (*networkGamehis[i]).host=malloc(strlen (cfgServerhis[i].name));
      strcpy ((*networkGamehis[i]).host, cfgServerhis[i].name);
      fprintf(stderr," %s \n",(*networkGamehis[i]).host);
      (*networkGamehis[i]).game=" bla";
      (*networkGamehis[i]).version=" bla";
    }
    (*networkGamehis[i]).ping=1;
    (*networkGamehis[i]).numLives=1;
    (*networkGamehis[i]).numWins=1;
    (*networkGamehis[i]).frameRate=1;
  }
  MenuClear ();
  /* Title */
  MenuAddLabel (TITLE_LEFT, TITLE_TOP, TITLE_WIDTH, "History Ips");
  /* list of games */
  MenuSetActive (MenuAddGameHeader (SEARCH_LEFT, SEARCH_TOP, SEARCH_WIDTH), XBFalse);
  for (i = 0; i < 10; i ++) {
    networkItem[i] = MenuAddGameEntry (SEARCH_LEFT, SEARCH_ROW(i), SEARCH_WIDTH,(const XBNetworkGame **) (networkGamehis)+i, ButtonStartLanGamehis);
    MenuSetActive (networkItem[i], XBTrue);
  }
  /* Buttons */
  MenuSetAbort   (MenuAddHButton ( 5 * CELL_W/2, MENU_BOTTOM, 4*CELL_W, "Back",  ButtonBackLanGamehis,    par) );
  MenuSetLinks ();// should work infinite loop ??
  /* that's all*/
  return XBFalse;
} /* CreateHistoryNetGameMenu */

/***********************
 * client connect menu *
 ***********************/

/*
 * connect button activated
 */
static XBBool
ButtonClientConnect (void *par)
{
  XBAtom *atom = par;

  assert (atom != NULL);
  /* get server connection */
  cfgServer.name = serverName;
  cfgServer.port = serverPort;
  /* store in database */
  StoreIpHistory(&cfgServer,*atom);
  StoreGameHost (CT_Local, *atom, &cfgServer);
  /* now try connection */
  if (! Client_Connect (&cfgServer) ) {
    /* connection failed */
    GUI_ErrorMessage ("Connection to %s:%hu failed", serverName, serverPort);
    return CreateClientMenu (par);
  }
  return CreateClientWaitMenu (par);
} /* ButtonClientConnect */

/*
 * history button activated
 */
static XBBool
ButtonClientHistory (void *par)
{
  return CreateHistoryNetGameMenu (par);
} /* ButtonClientHistory */

/*
 * history button activated
 */
static XBBool
ButtonClientBack (void *par)
{
  return CreateJoinNetGameMenu (par);
} /* ButtonClientBack */

/*
 * setup connection to server
 */
XBBool
CreateClientMenu (void *par)
{
  XBAtom *atom = par;
  assert (atom != NULL);
  /* get old server from database */
  if (! RetrieveGameHost (CT_Local, *atom, &cfgServer)) {
    serverName[0] = 0;
    serverPort    = 16168;
  } else {
    strncpy (serverName, cfgServer.name, sizeof (serverName));
    serverName[sizeof (hostName) - 1] = 0;
    serverPort = cfgServer.port;
  }
  /* build menu */
  MenuClear ();
  /* Title */
  MenuAddLabel (TITLE_LEFT, TITLE_TOP, TITLE_WIDTH, "Connect to Server");
  /* parameters */
  MenuAddString  (DLG_LEFT, MENU_ROW (0), DLG_WIDTH, "Hostname:", 4*CELL_W, serverName, sizeof (serverName));
  MenuAddInteger (DLG_LEFT, MENU_ROW (1), DLG_WIDTH, "TCP-Port:", 4*CELL_W, &serverPort, 4096, 65535);
  /* Buttons */
  MenuSetAbort   (MenuAddHButton ( 3 * CELL_W/2, MENU_BOTTOM, 4*CELL_W, "Back",    ButtonClientBack, par) );
  MenuAddHButton ( 11 * CELL_W/2, MENU_BOTTOM, 4*CELL_W, "History", ButtonClientHistory , par) ;
  MenuSetDefault (MenuAddHButton (19 * CELL_W/2, MENU_BOTTOM, 4*CELL_W, "Connect", ButtonClientConnect,   par) );
  /* --- */
  MenuSetLinks ();
  /* that's all*/
  return XBFalse;
} /* CreateClientMenu */

/****************
 * search menus *
 ****************/

/*
 * poll incoming network games - for all search menus
 */
static void
PollNetworkGame (void *par)
{
  unsigned       gameID;
  XBNetworkEvent msg;

#ifdef W32
#else
  static struct timeval old;
  struct timeval tv;
  gettimeofday(&tv,NULL);
  if(tv.tv_sec-old.tv_sec>=10){
    ButtonRefreshSearchCentral(NULL);
    old=tv;
  }
#endif
  while (XBNW_None != (msg = Network_GetEvent (&gameID) ) ) {
    if (msg == XBNW_NetworkGame) {
      const XBNetworkGame *game = Client_NextNetworkGame ();
      if (gameID < NUM_SEARCH_ROWS) {
	networkGame[gameID] =( XBNetworkGame *)game;
	MenuSetActive (networkItem [gameID], ((NULL != game) && (game->frameRate!=0)&& (game->frameRate!=255)));
      }
    }
  }
} /* PollNetworkGame */

/*******************
 * search LAN menu *
 *******************/

/*
 * LAN entry activated
 */
static XBBool
ButtonStartLanGame (void *par)
{
  const XBNetworkGame **ptr = par;
  XBBool connect;

  assert (NULL != ptr);
  if (NULL == *ptr) {
    return XBFalse;
  }

  /* get server connection */
  cfgServer.name = (*ptr)->host;
  cfgServer.port = (*ptr)->port;
  /* store in database */
  StoreGameHost (CT_Local, networkAtom, &cfgServer);
  /* now try connection */
  connect = Client_Connect (&cfgServer);
  /* clean up */
  Client_StopQuery ();
  memset (networkGame, 0, sizeof (networkGame));
  if (! connect) {
    /* connection failed */
    GUI_ErrorMessage ("Connection to %s:%hu failed", serverName, serverPort);
    return CreateSearchLanMenu (&networkAtom);
  }

  return CreateClientWaitMenu (&networkAtom);
} /* ButtonStartLanGame */

/*
 * refresh button activated
 */
static XBBool
ButtonRefreshSearchLan (void *par)
{
  unsigned i;

  /* delete old games */
  teamMode=0;
  memset (networkGame, 0, sizeof (networkGame));
  for (i = 0; i < NUM_SEARCH_ROWS; i ++) {
    MenuSetActive (networkItem[i], XBFalse);
  }
  /* starts new query */
  Client_RestartQuery ();
  return XBFalse;
} /* ButtonRefreshSearchLam */

/*
 * exit button activated
 */
static XBBool
ButtonExitSearchLan (void *par)
{
  /* stop network search */
  Client_StopQuery ();
  teamMode=0;
  /* back to client menu */
  return CreateJoinNetGameMenu (par);
} /* ButtonExitSearchLan */

/*
 * build search LAN menu
 */
XBBool
CreateSearchLanMenu (void *par)
{
  XBAtom *atom = par;
  int i;

  assert (NULL != atom);
  networkAtom = *atom;
  /* setup */
  memset (networkGame, 0, sizeof (networkGame));
  (void) RetrieveGameHost (CT_Local, networkAtom, &cfgServer);
  /* build menu */
  MenuClear ();
  /* Title */
  MenuAddLabel (TITLE_LEFT, TITLE_TOP, TITLE_WIDTH, "Search LAN for Games");
  /* list of games */
  MenuSetActive (MenuAddGameHeader (SEARCH_LEFT, SEARCH_TOP, SEARCH_WIDTH), XBFalse);
  for (i = 0; i < NUM_SEARCH_ROWS; i ++) {
    networkItem[i] = MenuAddGameEntry (SEARCH_LEFT, SEARCH_ROW(i), SEARCH_WIDTH,(const XBNetworkGame **) networkGame + i, ButtonStartLanGame);
    MenuSetActive (networkItem[i], XBFalse);
  }
  /* Buttons */
  MenuSetAbort   (MenuAddHButton ( 5 * CELL_W/2, MENU_BOTTOM, 4*CELL_W, "Back",    ButtonExitSearchLan,    par) );
  MenuSetDefault (MenuAddHButton (17 * CELL_W/2, MENU_BOTTOM, 4*CELL_W, "Refresh", ButtonRefreshSearchLan, par) );
  /* add polling routine */
  MenuAddCyclic (PollNetworkGame, NULL);
  /* query for local games */
  Client_StartQuery ();

  // MenuSetLinks (); should work infinite loop ??
  /* that's all*/
  return XBFalse;
} /* CreateSearchLanMenu */

/***********************
 * search central menu *
 ***********************/

/*
 * central entry activated
 */
static XBBool
ButtonStartCentralGame (void *par)
{
  const XBNetworkGame **ptr = par;
  XBBool connect;

  assert (NULL != ptr);
  if (NULL == *ptr) {
    return XBFalse;
  }

  /* get server connection */
  cfgServer.name = (*ptr)->host;
  cfgServer.port = (*ptr)->port;
  /* store in database */
  StoreGameHost (CT_Local, networkAtom, &cfgServer);
  /* now try connection */
  connect = Client_Connect (&cfgServer);
  /* clean up */
  Client_StopQuery ();
  memset (networkGame, 0, sizeof (networkGame));
  if (! connect) {
    /* connection failed */
    GUI_ErrorMessage ("Connection to %s:%hu failed", serverName, serverPort);
    return CreateSearchCentralMenu (&networkAtom);
  }

  return CreateClientWaitMenu (&networkAtom);
} /* ButtonStartCentralGame */

/*
 * refresh button activated
 */
XBBool
ButtonRefreshSearchCentral (void *par)
{
  unsigned i;

  /* delete old games */
  teamMode=0;
  memset (networkGame, 0, sizeof (networkGame));
  for (i = 0; i < NUM_SEARCH_ROWS; i ++) {
    MenuSetActive (networkItem[i], XBFalse);
  }
  /* starts new query */
  Client_RestartQuery ();
  return XBFalse;
} /* ButtonRefreshSearchCentral */

/*
 *  exit button activated
 */
static XBBool
ButtonExitSearchCentral (void *par)
{
  /* stop network search */
  Client_StopQuery ();
  teamMode=0;
  /* back to client menu */
  return CreateJoinNetGameMenu (par);
} /* ButtonExitSearchCentral */

/*
 * search CENTRAL for network games XBCC
 */
XBBool
CreateSearchCentralMenu (void *par)
{
  XBAtom *atom = par;
  int i;

  assert (NULL != atom);
  networkAtom = *atom;
  /* setup */
  memset (networkGame, 0, sizeof (networkGame));
  (void) RetrieveGameHost (CT_Local, networkAtom, &cfgServer);
  /* build menu */
  MenuClear ();
  /* Title */
  MenuAddLabel2 (TITLE_LEFT-25, TITLE_TOP-15, TITLE_WIDTH+50, "Lamer Rado waits for you in http://irc.xblast-center.com");
  MenuAddLabel (TITLE_LEFT, TITLE_TOP, TITLE_WIDTH, "Central Games");
  /* list of games */
  MenuSetActive (MenuAddGameHeader (SEARCH_LEFT, SEARCH_TOP, SEARCH_WIDTH), XBFalse);
  for (i = 0; i < NUM_SEARCH_ROWS; i ++) {
    networkItem[i] = MenuAddGameEntry (SEARCH_LEFT, SEARCH_ROW(i), SEARCH_WIDTH,( const XBNetworkGame **)networkGame + i, ButtonStartCentralGame);
    MenuSetActive (networkItem[i], XBFalse);
  }
  /* Buttons */
  MenuSetAbort   (MenuAddHButton ( 5 * CELL_W/2, MENU_BOTTOM, 4*CELL_W, "Back",    ButtonExitSearchCentral,    par) );
  MenuSetDefault (MenuAddHButton (17 * CELL_W/2, MENU_BOTTOM, 4*CELL_W, "Refresh", ButtonRefreshSearchCentral, par) );
  /* add polling routine */
  MenuAddCyclic (PollNetworkGame, NULL);
  /* query for local games */
  Client_StartCentralQuery ();

  // MenuSetLinks (); should work infinite loop ??
  /* that's all*/
  return XBFalse;
} /* CreateSearchCentralMenu */

/*
 * end of file menu_network.c
 */
