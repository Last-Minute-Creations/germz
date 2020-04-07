/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_DISPLAY_H_
#define _GERMZ_DISPLAY_H_

#include <ace/types.h>
#include "map.h"

void displayCreate(void);

void displayDestroy(void);

UBYTE displayInitialAnim(const tMap *pMap);

void displayAddNodeToQueue(tNode *pNode);

void displayQueueProcess(void);

void displayEnable(void);

void displayDebugColor(UWORD uwColor);

void displayLag(void);

void displayProcess(void);

#endif // _GERMZ_DISPLAY_H_
