/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "fade.h"
#include <ace/utils/palette.h>

tFade *fadeCreate(tView *pView, UWORD *pPalette, UBYTE ubColorCount) {
	logBlockBegin(
		"fadeCreate(pView: %p, pPalette: %p, ubColorCount: %hhu)",
		pView, pPalette, ubColorCount
	);
	tFade *pFade = memAllocFastClear(sizeof(*pFade));
	pFade->eState = FADE_STATE_IDLE;
	pFade->pView = pView;
	pFade->ubColorCount = ubColorCount;
	const UBYTE ubMaxColors = (
		sizeof(pFade->pPaletteRef) / sizeof(pFade->pPaletteRef[0])
	);
	if(ubColorCount > ubMaxColors) {
		logWrite(
			"ERR: Unsupported palette size: %hhu, max: %hhu",
			ubColorCount, ubMaxColors
		);
	}
	for(UBYTE i = 0; i < ubColorCount; ++i) {
		pFade->pPaletteRef[i] = pPalette[i];
	}
	logBlockEnd("fadeCreate()");
	return pFade;
}

void fadeDestroy(tFade *pFade) {
	memFree(pFade, sizeof(*pFade));
}

void fadeSet(
	tFade *pFade, tFadeState eState, UBYTE ubFramesToFullFade,
	tCbFadeOnDone cbOnDone
) {
	logBlockBegin(
		"fadeSet(pFade: %p, eState: %d, ubFramesToFullFade: %hhu, cbOnDone: %p)",
		pFade, eState, ubFramesToFullFade, cbOnDone
	);
	pFade->eState = eState;
	pFade->ubCnt = 0;
	pFade->ubCntEnd = ubFramesToFullFade;
	pFade->cbOnDone = cbOnDone;
	logBlockEnd("fadeSet()");
}

tFadeState fadeProcess(tFade *pFade) {
	if(pFade->eState != FADE_STATE_IDLE && pFade->eState != FADE_STATE_EVENT_FIRED) {
		++pFade->ubCnt;

		UBYTE ubCnt = pFade->ubCnt;
		if(pFade->eState == FADE_STATE_OUT) {
			ubCnt = pFade->ubCntEnd - pFade->ubCnt;
		}
		UBYTE ubRatio = (15 * ubCnt) / pFade->ubCntEnd;

		paletteDim(
			pFade->pPaletteRef, pFade->pView->pFirstVPort->pPalette,
			pFade->ubColorCount, ubRatio
		);
		viewUpdateCLUT(pFade->pView);

		if(pFade->ubCnt >= pFade->ubCntEnd) {
			if(pFade->cbOnDone) {
				pFade->cbOnDone();
			}
			pFade->eState = FADE_STATE_EVENT_FIRED;
		}
	}
	else {
		pFade->eState = FADE_STATE_IDLE;
	}
	return pFade->eState;
}