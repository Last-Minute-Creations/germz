/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_STEER_H_
#define _GERMZ_STEER_H_

#include <ace/types.h>
#include "dir.h"

struct _tSteer;

typedef tDir (*tCbSteerProcess)(struct _tSteer *pSteer);

typedef enum _tKeymap {
	KEYMAP_WSAD,
	KEYMAP_ARROWS
} tKeymap;

typedef struct _tSteer {
	tCbSteerProcess cbProcess;
	union {
		UBYTE ubJoy;
		tKeymap eKeymap;
	};
} tSteer;

tSteer steerInitJoy(UBYTE ubJoy);

tSteer steerInitKey(tKeymap eKeymap);

tSteer steerInitAi(void);

tDir steerProcess(tSteer *pSteer);

#endif // _GERMZ_STEER_H_