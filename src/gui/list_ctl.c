/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "list_ctl.h"
#include <stdlib.h>
#include "config.h"
#include "border.h"
#include <ace/managers/log.h>
#include <ace/macros.h>

#define LISTCTL_BTN_WIDTH 10

static int onSortAsc(const void *pA, const void *pB) {
	const char *szA = *(const char **)pA;
	const char *szB = *(const char **)pB;
	return SGN(strcmp(szA, szB));
}

static UBYTE listCtlGetMaxVisibleEntries(const tListCtl *pCtl) {
	return (pCtl->sRect.uwHeight - 4) / pCtl->ubEntryHeight;
}

static void listCtlDrawEntry(tListCtl *pCtl, UWORD uwIdx) {
	UWORD uwFirstVisible = pCtl->uwEntryScrollPos;
	UWORD uwLastVisible = MIN(
		pCtl->uwEntryCnt, uwFirstVisible + listCtlGetMaxVisibleEntries(pCtl)
	);
	if(uwIdx >= uwFirstVisible && uwIdx <= uwLastVisible) {
		UBYTE ubPosOnView = uwIdx - pCtl->uwEntryScrollPos;
		UBYTE isSelected = (uwIdx == pCtl->uwEntrySel);
		if(pCtl->cbDrawPos) {
			pCtl->cbDrawPos(pCtl, uwIdx, ubPosOnView, isSelected);
		}
	}
}

static void listCtlDrawAllEntries(tListCtl *pCtl) {
	const tGuiConfig *pCfg = guiGetConfig();

	// Draw scroll bar
	UBYTE ubScrollBarHeight = pCtl->sRect.uwHeight - 2 * LISTCTL_BTN_WIDTH - 6;
	UBYTE ubBeadHeight = MIN(
		ubScrollBarHeight,
		ubScrollBarHeight * listCtlGetMaxVisibleEntries(pCtl) / pCtl->uwEntryCnt
	);
	UBYTE ubBeadStart = CLAMP(
		ubScrollBarHeight * pCtl->uwEntryScrollPos / pCtl->uwEntryCnt,
		0, ubScrollBarHeight - ubBeadHeight
	);
	UWORD uwScrollX = pCtl->sRect.uwX + pCtl->sRect.uwWidth - LISTCTL_BTN_WIDTH - 2;
	UWORD uwScrollY =  pCtl->sRect.uwY + LISTCTL_BTN_WIDTH + 3;
	if(pCtl->cbUndraw) {
		pCtl->cbUndraw(uwScrollX, uwScrollY, LISTCTL_BTN_WIDTH, ubScrollBarHeight);
	}
	blitRect(
		pCtl->pBfr, uwScrollX, uwScrollY + ubBeadStart, LISTCTL_BTN_WIDTH, ubBeadHeight,
		pCfg->ubColorDark
	);

	// Draw elements
	UWORD uwFirstVisible = pCtl->uwEntryScrollPos;
	UWORD uwLastVisible = MIN(
		pCtl->uwEntryCnt, uwFirstVisible + listCtlGetMaxVisibleEntries(pCtl)
	);
	for(UWORD i = uwFirstVisible; i != uwLastVisible; ++i) {
		listCtlDrawEntry(pCtl, i);
	}
}

static void listCtlSelect(tListCtl *pCtl, UWORD uwNewSelection) {
	UWORD uwPrevSel = pCtl->uwEntrySel;
	UWORD uwPrevScroll = pCtl->uwEntryScrollPos;
	listCtlSetSelectionIdx(pCtl, uwNewSelection);

	if(pCtl->uwEntryScrollPos != uwPrevScroll) {
		listCtlDrawAllEntries(pCtl);
	}
	else {
		listCtlDrawEntry(pCtl, uwPrevSel);
		listCtlDrawEntry(pCtl, pCtl->uwEntrySel);
		if(pCtl->cbOnSelect) {
			pCtl->cbOnSelect();
		}
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
	tBitMap *pBfr, UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight,
	tFont *pFont, UWORD uwEntryMaxCnt, tTextBitMap *pTextBfr,
	tGuiListCtlCbOnSelect cbOnSelect, tGuiListCtlCbUndraw cbUndraw,
	tGuiListCtlCbDrawPos cbDrawPos
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
	pCtl->pBfr = pBfr;
	pCtl->uwEntrySel = 0;
	pCtl->uwEntryScrollPos = 0;
	pCtl->cbOnSelect = cbOnSelect;
	pCtl->cbUndraw = cbUndraw;
	pCtl->cbDrawPos = cbDrawPos;

	pCtl->pEntries =  memAllocFastClear(uwEntryMaxCnt * sizeof(char*));

	if(pTextBfr) {
		pCtl->pEntryTextBfr = pTextBfr;
		pCtl->isTextBfrAlloc = 0;
	}
	else {
		pCtl->pEntryTextBfr = fontCreateTextBitMap(uwWidth, pFont->uwHeight);
		pCtl->isTextBfrAlloc = 1;
	}

	pCtl->pButtonUp = buttonAdd(
		uwX + uwWidth - LISTCTL_BTN_WIDTH-2, uwY+2,
		LISTCTL_BTN_WIDTH, LISTCTL_BTN_WIDTH, "U", onPressUp, pCtl
	);
	pCtl->pButtonDown = buttonAdd(
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
	if(pCtl->cbUndraw) {
		pCtl->cbUndraw(
			pCtl->sRect.uwX, pCtl->sRect.uwY, pCtl->sRect.uwWidth, pCtl->sRect.uwHeight
		);
	}
}

void listCtlDraw(tListCtl *pCtl) {
	listCtlUndraw(pCtl);

	// TODO: move to callback
	// guiDraw3dBorder(
	// 	pCtl->pBfr, pCtl->sRect.uwX, pCtl->sRect.uwY,
	// 	pCtl->sRect.uwWidth, pCtl->sRect.uwHeight
	// );

	listCtlDrawAllEntries(pCtl);
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

UBYTE listCtlSelectPrev(tListCtl *pCtl) {
	if(pCtl->uwEntrySel) {
		onPressUp(pCtl);
		return 1;
	}
	return 0;
}

UBYTE listCtlSelectNext(tListCtl *pCtl) {
	if(pCtl->uwEntrySel < pCtl->uwEntryCnt - 1) {
		onPressDown(pCtl);
		return 1;
	}
	return 0;
}

void listCtlSortEntries(tListCtl *pCtl) {
	qsort(pCtl->pEntries, pCtl->uwEntryCnt, sizeof(pCtl->pEntries[0]), onSortAsc);
}

const char *listCtlGetSelection(const tListCtl *pCtl) {
	return pCtl->pEntries[pCtl->uwEntrySel];
}

void listCtlSetSelectionIdx(tListCtl *pCtl, UWORD uwIdx) {
	pCtl->uwEntrySel = uwIdx;
	UBYTE ubMaxPerScreen = listCtlGetMaxVisibleEntries(pCtl);

	if(uwIdx < pCtl->uwEntryScrollPos) {
		// New pos is above current display window
		pCtl->uwEntryScrollPos = uwIdx;
	}
	else if(uwIdx > pCtl->uwEntryScrollPos + ubMaxPerScreen - 1) {
		// New pos is below current display window
		pCtl->uwEntryScrollPos = MAX(
			0, uwIdx - (ubMaxPerScreen - 1)
		);
	}
}
