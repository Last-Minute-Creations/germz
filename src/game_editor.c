/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "game_editor.h"
#include <ace/managers/game.h>
#include "game_assets.h"
#include "game.h"
#include "game_init.h"
#include "blob_anim.h"
#include "steer.h"

typedef struct _tEditorPlayer {
	UBYTE ubX;
	UBYTE ubY;
	tTile eTile;
	UBYTE ubDrawCount;
	UBYTE isInPalette;
} tEditorPlayer;

typedef enum _tEditorAction {
	EDITOR_ACTION_NONE,
	EDITOR_ACTION_NEW,
	EDITOR_ACTION_TEST,
	EDITOR_ACTION_SAVE,
	EDITOR_ACTION_LOAD,
	EDITOR_ACTION_EXIT,
	EDITOR_ACTION_COUNT
} _tEditorAction;

tEditorPlayer s_pPlayers[4];
static UBYTE s_eAction;

static void editorInitialDraw(void) {
	tBitMap *pDisplay = gameGetBackBuffer();
	blitRect(pDisplay, 0, 0, 320, 128, 0);
	blitRect(pDisplay, 0, 128, 320, 128, 0);

	for(UBYTE x = 0; x < MAP_SIZE; ++x) {
		for(UBYTE y = 0; y < MAP_SIZE; ++y) {
			gameDrawMapTileAt(x, y, BLOB_FRAME_COUNT - 1);
		}
	}

	for(UBYTE i = 0; i < 4; ++i) {
		s_pPlayers[i].ubDrawCount = 0;
	}

	for(tTile eTile = 0; eTile < TILE_COUNT; ++eTile) {
		gameDrawTileAt(
			eTile, MAP_SIZE + 1 + (eTile & 1), eTile / 2, BLOB_FRAME_COUNT - 1
		);
	}
	s_eAction = EDITOR_ACTION_NONE;

	bobNewDiscardUndraw();
	gameCopyBackToFront();
}

static void editorReset(void) {
	mapDataClear(&g_sMapData);
	editorInitialDraw();
}

void gameEditorGsCreate(void) {
	for(UBYTE i = 0; i < 4; ++i) {
		s_pPlayers[i].ubX = 0;
		s_pPlayers[i].ubY = 0;
		s_pPlayers[i].eTile = TILE_BLOB_NEUTRAL;
		s_pPlayers[i].isInPalette = 0;
	}
	editorInitialDraw();
}

void gameEditorGsLoop(void) {
	switch(s_eAction) {
		case EDITOR_ACTION_EXIT:
			gamePopState();
			return;
		case EDITOR_ACTION_NEW:
			editorReset();
			break;
		case EDITOR_ACTION_SAVE:
			break;
		case EDITOR_ACTION_LOAD:
			break;
		case EDITOR_ACTION_TEST:
			mapDataRecalculateStuff(&g_sMapData);
			gameChangeState(gameInitGsCreate, gameInitGsLoop, gameInitGsDestroy);
			return;
		default:
			break;
	}
	s_eAction = EDITOR_ACTION_NONE;

	if(!gamePreprocess()) {
		return;
	}
	for(UBYTE ubPlayer = 0; ubPlayer < 4; ++ubPlayer ) {
		tEditorPlayer *pPlayer = &s_pPlayers[ubPlayer];
		tSteer *pSteer = gameGetSteerForPlayer(ubPlayer);
		if(!steerIsPlayer(pSteer)) {
			continue;
		}

		UBYTE ubMaxX = MAP_SIZE;
		UBYTE ubMaxY = MAP_SIZE;
		if(pPlayer->isInPalette) {
				ubMaxX = 2;
				ubMaxY = TILE_COUNT / 2;
		}
		if(pPlayer->ubDrawCount) {
			--pPlayer->ubDrawCount;
			gameDrawMapTileAt(pPlayer->ubX, pPlayer->ubY, BLOB_FRAME_COUNT - 1);
		}
		else {
			tDir eDir = steerProcess(pSteer);
			switch(eDir) {
				case DIR_UP:
					if(pPlayer->ubY > 0) {
						--pPlayer->ubY;
					}
					break;
				case DIR_DOWN:
					if(pPlayer->ubY < ubMaxY - 1) {
						++pPlayer->ubY;
					}
					break;
				case DIR_LEFT:
					if(pPlayer->ubX > 0) {
						--pPlayer->ubX;
					}
					else if(pPlayer->isInPalette) {
						pPlayer->isInPalette = 0;
						pPlayer->ubX = MAP_SIZE - 1;
					}
					break;
				case DIR_RIGHT:
					if(pPlayer->ubX < ubMaxX - 1) {
						++pPlayer->ubX;
					}
					else if(!pPlayer->isInPalette) {
						pPlayer->isInPalette = 1;
						pPlayer->ubX = 0;
						pPlayer->ubY = MIN(pPlayer->ubY, (TILE_COUNT / 2) - 1);
					}
					break;
				case DIR_FIRE:
					if(pPlayer->isInPalette) {
						tTile eTile = pPlayer->ubX + pPlayer->ubY * 2;
						switch(eTile) {
							case TILE_EDITOR_EXIT:
								s_eAction = EDITOR_ACTION_EXIT;
								break;
							case TILE_EDITOR_NEW:
								s_eAction = EDITOR_ACTION_NEW;
								break;
							case TILE_EDITOR_SAVE:
								break;
							case TILE_EDITOR_LOAD:
								break;
							case TILE_EDITOR_TEST:
								s_eAction = EDITOR_ACTION_TEST;
								break;
							default:
								pPlayer->eTile = eTile;
								break;
						}
					}
					else {
						g_sMapData.pTiles[pPlayer->ubX][pPlayer->ubY] = pPlayer->eTile;
						pPlayer->ubDrawCount = 2;
					}
					break;
				default:
					break;
			}
		}
	}

	// Now that all manual blits are done draw cursor bobs
	for(UBYTE ubPlayer = 0; ubPlayer < 4; ++ubPlayer) {
		tSteer *pSteer = gameGetSteerForPlayer(ubPlayer);
		if(!steerIsPlayer(pSteer)) {
			continue;
		}

		tEditorPlayer *pPlayer = &s_pPlayers[ubPlayer];
		tBobNew *pBobCursor = gameGetCursorBob(ubPlayer);
		pBobCursor->sPos.uwX = pPlayer->ubX * MAP_TILE_SIZE;
		pBobCursor->sPos.uwY = pPlayer->ubY * MAP_TILE_SIZE;
		if(pPlayer->isInPalette) {
			pBobCursor->sPos.uwX += (MAP_SIZE + 1) * MAP_TILE_SIZE;
		}
		bobNewPush(pBobCursor);
	}
	gamePostprocess();
}

void gameEditorGsDestroy(void) {

}

// #error MAP CREATE FAIL - in editor
