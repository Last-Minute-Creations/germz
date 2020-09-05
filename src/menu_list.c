/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "menu_list.h"

static UBYTE s_ubActiveOption;
static UBYTE s_ubOptionCount;
tOption *s_pOptions;
const char **s_pOptionCaptions;
tBitMap *s_pBmBg, *s_pBmBuffer;
tFont *s_pFont;
tTextBitMap *s_pTextBitmap;
UBYTE s_ubColorActive, s_ubColorInactive, s_ubColorShadow;
UWORD s_uwX, s_uwY;

void menuListInit(
	tOption *pOptions, const char **pOptionCaptions,
	UBYTE ubOptionCount, tFont *pFont, tTextBitMap *pTextBitmap,
	tBitMap *pBmBg, tBitMap *pBmBuffer, UWORD uwX, UWORD uwY,
	UBYTE ubColorActive, UBYTE ubColorInactive, UBYTE ubColorShadow
) {
	s_pOptions = pOptions;
	s_ubOptionCount = ubOptionCount;
	s_ubActiveOption = 0;
	s_pFont = pFont;
	s_pOptionCaptions = pOptionCaptions;
	s_pBmBg = pBmBg;
	s_pBmBuffer = pBmBuffer;
	s_pTextBitmap = pTextBitmap;
	s_ubColorActive = ubColorActive;
	s_ubColorInactive = ubColorInactive;
	s_ubColorShadow = ubColorShadow;
	s_uwX = uwX;
	s_uwY = uwY;

	for(UBYTE ubMenuPos = 0; ubMenuPos < ubOptionCount; ++ubMenuPos) {
		s_pOptions[ubMenuPos].eDirty = MENU_LIST_DIRTY_VAL_CHANGE;
	}
}

void menuListDraw(void) {
	for(UBYTE ubMenuPos = 0; ubMenuPos < s_ubOptionCount; ++ubMenuPos) {
		if(!s_pOptions[ubMenuPos].isHidden && s_pOptions[ubMenuPos].eDirty) {
			menuListDrawPos(ubMenuPos);
			s_pOptions[ubMenuPos].eDirty = 0;
		}
	}
}

void menuListDrawPos(UBYTE ubPos) {
	UWORD uwPosY = s_uwY + ubPos * (s_pFont->uwHeight + 1);

	char szBfr[50];
	const char *szText = 0;
	if(s_pOptions[ubPos].eOptionType == MENU_LIST_OPTION_TYPE_UINT8) {
		if(s_pOptions[ubPos].sOptUb.pEnumLabels) {
			sprintf(
				szBfr, "%s: %s", s_pOptionCaptions[ubPos],
				s_pOptions[ubPos].sOptUb.pEnumLabels[*s_pOptions[ubPos].sOptUb.pVar]
			);
		}
		else {
			sprintf(
				szBfr, "%s: %hhu", s_pOptionCaptions[ubPos],
				*s_pOptions[ubPos].sOptUb.pVar
			);
		}
		szText = szBfr;
	}
	else if(s_pOptions[ubPos].eOptionType == MENU_LIST_OPTION_TYPE_CALLBACK) {
		szText = s_pOptionCaptions[ubPos];
	}
	if(szText != 0) {
		fontFillTextBitMap(s_pFont, s_pTextBitmap, szText);
		if(s_pOptions[ubPos].eDirty == MENU_LIST_DIRTY_VAL_CHANGE) {
			const UWORD uwBlitBgOffsX = s_uwX & 0xFFF0;
			const UWORD uwBlitBgWidth = 320 - uwBlitBgOffsX;
			blitCopyAligned(
				s_pBmBg, uwBlitBgOffsX, uwPosY, s_pBmBuffer, uwBlitBgOffsX, uwPosY,
				uwBlitBgWidth, s_pFont->uwHeight
			);
		}
		fontDrawTextBitMap(s_pBmBuffer, s_pTextBitmap, s_uwX, uwPosY + 1,
			s_ubColorShadow, FONT_COOKIE
		);
		fontDrawTextBitMap(s_pBmBuffer, s_pTextBitmap, s_uwX, uwPosY,
			(ubPos == s_ubActiveOption) ? s_ubColorActive : s_ubColorInactive,
			FONT_COOKIE
		);
	}
}

UBYTE menuListNavigate(BYTE bDir) {
	WORD wNewPos = s_ubActiveOption;

	// Find next non-hidden pos
	do {
		wNewPos += bDir;
	} while(0 < wNewPos && wNewPos < (WORD)s_ubOptionCount && s_pOptions[wNewPos].isHidden);

	if(wNewPos < 0 || wNewPos >= (WORD)s_ubOptionCount) {
		// Out of bounds - cancel
		return 0;
	}

	// Update active pos and mark as dirty
	s_pOptions[s_ubActiveOption].eDirty = MENU_LIST_DIRTY_SELECTION;
	s_pOptions[wNewPos].eDirty = MENU_LIST_DIRTY_SELECTION;
	s_ubActiveOption = wNewPos;
	return 1;
}

UBYTE menuListToggle(BYTE bDelta) {
	if(s_pOptions[s_ubActiveOption].eOptionType == MENU_LIST_OPTION_TYPE_UINT8) {
		WORD wNewVal = *s_pOptions[s_ubActiveOption].sOptUb.pVar + bDelta;
		if(wNewVal < 0 || wNewVal > s_pOptions[s_ubActiveOption].sOptUb.ubMax) {
			if(s_pOptions[s_ubActiveOption].sOptUb.isCyclic) {
				wNewVal = wNewVal < 0 ? s_pOptions[s_ubActiveOption].sOptUb.ubMax : 0;
			}
			else {
				return 0; // Out of bounds on non-cyclic option
			}
		}
		*s_pOptions[s_ubActiveOption].sOptUb.pVar = wNewVal;
		s_pOptions[s_ubActiveOption].eDirty = MENU_LIST_DIRTY_VAL_CHANGE;
		return 1;
	}
	return 0;
}

UBYTE menuListEnter(void) {
	if(s_pOptions[s_ubActiveOption].eOptionType == MENU_LIST_OPTION_TYPE_CALLBACK) {
		s_pOptions[s_ubActiveOption].sOptCb.cbSelect();
		return 1;
	}
	return 0;
}