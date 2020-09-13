/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "input.h"
#include <ace/managers/system.h>
#include <ace/managers/key.h>
#include "config.h"
#include "border.h"

//------------------------------------------------------------------- STATIC FNS

static void inputDrawText(const tInput *pInput, UBYTE isDrawCursor) {
	// Clear bg
	const tGuiConfig *pConfig = guiGetConfig();
	blitRect(
		pInput->pBfr, pInput->sPos.uwX + 2, pInput->sPos.uwY + 2,
		pInput->uwWidth - 4, pInput->pFont->uwHeight, 0
	);

	// Draw new text
	fontFillTextBitMap(pInput->pFont, pInput->pTextBitMap, pInput->szValue);
	if(pInput->pTextBitMap->uwActualWidth) {
		fontDrawTextBitMap(
			pInput->pBfr, pInput->pTextBitMap,
			pInput->sPos.uwX + 2, pInput->sPos.uwY + 2,
			pConfig->ubColorText, FONT_COOKIE
		);
	}

	// Draw cursor if needed
	if(isDrawCursor) {
		UWORD uwTextLength = pInput->pTextBitMap->uwActualWidth;
		fontFillTextBitMap(pInput->pFont, pInput->pTextBitMap, "_");
		fontDrawTextBitMap(
			pInput->pBfr, pInput->pTextBitMap,
			pInput->sPos.uwX + 2 + uwTextLength, pInput->sPos.uwY + 2,
			pConfig->ubColorText, FONT_COOKIE
		);
	}
}

//------------------------------------------------------------------- PUBLIC FNS

tInput *inputCreate(
	tBitMap *pBg, tFont *pFont, tTextBitMap *pTextBitMap, UWORD uwX, UWORD uwY,
	UWORD uwWidth, UWORD uwMaxChars, const char *szLabel, char *szValueBuffer
) {
	systemUse();
	tInput *pInput = memAllocFast(sizeof(*pInput));
	if(!pInput) {
		goto fail;
	}
	if(szValueBuffer) {
		pInput->szValue = szValueBuffer;
		pInput->isValueBufferAllocated = 0;
	}
	else {
		pInput->isValueBufferAllocated = 1;
		pInput->szValue = memAllocFastClear(uwMaxChars);
		if(!pInput->szValue) {
			goto fail;
		}
	}
	systemUnuse();

	pInput->sPos.uwX = uwX;
	pInput->sPos.uwY = uwY;
	pInput->uwWidth = uwWidth;
	pInput->uwMaxChars = uwMaxChars;
	pInput->uwValueLength = strlen(pInput->szValue);
	pInput->pFont = pFont;
	pInput->pBfr = pBg;

	if(pTextBitMap) {
		pInput->pTextBitMap = pTextBitMap;
		pInput->isTextBitMapAllocated = 0;
	}
	else {
		pInput->pTextBitMap = fontCreateTextBitMap(uwWidth, pFont->uwHeight);
		pInput->isTextBitMapAllocated = 1;
	}

	// Initial draw
	UWORD uwHeight = inputGetHeight(pInput);
	const tGuiConfig *pConfig = guiGetConfig();
	if(szLabel) {
		fontFillTextBitMap(pFont, pInput->pTextBitMap, szLabel);
		fontDrawTextBitMap(
			pBg, pInput->pTextBitMap,
			pInput->sPos.uwX - 1, pInput->sPos.uwY + uwHeight / 2,
			pConfig->ubColorText, FONT_COOKIE | FONT_RIGHT | FONT_VCENTER
		);
	}
	guiDraw3dBorder(pBg, pInput->sPos.uwX, pInput->sPos.uwY, uwWidth, uwHeight);
	inputDrawText(pInput, 0);

	return pInput;
fail:
	inputDestroy(pInput);
	systemUnuse();
	return 0;
}

void inputDestroy(tInput *pInput) {
	systemUse();
	if(pInput) {
		if(pInput->isValueBufferAllocated && pInput->szValue) {
			memFree(pInput->szValue, pInput->uwMaxChars);
		}
		if(pInput->isTextBitMapAllocated) {
			fontDestroyTextBitMap(pInput->pTextBitMap);
		}
		memFree(pInput, sizeof(*pInput));
	}
	systemUnuse();
}

void inputProcess(tInput *pInput) {
	UBYTE isUpdateNeeded = 0;

	// Process keystrokes
	if(keyUse(g_sKeyManager.ubLastKey)) {
		isUpdateNeeded = 1;
		WORD wInput = g_pToAscii[g_sKeyManager.ubLastKey];
		if(
			(wInput >= 'A' && wInput <= 'Z') ||
			(wInput >= 'a' && wInput <= 'z') ||
			(wInput >= '0' && wInput <= '9')
		) {
			if(pInput->uwValueLength < pInput->uwMaxChars - 1) {
				if(pInput->szValue[pInput->uwValueLength] == '\0') {
					// Move null terminator one char further if editing end of string
					pInput->szValue[pInput->uwValueLength + 1] = '\0';
				}
				pInput->szValue[pInput->uwValueLength] = wInput;
				++pInput->uwValueLength;
			}
		}
		else if(g_sKeyManager.ubLastKey == KEY_BACKSPACE && pInput->uwValueLength){
			--pInput->uwValueLength;
			pInput->szValue[pInput->uwValueLength] = '\0';
		}
		else {
			isUpdateNeeded = 0;
		}
	}

	// Draw changed letters + cursor
	if(isUpdateNeeded) {
		inputDrawText(pInput, 1);
	}
}

UBYTE inputGetHeight(const tInput *pInput) {
	UBYTE ubHeight = pInput->pFont->uwHeight + 4;
	return ubHeight;
}

void inputLoseFocus(tInput *pInput) {
	inputDrawText(pInput, 0);
}

void inputSetFocus(tInput *pInput) {
	inputDrawText(pInput, 1);
}
