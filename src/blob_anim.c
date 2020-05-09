/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "blob_anim.h"
#include "map.h"
#include "game.h"
#include "game_assets.h"
#include "player.h"

#define QUEUE_ELEMENTS_MAX 10

typedef struct _tQueueElement {
	tNode *pNode;
	UBYTE isDrawnOnce;
	UBYTE ubFrame;
} tQueueElement;

typedef struct _tQueue {
	tQueueElement *pBeg, *pEnd, *pWrap;
	tQueueElement pElements[QUEUE_ELEMENTS_MAX];
} tQueue;

static tQueue s_sQueue;

static void advanceFrameForNode(tQueueElement *pElement) {
	if(pElement->isDrawnOnce) {
		pElement->isDrawnOnce = 0;
		if(++pElement->ubFrame >= BLOB_FRAME_COUNT) {
			// Sanity check
			if(pElement != s_sQueue.pBeg) {
				logWrite("ERR: Queue move not on begin of queue");
			}

			if(++s_sQueue.pBeg >= s_sQueue.pWrap) {
				s_sQueue.pBeg = &s_sQueue.pElements[0];
			}
		}
	}
	else {
		pElement->isDrawnOnce = 1;
	}
}

void blobAnimReset(void) {
	s_sQueue.pBeg = &s_sQueue.pElements[0];
	s_sQueue.pEnd = s_sQueue.pBeg;
	s_sQueue.pWrap = &s_sQueue.pElements[QUEUE_ELEMENTS_MAX];
}

void blobAnimQueueProcess(void) {
	tQueueElement *pElement = s_sQueue.pBeg;
	while(pElement != s_sQueue.pEnd) {
		const tNode *pNode = pElement->pNode;
		gameDrawBlobAt(
			playerToTile(pNode->pPlayer), pElement->ubFrame,
			pNode->ubTileX, pNode->ubTileY
		);
		advanceFrameForNode(pElement);
		if(++pElement >= s_sQueue.pWrap) {
			pElement = &s_sQueue.pElements[0];
		}
	}
}

void blobAnimAddToQueue(tNode *pNode) {
	tQueueElement *pElement = s_sQueue.pEnd;
	pElement->pNode = pNode;
	pElement->ubFrame = 0;
	pElement->isDrawnOnce = 0;
	if(++s_sQueue.pEnd >= s_sQueue.pWrap) {
		s_sQueue.pEnd = &s_sQueue.pElements[0];
	}
}
