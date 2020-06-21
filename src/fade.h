/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_FADE_H_
#define _GERMZ_FADE_H_

#include <ace/utils/extview.h>

typedef enum _tFadeState {
	FADE_STATE_IN,
	FADE_STATE_OUT,
	FADE_STATE_IDLE,
	FADE_STATE_EVENT_FIRED
} tFadeState;

typedef void (*tCbFadeOnDone)(void);

void fadeSetPalette(UWORD *pPalette, UBYTE ubColorCount);

void fadeSet(
	tView *pView, tFadeState eState, UBYTE ubFramesToFullFade,
	tCbFadeOnDone cbOnDone
);

tFadeState fadeProcess(void);

#endif // _GERMZ_FADE_H_
