/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "dialog_save.h"
#include <ace/managers/game.h>
#include <ace/managers/key.h>
#include <ace/managers/state.h>
#include "gui/dialog.h"
#include "gui/input.h"
#include "gui/button.h"
#include "gui/config.h"
#include "game_editor.h"
#include "game.h"
#include "assets.h"
#include "germz.h"

#define FILE_PATH_PREFIX "data/maps/"
#define FILE_PATH_EXTENSION ".json"
#define FILE_PATH_SIZE ( \
	sizeof(FILE_PATH_PREFIX) - 1 + sizeof(g_sMapData.szName) + \
	sizeof(FILE_PATH_EXTENSION) - 1 + 1 \
)

typedef enum _tSaveInput {
	SAVE_INPUT_TITLE,
	SAVE_INPUT_AUTHOR,
	SAVE_INPUT_COUNT
} tSaveInput;

static tBitMap *s_pBmDialog;
static tInput *s_pInputs[SAVE_INPUT_COUNT];
static tSaveInput s_eCurrentInput;
static tButton *s_pButtonSave, *s_pButtonCancel;
static tStateManager *s_pDlgStateMachine;

static char s_szFilePath[FILE_PATH_SIZE];

static tState s_sStateOverwrite, s_sStateSelect, s_sStateSaving;

//----------------------------------------------------------------- STATE SELECT

void dialogSaveSelectCreate(void) {
	dialogClear();

	UBYTE ubPadX = 3;
	UBYTE ubPadY = 10;

	buttonListCreate(2, s_pBmDialog, g_pFontSmall, g_pTextBitmap);
	s_pInputs[SAVE_INPUT_TITLE] = inputCreate(
		s_pBmDialog, g_pFontSmall, g_pTextBitmap, ubPadX + 50, ubPadY, 100,
		sizeof(g_sMapData.szName), "Title", g_sMapData.szName
	);
	s_pInputs[SAVE_INPUT_AUTHOR] = inputCreate(
		s_pBmDialog, g_pFontSmall, g_pTextBitmap, ubPadX + 50, ubPadY + 20, 100,
		sizeof(g_sMapData.szAuthor), "Author", g_sMapData.szAuthor
	);

	s_eCurrentInput = 0;
	UWORD uwBtnWidth = 50;
	UWORD uwBtnHeight = 20;
	UWORD uwDlgWidth = bitmapGetByteWidth(s_pBmDialog) * 8;
	UWORD uwDlgHeight = s_pBmDialog->Rows;
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

void dialogSaveSelectLoop(void) {
	UBYTE isTab = keyUse(KEY_TAB);
	tDirection eDir = gameEditorGetSteerDir();
	if(eDir == DIRECTION_UP || (isTab && keyCheck(KEY_LSHIFT))) {
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
	else if(eDir == DIRECTION_DOWN || (isTab && !keyCheck(KEY_LSHIFT))) {
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

	if(eDir == DIRECTION_FIRE || keyUse(KEY_RETURN) || keyUse(KEY_NUMENTER)) {
		if(buttonGetSelected() == s_pButtonSave) {
			if(!strlen(g_sMapData.szName)) {
				// TODO: ERR
			}
			else if(!strlen(g_sMapData.szAuthor)) {
				// TODO: ERR
			}
			else {
				strcpy(s_szFilePath, FILE_PATH_PREFIX);
				strcat(s_szFilePath, g_sMapData.szName);
				strcat(s_szFilePath, FILE_PATH_EXTENSION);
				if(fileExists(s_szFilePath)) {
					stateChange(s_pDlgStateMachine, &s_sStateOverwrite);
				}
				else {
					stateChange(s_pDlgStateMachine, &s_sStateSaving);
				}
			}
		}
		else if(buttonGetSelected() == s_pButtonCancel) {
			statePop(g_pStateMachineGame);
		}
	}
	else if(keyUse(KEY_ESCAPE)) {
		statePop(g_pStateMachineGame);
	}
}

void dialogSaveSelectDestroy(void) {
	buttonListDestroy();
	for(tSaveInput eInput = 0; eInput < SAVE_INPUT_COUNT; ++eInput) {
		inputDestroy(s_pInputs[eInput]);
	}
}

static tState s_sStateSelect = {
	.cbCreate = dialogSaveSelectCreate, .cbLoop = dialogSaveSelectLoop,
	.cbDestroy = dialogSaveSelectDestroy
};

//-------------------------------------------------------------- STATE OVERWRITE

void dialogSaveOverwriteCreate(void) {
	dialogClear();
	buttonListCreate(2, s_pBmDialog, g_pFontSmall, g_pTextBitmap);
	const tGuiConfig *pConfig = guiGetConfig();

	UWORD uwDlgWidth = bitmapGetByteWidth(s_pBmDialog) * 8;
	UWORD uwDlgHeight = s_pBmDialog->Rows;
	UWORD uwBtnWidth = 50;
	UWORD uwBtnHeight = 20;
	UWORD uwBtnY = uwDlgHeight - uwBtnHeight - 10;

	fontFillTextBitMap(g_pFontSmall, g_pTextBitmap, "File already exists");
	fontDrawTextBitMap(
		s_pBmDialog, g_pTextBitmap, uwDlgWidth / 2,
		uwBtnY / 2 - g_pFontSmall->uwHeight,
		pConfig->ubColorText, FONT_LAZY | FONT_CENTER
	);

	fontFillTextBitMap(g_pFontSmall, g_pTextBitmap, s_szFilePath);
	fontDrawTextBitMap(
		s_pBmDialog, g_pTextBitmap, uwDlgWidth / 2, uwBtnY / 2,
		pConfig->ubColorText, FONT_LAZY | FONT_CENTER
	);

	fontFillTextBitMap(g_pFontSmall, g_pTextBitmap, "Do you want to overwrite?");
	fontDrawTextBitMap(
		s_pBmDialog, g_pTextBitmap, uwDlgWidth / 2,
		uwBtnY / 2 + g_pFontSmall->uwHeight,
		pConfig->ubColorText, FONT_LAZY | FONT_CENTER
	);

	s_pButtonSave = buttonAdd(
		uwDlgWidth / 3 - uwBtnWidth / 2, uwDlgHeight - uwBtnHeight - 10,
		uwBtnWidth, uwBtnHeight, "Yes", 0, 0
	);
	s_pButtonCancel = buttonAdd(
		uwDlgWidth * 2 / 3 - uwBtnWidth / 2, uwDlgHeight - uwBtnHeight - 10,
		uwBtnWidth, uwBtnHeight, "No", 0, 0
	);
	buttonSelect(s_pButtonCancel);
	buttonDrawAll();
}

void dialogSaveOverwriteLoop(void) {
	tDirection eDir = gameEditorGetSteerDir();
	if(eDir == DIRECTION_LEFT && s_eCurrentInput == SAVE_INPUT_COUNT) {
		buttonSelect(s_pButtonSave);
		buttonDrawAll();
	}
	else if(eDir == DIRECTION_RIGHT && s_eCurrentInput == SAVE_INPUT_COUNT) {
		buttonSelect(s_pButtonCancel);
		buttonDrawAll();
	}

	dialogProcess(gameGetBackBuffer());
	gamePostprocess();

	if(eDir == DIRECTION_FIRE || keyUse(KEY_RETURN) || keyUse(KEY_NUMENTER)) {
		if(buttonGetSelected() == s_pButtonSave) {
			stateChange(s_pDlgStateMachine, &s_sStateSaving);
		}
		else if(buttonGetSelected() == s_pButtonCancel) {
			stateChange(s_pDlgStateMachine, &s_sStateSelect);
		}
	}
	else if(keyUse(KEY_ESCAPE)) {
		stateChange(s_pDlgStateMachine, &s_sStateSelect);
	}
}

void dialogSaveOverwriteDestroy(void) {
	buttonListDestroy();
}

static tState s_sStateOverwrite = {
	.cbCreate = dialogSaveOverwriteCreate, .cbLoop = dialogSaveOverwriteLoop,
	.cbDestroy = dialogSaveOverwriteDestroy
};

//----------------------------------------------------------------- STATE SAVING

static void dialogSaveSavingCreate(void) {
	dialogClear();
	const tGuiConfig *pConfig = guiGetConfig();

	fontFillTextBitMap(
		g_pFontSmall, g_pTextBitmap, "Saving map. Don't turn off the power..."
	);
	fontDrawTextBitMap(
		s_pBmDialog, g_pTextBitmap,
		(bitmapGetByteWidth(s_pBmDialog) * 8) / 2, s_pBmDialog->Rows / 2,
		pConfig->ubColorText, FONT_LAZY | FONT_CENTER
	);
}

void dialogSaveSavingLoop(void) {
	dialogProcess(gameGetBackBuffer());
	gamePostprocess();

	mapDataSaveToFile(&g_sMapData, s_szFilePath);
	statePop(g_pStateMachineGame);
}

static tState s_sStateSaving = {
	.cbCreate = dialogSaveSavingCreate, .cbLoop = dialogSaveSavingLoop
};

//----------------------------------------------------------------------- DIALOG

static void dialogSaveGsCreate(void) {
	s_eCurrentInput = 0;
	UWORD uwDlgWidth = 256;
	UWORD uwDlgHeight = 128;

	s_pBmDialog = dialogCreate(
		uwDlgWidth, uwDlgHeight, gameGetBackBuffer(), gameGetFrontBuffer()
	);

	s_pDlgStateMachine = stateManagerCreate();
	statePush(s_pDlgStateMachine, &s_sStateSelect);
}

static void dialogSaveGsLoop(void) {
	if(!gamePreprocess()) {
		return;
	}
	stateProcess(s_pDlgStateMachine);
}

static void dialogSaveGsDestroy(void) {
	stateManagerDestroy(s_pDlgStateMachine);
	dialogDestroy();
}

void dialogSaveShow(void) {
	statePush(g_pStateMachineGame, &g_sStateDialogSave);
}

tState g_sStateDialogSave = {
	.cbCreate = dialogSaveGsCreate, .cbLoop = dialogSaveGsLoop,
	.cbDestroy = dialogSaveGsDestroy
};
