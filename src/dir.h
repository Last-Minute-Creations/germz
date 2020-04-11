/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_DIR_H_
#define _GERMZ_DIR_H_

typedef enum _tDir {
	DIR_UP,
	DIR_DOWN,
	DIR_LEFT,
	DIR_RIGHT,
	DIR_FIRE,
	DIR_COUNT
} tDir;

static inline UBYTE dirIsVertical(tDir eDir) {
	return (eDir < DIR_LEFT);
}

#endif // _GERMZ_DIR_H_
