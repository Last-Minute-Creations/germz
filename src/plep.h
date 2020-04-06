/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_PLEP_H_
#define _GERMZ_PLEP_H_

#include "bob_new.h"

typedef struct _tPlep {
	UBYTE isActive;
	struct _tPlayer *pPlayer;
	BYTE bDeltaX, bDeltaY;
	tBobNew sBob;
	struct _tNode *pDestination;
} tPlep;

#include "player.h"

void plepCreate(void);

void plepDestroy(void);

void plepReset(tPlep *pPlep, struct _tPlayer *pPlayer);

void plepProcess(tPlep *pPlep);

void plepSpawn(tPlep *pPlep);

#endif // _GERMZ_PLEP_H_
