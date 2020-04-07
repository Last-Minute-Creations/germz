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
	// pSteer->eKeymap;
	return DIR_COUNT;
}

static tDir onAi(tSteer *pSteer) {
	// TODO onAi
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

tSteer steerInitAi(void) {
	tSteer sSteer = {
		.cbProcess = onAi
	};
	return sSteer;
}

tDir steerProcess(tSteer *pSteer) {
	if(pSteer->cbProcess) {
		return pSteer->cbProcess(pSteer);
	}
	return DIR_COUNT;
}
