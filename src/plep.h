/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_PLEP_H_
#define _GERMZ_PLEP_H_

#include "bob_new.h"
#include "dir.h"

//------------------------------------------------------------------------ TYPES

typedef enum _tPlepAnim {
	PLEP_ANIM_BORN,
	PLEP_ANIM_MOVE,
	PLEP_ANIM_WIN,
	PLEP_ANIM_LOSE,
	PLEP_ANIM_COUNT,
} tPlepAnim;

typedef struct _tPlep {
	UBYTE isActive;
	tDir eDir;
	UBYTE ubAnimFrame;
	tUwCoordYX sAnimAnchor;
	tPlepAnim eAnim;
	WORD wCharges;
	tBobNew sBob;
	struct _tPlayer *pPlayer;
	struct _tNode *pDestination;
} tPlep;

#include "player.h"

void plepCreate(void);

void plepDestroy(void);

void plepReset(tPlep *pPlep, struct _tPlayer *pPlayer);

void plepProcess(tPlep *pPlep);

void plepSpawn(tPlep *pPlep, WORD wCharges, tDir eDir);

void plepSetMap(const tMap *pMap);

#endif // _GERMZ_PLEP_H_
