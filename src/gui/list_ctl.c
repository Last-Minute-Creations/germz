/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "list_ctl.h"
#include "config.h"
#include "button.h"
#include "border.h"
#include <ace/managers/log.h>
#include <ace/macros.h>

#define LISTCTL_BTN_WIDTH 10

static int onSortAsc(const void *pA, const void *pB) {
	const char *szA = *(const char **)pA;
	const char *szB = *(const char **)pB;
	return SGN(strcmp(szA, szB));
}

static void clearRect(
	tListCtl *pCtl, UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight
) {
	if(pCtl->pBg) {
		// Fill background with bg
		blitCopy(
			pCtl->pBg, uwX, uwY, pCtl->pBfr, uwX, uwY,
			uwWidth, uwHeight, MINTERM_COOKIE
		);
	}
	else {
		// Clear background
		blitRect(pCtl->pBfr, uwX, uwY, uwWidth, uwHeight, 0);
	}
}

static void listCtlDrawEntry(tListCtl *pCtl, UWORD uwIdx) {
	UWORD uwFirstVisible = 0;
	UWORD uwLastVisible = MIN(
		pCtl->uwEntryCnt,
		uwFirstVisible + (pCtl->sRect.uwHeight - 4) / pCtl->ubEntryHeight
	);
	if(uwIdx >= uwFirstVisible && uwIdx <= uwLastVisible) {
		const tGuiConfig *pCfg = guiGetConfig();
		if(uwIdx == pCtl->uwEntrySel) {
			blitRect(
				pCtl->pBfr, pCtl->sRect.uwX+2, pCtl->sRect.uwY+2 + uwIdx* pCtl->ubEntryHeight,
				pCtl->sRect.uwWidth - LISTCTL_BTN_WIDTH - 2 - 2 - 1, pCtl->ubEntryHeight,
				pCfg->ubColorFill
			);
		}
		else {
			clearRect(
				pCtl, pCtl->sRect.uwX+2, pCtl->sRect.uwY+2 + uwIdx* pCtl->ubEntryHeight,
				pCtl->sRect.uwWidth - LISTCTL_BTN_WIDTH - 2 - 2 - 1, pCtl->ubEntryHeight
			);
		}
		fontDrawStr(
			pCtl->pFont, pCtl->pBfr,
			pCtl->sRect.uwX+2+1, pCtl->sRect.uwY+2+1 + uwIdx*pCtl->ubEntryHeight,
			pCtl->pEntries[uwIdx], pCfg->ubColorText,
			FONT_LEFT | FONT_TOP | FONT_COOKIE, pCtl->pEntryTextBfr
		);
	}
}

static void listCtlSelect(tListCtl *pCtl, UWORD uwNewSelection) {
	UWORD uwPrevSel = pCtl->uwEntrySel;
	pCtl->uwEntrySel = uwNewSelection;
	listCtlDrawEntry(pCtl, uwPrevSel);
	listCtlDrawEntry(pCtl, pCtl->uwEntrySel);
	if(pCtl->cbOnSelect) {
		pCtl->cbOnSelect();
	}
}

static void onPressUp(void *pData) {
	tListCtl *pCtl = (tListCtl *)pData;
	if(pCtl->uwEntrySel) {
		listCtlSelect(pCtl, pCtl->uwEntrySel - 1);
	}
}

static void onPressDown(void *pData) {
	tListCtl *pCtl = (tListCtl *)pData;
	if(pCtl->uwEntrySel < pCtl->uwEntryCnt - 1) {
		listCtlSelect(pCtl, pCtl->uwEntrySel + 1);
	}
}

tListCtl *listCtlCreate(
	const tBitMap *pBg, tBitMap *pBfr, UWORD uwX, UWORD uwY,
	UWORD uwWidth, UWORD uwHeight, tFont *pFont, UWORD uwEntryMaxCnt,
	tTextBitMap *pTextBfr, tCbListCtlOnSelect cbOnSelect
) {
	logBlockBegin(
		"listCtlCreate(uwX: %hu, uwY: %hu, uwWidth: %hu, uwHeight: %hu, pFont: %p)",
		uwX, uwY, uwWidth, uwHeight, pFont
	);

	tListCtl *pCtl = memAllocFast(sizeof(tListCtl));
	pCtl->ubEntryHeight = pFont->uwHeight + 2;
	pCtl->uwEntryCnt = 0;
	pCtl->uwEntryMaxCnt = uwEntryMaxCnt;
	pCtl->sRect.uwX = uwX;
	pCtl->sRect.uwY = uwY;
	pCtl->sRect.uwWidth = uwWidth;
	pCtl->sRect.uwHeight = uwHeight;
	pCtl->ubDrawState = LISTCTL_DRAWSTATE_NEEDS_REDRAW;
	pCtl->pFont = pFont;
	pCtl->pBg = pBg;
	pCtl->pBfr = pBfr;
	pCtl->uwEntrySel = 0;
	pCtl->cbOnSelect = cbOnSelect;

	pCtl->pEntries =  memAllocFastClear(uwEntryMaxCnt * sizeof(char*));

	if(pTextBfr) {
		pCtl->pEntryTextBfr = pTextBfr;
		pCtl->isTextBfrAlloc = 0;
	}
	else {
		pCtl->pEntryTextBfr = fontCreateTextBitMap(uwWidth, pFont->uwHeight);
		pCtl->isTextBfrAlloc = 1;
	}

	buttonAdd(
		uwX + uwWidth - LISTCTL_BTN_WIDTH-2, uwY+2,
		LISTCTL_BTN_WIDTH, LISTCTL_BTN_WIDTH, "U", onPressUp, pCtl
	);
	buttonAdd(
		uwX + uwWidth - LISTCTL_BTN_WIDTH-2, uwY + uwHeight - LISTCTL_BTN_WIDTH -2,
		LISTCTL_BTN_WIDTH, LISTCTL_BTN_WIDTH, "D", onPressDown, pCtl
	);

	logBlockEnd("listCtlCreate()");
	return pCtl;
}

void listCtlDestroy(tListCtl *pCtl) {
	for(UWORD i = pCtl->uwEntryCnt; i--;) {
		listCtlRemoveEntry(pCtl, i);
	}
	memFree(pCtl->pEntries, pCtl->uwEntryMaxCnt * sizeof(char*));
	if(pCtl->isTextBfrAlloc) {
		fontDestroyTextBitMap(pCtl->pEntryTextBfr);
	}
	memFree(pCtl, sizeof(tListCtl));
}

UWORD listCtlAddEntry(tListCtl *pCtl, const char *szTxt) {
	if(pCtl->uwEntryCnt >= pCtl->uwEntryMaxCnt) {
		return LISTCTL_ENTRY_INVALID;
	}
	pCtl->pEntries[pCtl->uwEntryCnt] = memAllocFast(strlen(szTxt)+1);
	strcpy(pCtl->pEntries[pCtl->uwEntryCnt], szTxt);
	pCtl->ubDrawState = LISTCTL_DRAWSTATE_NEEDS_REDRAW;
	return pCtl->uwEntryCnt++;
}

void listCtlRemoveEntry(tListCtl *pCtl, UWORD uwIdx) {
	if(pCtl->pEntries[uwIdx])
		memFree(pCtl->pEntries[uwIdx], strlen(pCtl->pEntries[uwIdx])+1);
}

void listCtlUndraw(tListCtl *pCtl) {
	clearRect(
		pCtl, pCtl->sRect.uwX, pCtl->sRect.uwY,
		pCtl->sRect.uwWidth, pCtl->sRect.uwHeight
	);
}

void listCtlDraw(tListCtl *pCtl) {
	listCtlUndraw(pCtl);

	const tGuiConfig *pCfg = guiGetConfig();
	if(pCfg->eFill == FILL_STYLE_3D) {
		guiDraw3dBorder(
			pCtl->pBfr, pCtl->sRect.uwX, pCtl->sRect.uwY,
			pCtl->sRect.uwWidth, pCtl->sRect.uwHeight
		);
	}

	// Draw scroll bar
	blitRect(
		pCtl->pBfr,
		pCtl->sRect.uwX + pCtl->sRect.uwWidth - LISTCTL_BTN_WIDTH - 2,
		pCtl->sRect.uwY + LISTCTL_BTN_WIDTH + 3,
		LISTCTL_BTN_WIDTH, pCtl->sRect.uwHeight - 2*LISTCTL_BTN_WIDTH - 6,
		pCfg->ubColorDark
	);

	UWORD uwFirstVisible = 0;
	UWORD uwLastVisible = MIN(
		pCtl->uwEntryCnt,
		uwFirstVisible + (pCtl->sRect.uwHeight - 4) / pCtl->ubEntryHeight
	);

	// Draw elements
	for(UWORD i = uwFirstVisible; i != uwLastVisible; ++i) {
		listCtlDrawEntry(pCtl, i);
	}
}

UBYTE listCtlProcessClick(tListCtl *pCtl, UWORD uwMouseX, UWORD uwMouseY) {
	if(inRect(uwMouseX, uwMouseY, pCtl->sRect)) {
		UWORD uwFirstVisible = 0;
		UWORD uwNewSelection = MIN(
			uwFirstVisible + (uwMouseY - pCtl->sRect.uwX - 2) / pCtl->ubEntryHeight,
			pCtl->uwEntryCnt-1
		);
		listCtlSelect(pCtl, uwNewSelection);
		return 1;
	}
	return 0;
}

void listCtlSelectPrev(tListCtl *pCtl) {
	onPressUp(pCtl);
}

void listCtlSelectNext(tListCtl *pCtl) {
	onPressDown(pCtl);
}

void listCtlSortEntries(tListCtl *pCtl) {
	// qsort(pCtl->pEntries, pCtl->uwEntryCnt, sizeof(pCtl->pEntries[0]), onSortAsc);
}

const char *listCtlGetSelection(const tListCtl *pCtl) {
	return pCtl->pEntries[pCtl->uwEntrySel];
}
