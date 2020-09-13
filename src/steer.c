/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "steer.h"
#include "player.h"
#include <ace/managers/joy.h>
#include <ace/managers/key.h>

//------------------------------------------------------------------ PRIVATE FNS

static tDirection onJoy(tSteer *pSteer) {
	UBYTE isRepeat = (timerGetDelta(pSteer->ulTimer, timerGet()) > 10);
	UBYTE ubJoy = pSteer->ubJoy;
	tDirection eDir = DIRECTION_COUNT;
	if(joyUse(ubJoy + JOY_LEFT) || (isRepeat && joyCheck(ubJoy + JOY_LEFT))) {
		eDir = DIRECTION_LEFT;
		pSteer->ulTimer = timerGet();
	}
	else if(joyUse(ubJoy + JOY_RIGHT) || (isRepeat && joyCheck(ubJoy + JOY_RIGHT))) {
		eDir = DIRECTION_RIGHT;
		pSteer->ulTimer = timerGet();
	}
	else if(joyUse(ubJoy + JOY_UP) || (isRepeat && joyCheck(ubJoy + JOY_UP))) {
		eDir = DIRECTION_UP;
		pSteer->ulTimer = timerGet();
	}
	else if(joyUse(ubJoy + JOY_DOWN) || (isRepeat && joyCheck(ubJoy + JOY_DOWN))) {
		eDir = DIRECTION_DOWN;
		pSteer->ulTimer = timerGet();
	}
	else if(joyUse(ubJoy + JOY_FIRE)) {
		eDir = DIRECTION_FIRE;
		pSteer->ulTimer = timerGet();
	}
	return eDir;
}

static tDirection onKey(tSteer *pSteer) {
	static const UBYTE pDirsWsad[] = {
		[DIRECTION_UP] = KEY_W,
		[DIRECTION_DOWN] = KEY_S,
		[DIRECTION_LEFT] = KEY_A,
		[DIRECTION_RIGHT] = KEY_D,
		[DIRECTION_FIRE] = KEY_LSHIFT
	};
	static const UBYTE pDirsArrows[] = {
		[DIRECTION_UP] = KEY_UP,
		[DIRECTION_DOWN] = KEY_DOWN,
		[DIRECTION_LEFT] = KEY_LEFT,
		[DIRECTION_RIGHT] = KEY_RIGHT,
		[DIRECTION_FIRE] = KEY_RSHIFT
	};

	const UBYTE *pKeymap = (pSteer->eKeymap == KEYMAP_WSAD) ? pDirsWsad : pDirsArrows;

	tDirection eDir = DIRECTION_COUNT;
	for(UBYTE i = 0; i < DIRECTION_COUNT; ++i) {
		if(
			keyUse(pKeymap[i]) ||
			(keyCheck(pKeymap[i]) && timerGetDelta(pSteer->ulTimer, timerGet()) > 10)
		) {
			pSteer->ulTimer = timerGet();
			eDir = i;
			break;
		}
	}

	return eDir;
}

static tDirection onAi(tSteer *pSteer) {
	tAi *pAi = &pSteer->sAi;
	return aiProcess(pAi);
}

static tDirection onIdle(UNUSED_ARG tSteer *pSteer) {
	// Do nothing
	return DIRECTION_COUNT;
}

//------------------------------------------------------------------- PUBLIC FNS

tSteer steerInitJoy(UBYTE ubJoy) {
	tSteer sSteer = {
		.cbProcess = onJoy,
		.ubJoy = ubJoy
	};
	return sSteer;
}

tSteer steerInitKey(tKeymap eKeymap) {
	tSteer sSteer = {
		.cbProcess = onKey,
		.eKeymap = eKeymap
	};
	return sSteer;
}

tSteer steerInitAi(UBYTE ubPlayerIdx) {
	tSteer sSteer = {
		.cbProcess = onAi
	};
	aiInit(&sSteer.sAi, ubPlayerIdx);
	return sSteer;
}

tSteer steerInitIdle(void) {
	tSteer sSteer = {
		.cbProcess = onIdle
	};
	return sSteer;
}

tDirection steerProcess(tSteer *pSteer) {
	if(pSteer->cbProcess) {
		return pSteer->cbProcess(pSteer);
	}
	return DIRECTION_COUNT;
}

UBYTE steerIsPlayer(const tSteer *pSteer) {
	return (pSteer->cbProcess == onJoy || pSteer->cbProcess == onKey);
}
