/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "game_editor.h"
#include <ace/managers/game.h>
#include <ace/managers/system.h>
#include "game_assets.h"
#include "dialog_load.h"
#include "dialog_save.h"
#include "game.h"
#include "game_init.h"
#include "blob_anim.h"
#include "steer.h"

typedef struct _tEditorPlayer {
	UBYTE ubX;
	UBYTE ubY;
	UBYTE ubPaletteOption;
} tEditorPlayer;

typedef enum _tDialogResult {
	DIALOG_RESULT_NONE,
	DIALOG_RESULT_RELOAD_MAP,
	DIALOG_RESULT_EXIT,
} tDialogResult;

typedef void (*tCbFn)(void);

static tEditorPlayer s_sPlayer;
static tDialogResult s_eDialogResult;

static tTile s_pMenuTiles[] = {TILE_BLOB_P1, TILE_PATH_X1, TILE_BLANK};
static const UBYTE s_ubMenuPosCount = sizeof(s_pMenuTiles) / sizeof(s_pMenuTiles[0]);

static tBitMap *s_pBtnSmall, *s_pBtnSmallMask, *s_pBtnBig, *s_pBtnBigMask;
static tBitMap *s_pLedSmall, *s_pLedSmallMask, *s_pLedBig, *s_pLedBigMask;
static tBobNew s_sBobBtnTile, s_sBobLedTile, s_sBobBtnFn, s_sBobLedColor;
static UBYTE s_ubCurrentColor;
static UBYTE s_ubTileDrawCount, s_ubPaletteDrawCount;

static void editorDrawMapTileAt(UBYTE ubTileX, UBYTE ubTileY) {
	const UBYTE ubFrame = BLOB_FRAME_COUNT - 1;
	blitRect(
		gameGetBackBuffer(), ubTileX * MAP_TILE_SIZE, ubTileY * MAP_TILE_SIZE,
		16, 16, 0
	);
	gameDrawTileAt( // TODO: blit all tiles with mask
		TILE_EDITOR_BLANK,
		ubTileX * MAP_TILE_SIZE, ubTileY * MAP_TILE_SIZE, ubFrame
	);
	gameDrawMapTileAt(ubTileX, ubTileY, ubFrame);
}

static void setPaletteBlobColor(UBYTE ubColor) {
	s_ubCurrentColor = ubColor;
	s_pMenuTiles[0] = TILE_BLOB_P1 + ubColor;
	bobNewSetBitMapOffset(&s_sBobLedColor, 10 * ubColor);
	s_ubPaletteDrawCount = 2;
}

static void editorInitialDraw(void) {
	tBitMap *pDisplay = gameGetBackBuffer();
	blitRect(pDisplay, 0, 0, HUD_OFFS_X, 128, 0);
	blitRect(pDisplay, 0, 128, HUD_OFFS_X, 128, 0);
	bitmapLoadFromFile(pDisplay, "data/editor_hud.bm", HUD_OFFS_X, 0);

	for(UBYTE x = 0; x < MAP_SIZE; ++x) {
		for(UBYTE y = 0; y < MAP_SIZE; ++y) {
			editorDrawMapTileAt(x, y);
		}
	}

	// Schedule tile palette drawing
	s_ubTileDrawCount = 0;
	setPaletteBlobColor(0);

	gameCopyBackToFront();
}

void gameEditorGsCreate(void) {
	systemUse();

	s_pBtnSmall = bitmapCreateFromFile("data/btn_small.bm", 0);
	s_pBtnSmallMask = bitmapCreateFromFile("data/btn_small_mask.bm", 0);
	s_pBtnBig = bitmapCreateFromFile("data/btn_big.bm", 0);
	s_pBtnBigMask = bitmapCreateFromFile("data/btn_big_mask.bm", 0);
	s_pLedSmall = bitmapCreateFromFile("data/led_small.bm", 0);
	s_pLedSmallMask = bitmapCreateFromFile("data/led_small_mask.bm", 0);
	s_pLedBig = bitmapCreateFromFile("data/led_big.bm", 0);
	s_pLedBigMask = bitmapCreateFromFile("data/led_big_mask.bm", 0);

	bobNewManagerReset();
	bobNewInit(&s_sBobBtnTile, 16, 10, 1, s_pBtnSmall, s_pBtnSmallMask, 0, 0);
	bobNewInit(&s_sBobLedTile, 16, 10, 1, s_pLedSmall, s_pLedSmallMask, 0, 0);
	bobNewInit(&s_sBobBtnFn, 32, 10, 1, s_pBtnBig, s_pBtnBigMask, 0, 0);
	bobNewInit(&s_sBobLedColor, 32, 10, 1, s_pLedBig, s_pLedBigMask, 0, 0);
	gameInitCursorBobs();
	bobNewReallocateBgBuffers();

	s_sBobLedTile.sPos.uwX = HUD_OFFS_X + 26;
	s_sBobLedTile.sPos.uwY = 9;

	s_sBobLedColor.sPos.uwX = HUD_OFFS_X + 5;
	s_sBobLedColor.sPos.uwY = 148;

	s_sBobBtnTile.sPos.uwX = HUD_OFFS_X + 37;
	s_sBobBtnFn.sPos.uwX = HUD_OFFS_X + 29;

	systemUnuse();

	s_sPlayer.ubPaletteOption = 0;
	s_sPlayer.ubX = 0;
	s_sPlayer.ubY = 0;
	s_eDialogResult = DIALOG_RESULT_NONE;
	editorInitialDraw();
}

static void onChangeColor(void) {
	// Change color
	setPaletteBlobColor((s_ubCurrentColor + 1) % 5);
}

static void onReset(void) {
	mapDataClear(&g_sMapData);
	editorInitialDraw();
}

static void onTest(void) {
	mapDataRecalculateStuff(&g_sMapData);
	gameChangeState(gameInitGsCreate, gameInitGsLoop, gameInitGsDestroy);
}

static void onLoad(void) {
	dialogLoadShow();
	s_eDialogResult = DIALOG_RESULT_RELOAD_MAP;
}

static void onSave(void) {
	// TODO: Save
	// dialogShow(DIALOG_SAVE);
	dialogSaveShow();
}

static void onQuit(void) {
	// dialogShow(DIALOG_QUIT);
}

static const tCbFn s_pFnCallbacks[] = {
	onChangeColor, onReset, onTest, onSave, onLoad, onQuit
};
static const UBYTE s_ubFnBtnCount = sizeof(s_pFnCallbacks) / sizeof(s_pFnCallbacks[0]);

void gameEditorGsLoop(void) {
	if(keyUse(KEY_ESCAPE)) {
		// dialogShow(DIALOG_QUIT);
		gamePopState();
		return;
	}

	if(s_eDialogResult != DIALOG_RESULT_NONE) {
		if(s_eDialogResult == DIALOG_RESULT_RELOAD_MAP) {
			editorInitialDraw();
		}
		s_eDialogResult = DIALOG_RESULT_NONE;
		return;
	}

	UBYTE ubFnBtnPressed = s_ubFnBtnCount;
	for(UBYTE i = 0; i < s_ubFnBtnCount; ++i) {
		if(keyCheck(KEY_F1 + i)) {
			ubFnBtnPressed = i;
			break;
		}
	}
	if(ubFnBtnPressed != s_ubFnBtnCount && keyUse(KEY_F1 + ubFnBtnPressed)) {
		bobNewSetBitMapOffset(&s_sBobBtnFn, ubFnBtnPressed * 10);
		s_sBobBtnFn.sPos.uwY = 148 + ubFnBtnPressed * 14;
		s_pFnCallbacks[ubFnBtnPressed]();
		// Don't process anything else since callbacks could change game state
		return;
	}

	tSteer *pSteer = gameGetSteerForPlayer(0);
	if(!steerIsPlayer(pSteer)) {
		logWrite("ERR: P1 not set as a player\n");
		gamePopState();
		return;
	}

	if(!gamePreprocess()) {
		return;
	}

	// Process tile button press
	UBYTE ubTileBtnPressed = s_ubMenuPosCount;
	for(UBYTE i = 0; i < s_ubMenuPosCount; ++i) {
		if(keyCheck(KEY_1 + i)) {
			ubTileBtnPressed = i;
			break;
		}
	}
	if(ubTileBtnPressed != s_ubMenuPosCount) {
		s_sBobLedTile.sPos.uwY = 9 + ubTileBtnPressed * 17;
		s_sBobBtnTile.sPos.uwY = 9 + ubTileBtnPressed * 17;
		bobNewSetBitMapOffset(&s_sBobBtnTile, ubTileBtnPressed * 10);
		s_sPlayer.ubPaletteOption = ubTileBtnPressed;
	}

	if(s_ubPaletteDrawCount) {
		--s_ubPaletteDrawCount;
		for(UBYTE i = 0; i < s_ubMenuPosCount; ++i) {
			logWrite("Drawing editor palette tile %hhu\n", i);
			gameDrawTileAt(
				s_pMenuTiles[i], HUD_OFFS_X + 6, 7 + i * MAP_TILE_SIZE, BLOB_FRAME_COUNT - 1
			);
		}
	}
	else if(s_ubTileDrawCount) {
		--s_ubTileDrawCount;
		editorDrawMapTileAt(s_sPlayer.ubX, s_sPlayer.ubY);
	}
	else {
		tDirection eDir = steerProcess(pSteer);
		switch(eDir) {
			case DIRECTION_UP:
				if(s_sPlayer.ubY > 0) {
					--s_sPlayer.ubY;
				}
				break;
			case DIRECTION_DOWN:
				if(s_sPlayer.ubY < MAP_SIZE - 1) {
					++s_sPlayer.ubY;
				}
				break;
			case DIRECTION_LEFT:
				if(s_sPlayer.ubX > 0) {
					--s_sPlayer.ubX;
				}
				break;
			case DIRECTION_RIGHT:
				if(s_sPlayer.ubX < MAP_SIZE - 1) {
					++s_sPlayer.ubX;
				}
				break;
			case DIRECTION_FIRE: {
				tTile eTile = s_pMenuTiles[s_sPlayer.ubPaletteOption];
				g_sMapData.pTiles[s_sPlayer.ubX][s_sPlayer.ubY] = eTile;
				s_ubTileDrawCount = 2;
			} break;
			default:
				break;
		}
	}

	// Now that all manual blits are done draw bobs
	if(ubFnBtnPressed != s_ubFnBtnCount) {
		bobNewPush(&s_sBobBtnFn);
	}
	if(ubTileBtnPressed != s_ubMenuPosCount) {
		bobNewPush(&s_sBobBtnTile);
	}
	bobNewPush(&s_sBobLedTile);
	bobNewPush(&s_sBobLedColor);

	for(UBYTE ubPlayer = 0; ubPlayer < 4; ++ubPlayer) {
		tSteer *pSteer = gameGetSteerForPlayer(ubPlayer);
		if(!steerIsPlayer(pSteer)) {
			continue;
		}

		tBobNew *pBobCursor = gameGetCursorBob(ubPlayer);
		pBobCursor->sPos.uwX = s_sPlayer.ubX * MAP_TILE_SIZE;
		pBobCursor->sPos.uwY = s_sPlayer.ubY * MAP_TILE_SIZE;
		bobNewPush(pBobCursor);
	}
	gamePostprocess();
}

void gameEditorGsDestroy(void) {
	systemUse();
	bitmapDestroy(s_pBtnSmall);
	bitmapDestroy(s_pBtnSmallMask);
	bitmapDestroy(s_pBtnBig);
	bitmapDestroy(s_pBtnBigMask);
	bitmapDestroy(s_pLedSmall);
	bitmapDestroy(s_pLedSmallMask);
	bitmapDestroy(s_pLedBig);
	bitmapDestroy(s_pLedBigMask);
	systemUnuse();
}

// #error MAP CREATE FAIL - in editor
