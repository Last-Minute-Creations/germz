/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "fade.h"
#include <ace/utils/palette.h>

static tFadeState s_eFadeState;
static UBYTE s_ubColorCount;
static UBYTE s_ubFadeCnt;
static UBYTE s_ubFadeCntEnd;
static UWORD s_pPaletteRef[32];
static tCbFadeOnDone s_cbOnDone;
static tView *s_pView;

void fadeSetPalette(UWORD *pPalette, UBYTE ubColorCount) {
	for(UBYTE i = 0; i < ubColorCount; ++i) {
		s_pPaletteRef[i] = pPalette[i];
	}
	s_ubColorCount = ubColorCount;
}

void fadeSet(
	tView *pView, tFadeState eState, UBYTE ubFramesToFullFade,
	tCbFadeOnDone cbOnDone
) {
	s_pView = pView;
	s_eFadeState = eState;
	s_ubFadeCnt = 0;
	s_ubFadeCntEnd = ubFramesToFullFade;
	s_cbOnDone = cbOnDone;
}

tFadeState fadeProcess(void) {
	if(s_eFadeState != FADE_STATE_IDLE && s_eFadeState != FADE_STATE_EVENT_FIRED) {
		++s_ubFadeCnt;

		UBYTE ubCnt = s_ubFadeCnt;
		if(s_eFadeState == FADE_STATE_OUT) {
			ubCnt = s_ubFadeCntEnd - s_ubFadeCnt;
		}
		UBYTE ubRatio = (15 * ubCnt) / s_ubFadeCntEnd;

		paletteDim(
			s_pPaletteRef, s_pView->pFirstVPort->pPalette, s_ubColorCount, ubRatio
		);
		viewUpdateCLUT(s_pView);

		if(s_ubFadeCnt >= s_ubFadeCntEnd) {
			if(s_cbOnDone) {
				s_cbOnDone();
			}
			s_eFadeState = FADE_STATE_EVENT_FIRED;
		}
	}
	else {
		s_eFadeState = FADE_STATE_IDLE;
	}
	return s_eFadeState;
}
