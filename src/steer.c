/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "steer.h"
#include <ace/managers/joy.h>
#include <ace/managers/key.h>

//------------------------------------------------------------------ PRIVATE FNS

static tDir onJoy(tSteer *pSteer) {
	UBYTE ubJoy = pSteer->ubJoy;
	tDir eDir = DIR_COUNT;
	if(joyUse(ubJoy + JOY_LEFT)) {
		eDir = DIR_LEFT;
	}
	else if(joyUse(ubJoy + JOY_RIGHT)) {
		eDir = DIR_RIGHT;
	}
	else if(joyUse(ubJoy + JOY_UP)) {
		eDir = DIR_UP;
	}
	else if(joyUse(ubJoy + JOY_DOWN)) {
		eDir = DIR_DOWN;
	}
	else if(joyUse(ubJoy + JOY_FIRE)) {
		eDir = DIR_FIRE;
	}
	return eDir;
}

static tDir onKey(tSteer *pSteer) {
	static const UBYTE pDirsWsad[] = {
		[DIR_UP] = KEY_W,
		[DIR_DOWN] = KEY_S,
		[DIR_LEFT] = KEY_A,
		[DIR_RIGHT] = KEY_D,
		[DIR_FIRE] = KEY_LSHIFT
	};
	static const UBYTE pDirsArrows[] = {
		[DIR_UP] = KEY_UP,
		[DIR_DOWN] = KEY_DOWN,
		[DIR_LEFT] = KEY_LEFT,
		[DIR_RIGHT] = KEY_RIGHT,
		[DIR_FIRE] = KEY_RSHIFT
	};

	const UBYTE *pKeymap = (pSteer->eKeymap == KEYMAP_WSAD) ? pDirsWsad : pDirsArrows;

	tDir eDir = DIR_COUNT;
	for(UBYTE i = 0; i < DIR_COUNT; ++i) {
		if(keyUse(pKeymap[i])) {
			eDir = i;
			break;
		}
	}

	return eDir;
}

static tDir onAi(tSteer *pSteer) {
	// TODO onAi
	return DIR_COUNT;
}

static tDir onIdle(UNUSED_ARG tSteer *pSteer) {
	// Do nothing
	return DIR_COUNT;
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
		.cbProcess = onAi,
		.ubPlayerIdx = ubPlayerIdx
	};
	return sSteer;
}

tSteer steerInitIdle(void) {
	tSteer sSteer = {
		.cbProcess = onIdle
	};
	return sSteer;
}

tDir steerProcess(tSteer *pSteer) {
	if(pSteer->cbProcess) {
		return pSteer->cbProcess(pSteer);
	}
	return DIR_COUNT;
}
