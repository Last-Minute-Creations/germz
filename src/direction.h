/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_DIRECTION_H_
#define _GERMZ_DIRECTION_H_

#include <ace/types.h>

typedef enum _tDirection {
	DIRECTION_UP,
	DIRECTION_DOWN,
	DIRECTION_LEFT,
	DIRECTION_RIGHT,
	DIRECTION_FIRE,
	DIRECTION_COUNT
} tDirection;

static inline UBYTE dirIsVertical(tDirection eDir) {
	return (eDir < DIRECTION_LEFT);
}

#endif // _GERMZ_DIRECTION_H_
