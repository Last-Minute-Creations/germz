/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "button.h"
#include "border.h"
#include "config.h"
#include <ace/managers/log.h>

static UBYTE s_ubButtonCount;
static UBYTE s_ubMaxButtonCount;
static tButton *s_pSelected = 0;
static tButton *s_pButtons;
static tBitMap *s_pBfr;
static const tFont *s_pFont;
static tTextBitMap *s_pLabelTextBfr;
static UBYTE s_ubIsTextBfrAlloc;

// TODO: move font / textBfr to config?
// TODO: when moved to C++, pass font and textBfr as shared_ptr

void buttonListCreate(
	UBYTE ubButtonCount, tBitMap *pBfr, const tFont *pFont, tTextBitMap *pTextBfr
) {
	logBlockBegin(
		"buttonListCreate(ubButtonCount: %hhu, pBfr: %p, tFont: %p)",
		ubButtonCount, pBfr, pFont
	);
	s_ubButtonCount = 0;
	s_ubMaxButtonCount = ubButtonCount;
	s_pSelected = 0;
	s_pBfr = pBfr;
	s_pFont = pFont;
	if(pTextBfr) {
		s_pLabelTextBfr = pTextBfr;
		s_ubIsTextBfrAlloc = 0;
	}
	else {
		s_pLabelTextBfr = fontCreateTextBitMap(320, pFont->uwHeight);
		s_ubIsTextBfrAlloc = 1;
	}
	s_pButtons = memAllocFastClear(s_ubMaxButtonCount * sizeof(tButton));
	logBlockEnd("buttonListCreate()");
}

void buttonListDestroy(void) {
	logBlockBegin("buttonListDestroy()");
	memFree(s_pButtons, s_ubMaxButtonCount * sizeof(tButton));
	if(s_ubIsTextBfrAlloc) {
		fontDestroyTextBitMap(s_pLabelTextBfr);
	}
	s_ubButtonCount = 0;
	s_ubMaxButtonCount = 0;
	logBlockEnd("buttonListDestroy()");
}

tButton *buttonAdd(
	UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight,
	const char *szText, tBtnOnClick cbOnClick, void *pData
) {
	// Count check
	if(s_ubButtonCount >= s_ubMaxButtonCount) {
		return 0;
	}
	// Sanity check
	if(strlen(szText) >= BUTTON_MAX_TEXT) {
		return 0;
	}

	// Fill fields
	tButton *pButton = &s_pButtons[s_ubButtonCount];
	pButton->sRect.uwX = uwX;
	pButton->sRect.uwY = uwY;
	pButton->sRect.uwWidth = uwWidth;
	pButton->sRect.uwHeight = uwHeight;
	strcpy(pButton->szText, szText);
	pButton->onClick = cbOnClick;
	pButton->pData = pData;

	++s_ubButtonCount;
	return pButton;
}

static void buttonDraw(tButton *pButton) {
	tUwRect *pRect = &pButton->sRect;
	const tGuiConfig *pConfig = guiGetConfig();

	// Fill
	blitRect(
		s_pBfr, pRect->uwX, pRect->uwY,
		pRect->uwWidth, pRect->uwHeight, pConfig->ubColorFill
	);

	guiDraw3dBorder(
		s_pBfr, pRect->uwX, pRect->uwY, pRect->uwWidth, pRect->uwHeight
	);

	// Text
	UBYTE ubFontColor = pConfig->ubColorDark;
	UBYTE ubFontFlags = FONT_CENTER | FONT_COOKIE;
	if(pButton == s_pSelected) {
		ubFontColor = pConfig->ubColorText;
		ubFontFlags |= FONT_SHADOW;
	}
	fontFillTextBitMap(s_pFont, s_pLabelTextBfr, pButton->szText);
	fontDrawTextBitMap(
		s_pBfr, s_pLabelTextBfr,
		pRect->uwX + pRect->uwWidth/2, pRect->uwY + pRect->uwHeight/2,
		ubFontColor, ubFontFlags
	);
}

void buttonDrawAll(void) {
	for(UBYTE i = 0; i < s_ubButtonCount; ++i) {
		buttonDraw(&s_pButtons[i]);
	}
}

UBYTE buttonListProcessMouseClick(UWORD uwX, UWORD uwY) {
	for(UBYTE i = 0; i < s_ubButtonCount; ++i) {
		const tButton *pButton = &s_pButtons[i];
		if(inRect(uwX, uwY, pButton->sRect)) {
			buttonClick(pButton);
			return 1;
		}
	}
	return 0;
}

void buttonClick(const tButton *pButton) {
	if(pButton->onClick) {
		pButton->onClick(pButton->pData);
	}
}

void buttonSelect(tButton *pButton) {
	s_pSelected = pButton;
}

tButton *buttonGetSelected(void) {
	return s_pSelected;
}

void buttonSelectNext(void) {
	tButton *pNext = s_pSelected + 1;
	if(pNext != &s_pButtons[s_ubButtonCount]) {
		s_pSelected = pNext;
	}
}

void buttonSelectPrev(void) {
	if(s_pSelected != &s_pButtons[0]) {
		--s_pSelected;
	}
}
