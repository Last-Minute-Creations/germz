/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_GAME_H_
#define _GERMZ_GAME_H_

#include "map_data.h"
#include <ace/utils/font.h>
#include "steer.h"
#include "bob_new.h"
#include "fade.h"
#include "player.h"

#define HUD_OFFS_X 256
#define HUD_MONITOR_SIZE 64

typedef enum _tTeamConfig {
	TEAM_CONFIG_P1_P2_AND_P3_P4,
	TEAM_CONFIG_P1_P3_AND_P2_P4,
	TEAM_CONFIG_P1_P4_AND_P2_P3,
	TEAM_CONFIG_COUNT
} tTeamConfig;

typedef enum _tBattleMode {
	BATTLE_MODE_FFA,
	BATTLE_MODE_TEAMS,
} tBattleMode;

typedef enum _tTeamIdx {
	TEAM_1,
	TEAM_2,
	TEAM_NONE,
} tTeamIdx;

//------------------------------------------------------------------------ UTILS

void gameCopyBackToFront(void);

tBitMap *gameGetBackBuffer(void);

tBitMap *gameGetFrontBuffer(void);

void gameInitMap(void);

//------------------------------------------------------------------------ UTILS

UBYTE gamePreprocess(UBYTE isAllowPause);

void gamePostprocess(void);

tSteer *gameGetSteerForPlayer(UBYTE ubPlayer);

void gameDrawBlobAt(tTile eTile, UBYTE ubFrame, UWORD uwX, UWORD uwY);

void gameDrawMapTileAt(tUbCoordYX sPosTile, UBYTE ubFrame);

void gameDrawTileAt(tTile eTile, UWORD uwX, UWORD uwY, UBYTE ubFrame);

void gameInitCursorBobs(void);

void gameSetRules(
	UBYTE isEditor, tBattleMode eBattleMode, tTeamConfig eTeamCfg,
	UBYTE isCampaign
);

tBobNew *gameGetCursorBob(UBYTE ubIdx);

tFade *gameGetFade(void);

void gameQuit(void);

UBYTE *gameGetScores(void);

void gameRestart(void);

tPlayer **gameGetTeamLeaders(void);

UBYTE gameIsCampaign(void);

tBattleMode gameGetBattleMode(void);

tTeamIdx gameGetWinnerTeams(void);

tPlayerIdx gameGetWinnerFfa(void);

tTeamConfig gameGetTeamConfig(void);

tCopCmd *gameGetColorCopperlist(void);

void gameSetSpecialColors(UWORD uwSpecial1, UWORD uwSpecial2);

#endif // _GERMZ_GAME_H_
