/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_BLOB_ANIM_H_
#define _GERMZ_BLOB_ANIM_H_

#include <ace/utils/bitmap.h>
#include "map.h"

void blobAnimReset(void);

void blobAnimQueueProcess(void);

void blobAnimAddToQueue(tNode *pNode);

#endif // _GERMZ_BLOB_ANIM_H_
