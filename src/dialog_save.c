/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dialog_save.h"
#include <ace/managers/game.h>
#include "gui/dialog.h"
#include "gui/input.h"
#include "gui/button.h"
#include "gui/config.h"
#include "game.h"
#include "game_assets.h"

typedef enum _tSaveInput {
	SAVE_INPUT_TITLE,
	SAVE_INPUT_AUTHOR,
	SAVE_INPUT_FILENAME,
	SAVE_INPUT_COUNT
} tSaveInput;

static tBitMap *s_pBmDialog;
static tInput *s_pInputs[SAVE_INPUT_COUNT];
static tSaveInput s_eCurrentInput;
static tButton *s_pButtonSave, *s_pButtonCancel;

static char s_szFileName[30] = "";

void dialogSaveGsCreate(void) {
	s_eCurrentInput = 0;
	UWORD uwDlgWidth = 256;
	UWORD uwDlgHeight = 128;

	const tGuiConfig *pConfig = guiGetConfig();
	s_pBmDialog = dialogCreate(
		uwDlgWidth, uwDlgHeight, gameGetBackBuffer(), gameGetFrontBuffer()
	);
	buttonListCreate(2, s_pBmDialog, g_pFont, g_pTextBitmap);
	s_pInputs[SAVE_INPUT_TITLE] = inputCreate(
		s_pBmDialog, g_pFont, g_pTextBitmap, 3 + 50, 3, 100,
		sizeof(g_sMapData.szName), "Title", g_sMapData.szName
	);
	s_pInputs[SAVE_INPUT_AUTHOR] = inputCreate(
		s_pBmDialog, g_pFont, g_pTextBitmap, 3 + 50, 3+20, 100,
		sizeof(g_sMapData.szAuthor), "Author", g_sMapData.szAuthor
	);
	s_pInputs[SAVE_INPUT_FILENAME] = inputCreate(
		s_pBmDialog, g_pFont, g_pTextBitmap, 3 + 50, 3+40, 100,
		sizeof(s_szFileName), "File name", s_szFileName
	);
	fontFillTextBitMap(g_pFont, g_pTextBitmap, ".json");
	fontDrawTextBitMap(
		s_pBmDialog, g_pTextBitmap,
		s_pInputs[SAVE_INPUT_FILENAME]->sPos.uwX + s_pInputs[SAVE_INPUT_FILENAME]->uwWidth + 2,
		s_pInputs[SAVE_INPUT_FILENAME]->sPos.uwY + inputGetHeight(s_pInputs[SAVE_INPUT_FILENAME]) / 2,
		pConfig->ubColorText, FONT_COOKIE | FONT_VCENTER
	);

	UWORD uwBtnWidth = 50;
	UWORD uwBtnHeight = 20;
	s_pButtonSave = buttonAdd(
		uwDlgWidth / 3 - uwBtnWidth / 2, uwDlgHeight - uwBtnHeight - 10,
		uwBtnWidth, uwBtnHeight, "Save", 0, 0
	);
	s_pButtonCancel = buttonAdd(
		uwDlgWidth * 2 / 3 - uwBtnWidth / 2, uwDlgHeight - uwBtnHeight - 10,
		uwBtnWidth, uwBtnHeight, "Cancel", 0, 0
	);
	buttonDrawAll();
	inputSetFocus(s_pInputs[s_eCurrentInput]);
}

void dialogSaveGsLoop(void) {
	gamePreprocess();

	tSteer *pSteer = gameGetSteerForPlayer(0);
	tDirection eDir = steerProcess(pSteer);

	if(eDir == DIRECTION_UP) {
		if(s_eCurrentInput) {
			if(s_eCurrentInput == SAVE_INPUT_COUNT) {
				// Focus out of button
				buttonSelect(0);
				buttonDrawAll();
			}
			else {
				inputLoseFocus(s_pInputs[s_eCurrentInput]);
			}
			--s_eCurrentInput;
			inputSetFocus(s_pInputs[s_eCurrentInput]);
		}
	}
	else if(eDir == DIRECTION_DOWN) {
		if(s_eCurrentInput < SAVE_INPUT_COUNT) {
			inputLoseFocus(s_pInputs[s_eCurrentInput]);
			++s_eCurrentInput;
			if(s_eCurrentInput == SAVE_INPUT_COUNT) {
				buttonSelect(s_pButtonSave);
				buttonDrawAll();
			}
			else {
				inputSetFocus(s_pInputs[s_eCurrentInput]);
			}
		}
	}
	else if(eDir == DIRECTION_LEFT && s_eCurrentInput == SAVE_INPUT_COUNT) {
		buttonSelect(s_pButtonSave);
		buttonDrawAll();
	}
	else if(eDir == DIRECTION_RIGHT && s_eCurrentInput == SAVE_INPUT_COUNT) {
		buttonSelect(s_pButtonCancel);
		buttonDrawAll();
	}

	if(s_eCurrentInput < SAVE_INPUT_COUNT) {
		inputProcess(s_pInputs[s_eCurrentInput]);
	}
	dialogProcess(gameGetBackBuffer());

	gamePostprocess();

	if(eDir == DIRECTION_FIRE) {
		if(buttonGetSelected() == s_pButtonSave) {
			if(!strlen(g_sMapData.szName)) {
				// TODO: ERR
			}
			else if(!strlen(g_sMapData.szAuthor)) {
				// TODO: ERR
			}
			else if(!strlen(s_szFileName)) {
				// TODO: ERR
			}
			else {
				char szPath[45];
				strcpy(szPath, "data/maps/");
				strcat(szPath, s_szFileName);
				strcat(szPath, ".json");
				mapDataSaveToFile(&g_sMapData, szPath);
				gamePopState();
			}
		}
		else if(buttonGetSelected() == s_pButtonCancel) {
			gamePopState();
		}
	}
	else if(keyUse(KEY_ESCAPE)) {
		gamePopState();
	}
}

void dialogSaveGsDestroy(void) {
	buttonListDestroy();
	for(tSaveInput eInput = 0; eInput < SAVE_INPUT_COUNT; ++eInput) {
		inputDestroy(s_pInputs[eInput]);
	}
	dialogDestroy();
}

void dialogSaveShow(void) {
	gamePushState(dialogSaveGsCreate, dialogSaveGsLoop, dialogSaveGsDestroy);
}

void dialogSaveSetSaveName(const char *szName) {
	strcpy(s_szFileName, szName);
}
