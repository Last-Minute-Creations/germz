/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "game.h"
#include <ace/managers/key.h>
#include <ace/managers/joy.h>
#include <ace/managers/system.h>
#include <ace/managers/game.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/palette.h>
#include "map.h"
#include "menu.h"
#include "ai.h"
#include "player.h"
#include "assets.h"
#include "game_init.h"
#include "game_editor.h"
#include "game_play.h"
#include "game_pause.h"
#include "germz.h"
#include <bartman/gcc8_c_support.h>

static tView *s_pView;
static tVPort *s_pVp;
static tSimpleBufferManager *s_pBfr;
static tBobNew s_pCursorBobs[4];
static tFade *s_pFade;
static UBYTE s_isQuitting;
static UBYTE s_pScores[4];
static tBattleMode s_eBattleMode;
static tTeamConfig s_eTeamCfg;
static UBYTE s_ubCampaignStage;
static tPlayer *s_pTeamLeaders[2];
static const tPlayerIdx s_pPlayerTeamMates[TEAM_CONFIG_COUNT][4] = {
	[TEAM_CONFIG_P1_P2_AND_P3_P4] = {
		[PLAYER_1] = PLAYER_2, [PLAYER_2] = PLAYER_1,
		[PLAYER_3] = PLAYER_4, [PLAYER_4] = PLAYER_3,
	},
	[TEAM_CONFIG_P1_P3_AND_P2_P4] = {
		[PLAYER_1] = PLAYER_3, [PLAYER_2] = PLAYER_4,
		[PLAYER_3] = PLAYER_1, [PLAYER_4] = PLAYER_2,
	},
	[TEAM_CONFIG_P1_P4_AND_P2_P3] = {
		[PLAYER_1] = PLAYER_4, [PLAYER_2] = PLAYER_3,
		[PLAYER_3] = PLAYER_2, [PLAYER_4] = PLAYER_1,
	}
};

//------------------------------------------------------------------------ DEBUG

static UBYTE s_isEditor = 0;
static UBYTE s_isDebug = 0;
static UBYTE s_isDump = 0;
static tSteer s_pSteers[4];

void gameLag(void) {
	vPortWaitForEnd(s_pVp);
	vPortWaitForEnd(s_pVp);
	vPortWaitForEnd(s_pVp);
	vPortWaitForEnd(s_pVp);
}

void gameDumpFrame(void) {
	s_isDump = 1;
}

static void onGameRestartFadeout(void) {
	stateChange(g_pStateMachineGame, &g_sStateGameInit);
}

static void onGameQuitFadeout(void) {
	statePop(g_pStateMachineGame);
}

void gameQuit(void) {
	s_isQuitting = 1;
	fadeSet(s_pFade, FADE_STATE_OUT, 50, onGameQuitFadeout);
}

void gameRestart(void) {
	fadeSet(s_pFade, FADE_STATE_OUT, 50, onGameRestartFadeout);
}

UBYTE *gameGetScores(void) {
	return s_pScores;
}

//-------------------------------------------------------------------- GAMESTATE

UBYTE gamePreprocess(void) {
	fadeProcess(s_pFade);
	if(s_isQuitting) {
		// Already quitting - inform upper state to not do anything and wait
		// for fade end
		return 0;
	}
	else if(keyUse(KEY_ESCAPE) || keyUse(KEY_P)) {
		// TODO: if pause is triggered and text is written, be sure to restart HUD
		// state machine 'cuz it uses global textbitmap
		gamePauseEnable(PAUSE_KIND_BATTLE_PAUSE);
		return 0;
	}
	bobNewBegin(s_pBfr->pBack);
	return 1;
}

void gamePostprocess(void) {
	bobNewEnd();
	viewProcessManagers(s_pView);
	copProcessBlocks();
	debug_start_idle();
	vPortWaitUntilEnd(s_pVp);
	debug_stop_idle();
	if(s_isDump) {
		s_isDump = 0;
		static char szPath[30];
		static UWORD uwFrame = 0;
		sprintf(szPath, "debug/%hu.bmp", uwFrame++);
		bitmapSaveBmp(s_pBfr->pFront, s_pVp->pPalette, szPath);
	}

	if(keyCheck(KEY_B)) {
		gameLag();
	}
}

static void gameGsCreate(void) {
	UWORD uwCopListLength = simpleBufferGetRawCopperlistInstructionCount(5) + 2;

	s_pView = viewCreate(0,
		TAG_VIEW_COPLIST_MODE, COPPER_MODE_RAW,
		TAG_VIEW_COPLIST_RAW_COUNT, uwCopListLength,
		TAG_VIEW_GLOBAL_CLUT, 1,
		TAG_END
	);

	s_pVp = vPortCreate(0,
		TAG_VPORT_VIEW, s_pView,
		TAG_VPORT_BPP, 5,
		TAG_END
	);

	s_pBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pVp,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
		TAG_SIMPLEBUFFER_COPLIST_OFFSET, 0,
		TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
		TAG_END
	);

	UWORD pPalette[32];
	paletteLoad("data/germz.plt", pPalette, 32);
	s_pFade = fadeCreate(s_pView, pPalette, 32);

	assetsGameCreate();

	// Load settings from menu
	for(UBYTE i = 0; i < 4; ++i) {
		s_pSteers[i] = menuGetSteerForPlayer(i);
	}

	playerCreate();
	aiCreate(&g_sMap);
	s_isQuitting = 0;

	bobNewManagerCreate(s_pBfr->pFront, s_pBfr->pBack, s_pBfr->uBfrBounds.uwY);

	if(s_isEditor) {
		fadeSet(s_pFade, FADE_STATE_IN, 50, 0);
		statePush(g_pStateMachineGame, &g_sStateEditor);
	}
	else {
		// Reset scores
		for(UBYTE i = 0; i < 4; ++i) {
			s_pScores[i] = 0;
		}

		// Proceed with game
		statePush(g_pStateMachineGame, &g_sStateGameInit);
	}
	systemUnuse();
	viewLoad(s_pView);
}

static void gameGsLoop(void) {
	// If game reaches this code then init/play/summary state has popped.
	// Go to menu.
	stateChange(g_pStateMachineGame, &g_sStateMenu);
}

static void gameGsDestroy(void) {
	viewLoad(0);
	systemUse();

	assetsGameDestroy();
	playerDestroy();
	bobNewManagerDestroy();
	aiDestroy();
	fadeDestroy(s_pFade);

	viewDestroy(s_pView);
}

void gameCopyBackToFront(void) {
	// Split back->front for OCS limitations
	blitCopyAligned(s_pBfr->pBack, 0,   0, s_pBfr->pFront, 0,   0, 320, 128);
	blitCopyAligned(s_pBfr->pBack, 0, 128, s_pBfr->pFront, 0, 128, 320, 128);
	blitWait();
}

void gameInitMap(void) {
	// This is needed here because mapInitFromMapData sets charge rate
	// This codebase is a mess!
	for(UBYTE i = 0; i < 4; ++i) {
		playerFromIdx(i)->pMapData = &g_sMapData.pPlayerData[i];
	}

	// Reset map
	mapInitFromMapData();
	aiSetNodeCount();

	// Now that map is initialized, reset players with known start locations
	// First assume that all are dead, then init only those who really play
	playerAllDead();
	UBYTE ubMask = g_sMapData.ubPlayerMask;
	for(UBYTE i = 0; i < 4; ++i) {
		if(ubMask & 1) {
			playerReset(i, g_sMap.pPlayerStartNodes[i]);
			steerResetAi(&s_pSteers[i]);
		}
		ubMask >>= 1;
	}

	// Initialize team-specific stuff
	if(gameGetBattleMode()) {
		// Find team leaders
		tTeamConfig eTeamCfg = gameGetTeamConfig();
		s_pTeamLeaders[0] = playerFromIdx(PLAYER_1);
		switch(eTeamCfg) {
			case TEAM_CONFIG_P1_P2_AND_P3_P4:
				s_pTeamLeaders[1] = playerFromIdx(PLAYER_3);
				break;
			case TEAM_CONFIG_P1_P3_AND_P2_P4:
			case TEAM_CONFIG_P1_P4_AND_P2_P3:
				s_pTeamLeaders[1] = playerFromIdx(PLAYER_2);
				break;
			default:
				break;
		}

		// Assign teammates
		const tPlayerIdx *pMates = s_pPlayerTeamMates[eTeamCfg];
		for(tPlayerIdx eIdx = 0; eIdx <= PLAYER_4; ++eIdx) {
			tPlayer *pPlayer = playerFromIdx(eIdx);
			pPlayer->pTeamMate = playerFromIdx(pMates[eIdx]);
		}
	}

	// Now that players are initialized, update node counts for all of them
	mapUpdateNodeCountForPlayers();

	fadeSet(s_pFade, FADE_STATE_IN, 50, 0);
}

tBitMap *gameGetBackBuffer(void) {
	return s_pBfr->pBack;
}

tBitMap *gameGetFrontBuffer(void) {
	return s_pBfr->pFront;
}

tSteer *gameGetSteerForPlayer(UBYTE ubPlayer) {
	return &s_pSteers[ubPlayer];
}

void gameDrawTileAt(tTile eTile, UWORD uwX, UWORD uwY, UBYTE ubFrame) {
	if(tileIsNode(eTile)) {
		// Animate
		gameDrawBlobAt(eTile, ubFrame, uwX, uwY);
	}
	else if(eTile != TILE_BLANK) {
		// Don't animate
		UWORD uwTileY = 16 * (eTile - TILE_EDITOR_BLANK);
		blitCopyMask(
			g_pBmLinks, 0, uwTileY, s_pBfr->pBack, uwX, uwY,
			MAP_TILE_SIZE, MAP_TILE_SIZE, (UWORD*)g_pBmLinksMask->Planes[0]
		);
	}
}

void gameDrawMapTileAt(tUbCoordYX sPosTile, UBYTE ubFrame) {
	tTile eTile = g_sMapData.pTiles[sPosTile.ubX][sPosTile.ubY];
	gameDrawTileAt(eTile, sPosTile.ubX * MAP_TILE_SIZE, sPosTile.ubY * MAP_TILE_SIZE, ubFrame);
}

void gameDrawBlobAt(tTile eTile, UBYTE ubFrame, UWORD uwX, UWORD uwY) {
	if(s_isDebug) {
		logWrite(
			"gameDrawBlobAt(%d, %hhu, %hu, %hu), bitmap %p\n",
			eTile, ubFrame, uwX, uwY, g_pBmBlobs[eTile]
		);
	}
	tPlayerIdx ePlayer = playerIdxFromTile(eTile);
	tNodeType eNodeType = nodeTypeFromTile(eTile);
	UWORD uwSrcX;
	if(ubFrame < BLOB_FRAME_COUNT - 1) {
		uwSrcX = ubFrame * MAP_TILE_SIZE;
	}
	else {
		uwSrcX = (BLOB_FRAME_COUNT - 1 + eNodeType) * MAP_TILE_SIZE;
	}
	blitCopyMask(
		g_pBmBlobs[ePlayer], 0, uwSrcX, s_pBfr->pBack, uwX, uwY,
		MAP_TILE_SIZE, MAP_TILE_SIZE, (const UWORD*)g_pBmBlobMask->Planes[0]
	);
}

tBobNew *gameGetCursorBob(UBYTE ubIdx) {
	return &s_pCursorBobs[ubIdx];
}

void gameInitCursorBobs(void) {
	for(UBYTE ubIdx = 0; ubIdx < 4; ++ubIdx) {
		bobNewInit(
			&s_pCursorBobs[ubIdx], 16, 16, 1, g_pCursors, g_pCursorsMask, 0, 0
		);
		bobNewSetBitMapOffset(&s_pCursorBobs[ubIdx], ubIdx * 16);
	}
}

void gameSetRules(
	UBYTE isEditor, tBattleMode eBattleMode, tTeamConfig eTeamCfg,
	UBYTE isCampaign
) {
	s_isEditor = isEditor;
	s_eBattleMode = eBattleMode;
	s_eTeamCfg = eTeamCfg;
	s_ubCampaignStage = isCampaign;
}

tFade *gameGetFade(void) {
	return s_pFade;
}

tPlayer **gameGetTeamLeaders(void) {
	return s_pTeamLeaders;
}

tTeamIdx gameGetWinnerTeams(void) {
	tPlayer **pTeamLeaders = gameGetTeamLeaders();
	tTeamIdx eWinner = TEAM_NONE;
	if(pTeamLeaders[0]->isDead && pTeamLeaders[0]->pTeamMate->isDead) {
		eWinner = TEAM_2;
	}
	else if(pTeamLeaders[1]->isDead && pTeamLeaders[1]->pTeamMate->isDead) {
		eWinner = TEAM_1;
	}
	return eWinner;
}

tPlayerIdx gameGetWinnerFfa(void) {
	tPlayerIdx eWinner = PLAYER_NONE;
	for(tPlayerIdx eIdx = 0; eIdx <= PLAYER_4; ++eIdx) {
		const tPlayer *pPlayer = playerFromIdx(eIdx);
		if(!pPlayer->isDead) {
			eWinner = eIdx;
			break;
		}
	}
	return eWinner;
}

UBYTE gameIsCampaign(void) {
	return s_ubCampaignStage != 0;
}

tBattleMode gameGetBattleMode(void) {
	return s_eBattleMode;
}

tTeamConfig gameGetTeamConfig(void) {
	return s_eTeamCfg;
}

tState g_sStateGame = {
	.cbCreate = gameGsCreate, .cbLoop = gameGsLoop, .cbDestroy = gameGsDestroy
};
