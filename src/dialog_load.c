/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dialog_load.h"
#include <gui/dialog.h>
#include <gui/list_ctl.h>
#include <gui/button.h>
#include <ace/utils/dir.h>
#include <ace/managers/game.h>
#include <ace/managers/key.h>
#include "game_editor.h"
#include "dialog_save.h"
#include "game.h"
#include "assets.h"
#include "germz.h"
#include "map_list.h"

static tListCtl *s_pCtrl;
static tMapData *s_pPreview;
static const char *s_szFilePrev;
static tBitMap *s_pBmDialog;
static ULONG s_ullChangeTimer;
static UBYTE s_isMapInfoRefreshed;

static void clearMapInfo(tBitMap *pBmDialog) {
	const UWORD uwOffsX = s_pCtrl->sRect.uwWidth + 3;
	UWORD uwOffsY = 3;
	const UBYTE ubRowWidth = 100;
	const UBYTE ubRowHeight = g_pFontSmall->uwHeight + 2;

	blitRect(pBmDialog, uwOffsX, uwOffsY, ubRowWidth, ubRowHeight, 0);
	fontFillTextBitMap(g_pFontSmall, g_pTextBitmap, "Loading map...");
	fontDrawTextBitMap(pBmDialog, g_pTextBitmap, uwOffsX, uwOffsY, 19, FONT_COOKIE);

	uwOffsY += ubRowHeight;
	blitRect(pBmDialog, uwOffsX, uwOffsY, ubRowWidth, ubRowHeight, 0);

	s_isMapInfoRefreshed = 0;
}

static void updateMapInfo(tBitMap *pBmDialog) {
	const char *szFile = listCtlGetSelection(s_pCtrl);
	if(szFile == s_szFilePrev) {
		return;
	}

	char szPath[MAP_FILENAME_MAX];
	sprintf(szPath, "data/maps/%s.json", szFile);
	mapDataInitFromFile(s_pPreview, szPath);

	UWORD uwOffsX = s_pCtrl->sRect.uwWidth + 3;
	UWORD uwOffsY = 3;
	const UBYTE ubRowWidth = 100;
	const UBYTE ubRowHeight = g_pFontSmall->uwHeight + 2;

	char szLine[10 + MAX(MAP_NAME_MAX, MAP_AUTHOR_MAX)];
	blitRect(pBmDialog, uwOffsX, uwOffsY, ubRowWidth, ubRowHeight, 0);
	sprintf(szLine, "Title: %s", s_pPreview->szName);
	fontFillTextBitMap(g_pFontSmall, g_pTextBitmap, szLine);
	fontDrawTextBitMap(pBmDialog, g_pTextBitmap, uwOffsX, uwOffsY, 19, FONT_COOKIE);

	uwOffsY += ubRowHeight;
	blitRect(pBmDialog, uwOffsX, uwOffsY, ubRowWidth, ubRowHeight, 0);
	sprintf(szLine, "Author: %s", s_pPreview->szAuthor);
	fontFillTextBitMap(g_pFontSmall, g_pTextBitmap, szLine);
	fontDrawTextBitMap(pBmDialog, g_pTextBitmap, uwOffsX, uwOffsY, 19, FONT_COOKIE);
	uwOffsY += ubRowHeight;

	// Draw map
	uwOffsX += (((256 - uwOffsX) - 66) / 2);
	mapListDrawPreview(s_pPreview, pBmDialog, uwOffsX, uwOffsY);

	s_isMapInfoRefreshed = 1;
	s_szFilePrev = szFile;
}

static void dialogLoadGsCreate(void) {
	UWORD uwDlgWidth = 256;
	UWORD uwDlgHeight = 128;
	s_pBmDialog = dialogCreate(
		uwDlgWidth, uwDlgHeight, gameGetBackBuffer(), gameGetFrontBuffer()
	);
	s_pPreview = memAllocFast(sizeof(*s_pPreview));
	s_szFilePrev = 0;
	s_isMapInfoRefreshed = 0;

	// Initial draw
	UBYTE ubPad = 1;
	UWORD uwWidth = uwDlgWidth / 2;
	UWORD uwHeight = uwDlgHeight - 2;

	buttonListCreate(5, s_pBmDialog, g_pFontSmall, g_pTextBitmap);
	s_pCtrl = mapListCreateCtl(s_pBmDialog, ubPad, ubPad, uwWidth, uwHeight);
	if(!s_pCtrl) {
		statePop(g_pStateMachineGame);
	}

	updateMapInfo(s_pBmDialog);
	s_ullChangeTimer = timerGet();

	const UWORD uwBtnWidth = 50;
	const UWORD uwBtnHeight = 20;
	tButton *pButtonLoad = buttonAdd(
		uwWidth + (uwDlgWidth - uwWidth - uwBtnWidth) / 2,
		uwDlgHeight - uwBtnHeight - 10,
		uwBtnWidth, uwBtnHeight, "Load", 0, 0
	);
	listCtlDraw(s_pCtrl);
	buttonSelect(pButtonLoad);
	buttonDrawAll();
}

static void dialogLoadGsLoop(void) {
	if(!gamePreprocess()) {
		return;
	}

	UBYTE isMapSelected = 0;
	tDirection eDir = gameEditorGetSteerDir();
	if(eDir == DIRECTION_UP) {
		listCtlSelectPrev(s_pCtrl);
		clearMapInfo(s_pBmDialog);
		s_ullChangeTimer = timerGet();
	}
	else if(eDir == DIRECTION_DOWN) {
		listCtlSelectNext(s_pCtrl);
		clearMapInfo(s_pBmDialog);
		s_ullChangeTimer = timerGet();
	}
	else if(eDir == DIRECTION_FIRE || keyUse(KEY_RETURN) || keyUse(KEY_NUMENTER)) {
		// No clicking the button - code is shorter that way
		if(!s_isMapInfoRefreshed) {
			updateMapInfo(s_pBmDialog);
		}
		memcpy(&g_sMapData, s_pPreview, sizeof(g_sMapData));
		dialogSaveSetSaveName(listCtlGetSelection(s_pCtrl));
		isMapSelected = 1;
	}

	if(timerGetDelta(s_ullChangeTimer, timerGet()) >= 25) {
		updateMapInfo(s_pBmDialog);
		s_ullChangeTimer = timerGet();
	}

	// Copy dialog bitmap to screen
	dialogProcess(gameGetBackBuffer());

	gamePostprocess();

	if(isMapSelected || keyUse(KEY_ESCAPE)) {
		statePop(g_pStateMachineGame);
	}
}

static void dialogLoadGsDestroy(void) {
	memFree(s_pPreview, sizeof(*s_pPreview));
	buttonListDestroy();
	listCtlDestroy(s_pCtrl);
	dialogDestroy();
}


void dialogLoadShow(void) {
	statePush(g_pStateMachineGame, &g_sStateDialogLoad);
}

tState g_sStateDialogLoad = {
	.cbCreate = dialogLoadGsCreate, .cbLoop = dialogLoadGsLoop,
	.cbDestroy = dialogLoadGsDestroy
};
