/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "display.h"
#include <ace/managers/blit.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/blit.h>
#include <ace/managers/system.h>
#include <ace/utils/palette.h>
#include <ace/utils/font.h>
#include "bob_new.h"

// 1. wyjscie germza z komorki
// 2. ruch przez linie po wyjsciu z komorki
// 3. ruch przez linie (zapetlac na kazdej wolnej liniiaz do momentu jak bedzie sasiadowac z komorka)
// 4. wnikniecie do komorki
// 5. nieudane wnikniecie do komorki
// 6. zarazanie sąsiadujacej komórki

//---------------------------------------------------------------------- DEFINES

#define QUEUE_ELEMENTS_MAX 10
#define FRAME_COUNT 8
#define FRAME_SIZE 16
#define HUD_OFFS_X 256
#define HUD_MONITOR_SIZE 64

//------------------------------------------------------------------------ TYPES

typedef struct _tQueueElement {
	tNode *pNode;
	UBYTE isDrawnOnce;
	UBYTE ubFrame;
} tQueueElement;

typedef struct _tQueue {
	tQueueElement *pBeg, *pEnd, *pWrap;
	tQueueElement pElements[QUEUE_ELEMENTS_MAX];
} tQueue;

//----------------------------------------------------------------- PRIVATE VARS

tQueue s_sQueue;

static tView *s_pView;
static tVPort *s_pVp;
static tSimpleBufferManager *s_pBfr;

static tBitMap *s_pBmBlobs[TILE_BLOB_COUNT];
static tBitMap *s_pBmLinks;
static tBitMap *s_pBmBlobMask;
static UWORD s_uwColorBg;
static tFont *s_pFont;
static tTextBitMap *s_pBmLine;

//------------------------------------------------------------------ PRIVATE FNS

static void displayAdvanceFrameInElement(tQueueElement *pElement) {
	if(pElement->isDrawnOnce) {
		pElement->isDrawnOnce = 0;
		if(++pElement->ubFrame >= FRAME_COUNT) {
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

static void displayQueueReset(void) {
	s_sQueue.pBeg = &s_sQueue.pElements[0];
	s_sQueue.pEnd = s_sQueue.pBeg;
	s_sQueue.pWrap = &s_sQueue.pElements[QUEUE_ELEMENTS_MAX];
}

//------------------------------------------------------------------- PUBLIC FNS

void displayCreate(void) {
	s_pView = viewCreate(0,
		TAG_VIEW_COPLIST_MODE, COPPER_MODE_BLOCK,
		TAG_VIEW_GLOBAL_CLUT, 1,
		TAG_END
	);

	s_pVp = vPortCreate(0,
		TAG_VPORT_VIEW, s_pView,
		TAG_VPORT_BPP, 5,
		TAG_END
	);

	s_pBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_VPORT, s_pVp,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
		TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
		TAG_END
	);

	paletteLoad("data/germz.plt", s_pVp->pPalette, 32);
	s_uwColorBg = s_pVp->pPalette[0];

	// Draw monitors on back buffer
	bitmapLoadFromFile(s_pBfr->pBack, "data/monitor.bm", HUD_OFFS_X, 0);
	for(UBYTE i = 1; i < 4; ++i) {
		blitCopyAligned(
			s_pBfr->pBack, HUD_OFFS_X, 0, s_pBfr->pBack,
			HUD_OFFS_X, i * HUD_MONITOR_SIZE, HUD_MONITOR_SIZE, HUD_MONITOR_SIZE
		);
	}

	// Split back->front for OCS limitations
	blitCopyAligned(
		s_pBfr->pBack, HUD_OFFS_X, 0, s_pBfr->pFront, HUD_OFFS_X, 0,
		HUD_MONITOR_SIZE, 2 * HUD_MONITOR_SIZE
	);
	blitCopyAligned(
		s_pBfr->pBack, HUD_OFFS_X, 2 * HUD_MONITOR_SIZE,
		s_pBfr->pFront, HUD_OFFS_X, 2 * HUD_MONITOR_SIZE,
		HUD_MONITOR_SIZE, 2 * HUD_MONITOR_SIZE
	);

	for(UBYTE i = 0; i < TILE_BLOB_COUNT; ++i) {
		char szName[15];
		sprintf(szName, "data/blob%hhu.bm", i);
		s_pBmBlobs[i] = bitmapCreateFromFile(szName, 0);
	}
	s_pBmBlobMask = bitmapCreateFromFile("data/blob_mask.bm", 0);
	s_pBmLinks = bitmapCreateFromFile("data/links.bm", 0);

	bobNewManagerCreate(s_pBfr->pFront, s_pBfr->pBack, s_pBfr->uBfrBounds.uwY);
	displayQueueReset();
	playerCreate();

	s_pFont = fontCreate("data/uni54.fnt");
	s_pBmLine = fontCreateTextBitMap(64, s_pFont->uwHeight);
}

void displayDestroy(void) {
	viewLoad(0);
	systemUse();
	fontDestroyTextBitMap(s_pBmLine);
	fontDestroy(s_pFont);
	playerDestroy();
	for(UBYTE i = 0; i < TILE_BLOB_COUNT; ++i) {
		bitmapDestroy(s_pBmBlobs[i]);
	}
	bitmapDestroy(s_pBmBlobMask);
	bitmapDestroy(s_pBmLinks);

	bobNewManagerDestroy();

	viewDestroy(s_pView);
}

static tUbCoordYX getNth(UWORD uwN) {
	tUbCoordYX sPos = {.ubX = uwN % 16, .ubY = uwN / 16};
	return sPos;
}

static UWORD s_uwCurr = 0;
static UBYTE s_isEven = 0;


UBYTE displayInitialAnim(const tMap *pMap) {
	static const UBYTE s_ubStep = 2;
	static const UBYTE s_ubAnimCount = 8;
	static const UBYTE s_ubTailLength = 8;
	for(UBYTE i = 0; i < s_ubTailLength; ++i) {
		WORD wPos = s_uwCurr - i;
		if(wPos < 0) {
			break;
		}
		if(wPos >= 256) {
			continue;
		}
		tUbCoordYX sPos = getNth(wPos);
		tTile eTile = pMap->pTiles[sPos.ubX][sPos.ubY];
		if(eTile < TILE_BLOB_COUNT) {
			// Animate
			UWORD uwOffsY = ((i * s_ubAnimCount) / s_ubTailLength) * 16;
			blitCopyAligned(
				s_pBmBlobs[eTile], 0, uwOffsY,
				s_pBfr->pBack, sPos.ubX * 16, sPos.ubY * 16, 16, 16
			);
		}
		else {
			// Don't animate
			UWORD uwOffsY = 16 * (eTile - TILE_PATH_H1);
			blitCopyAligned(
				s_pBmLinks, 0, uwOffsY, s_pBfr->pBack,
				sPos.ubX * 16, sPos.ubY * 16, 16, 16
			);
		}
	}

	if(s_isEven) {
		s_uwCurr += s_ubStep;
		if(s_uwCurr - s_ubTailLength >= 16*16) {
			return 1;
		}
	}
	s_isEven = !s_isEven;
	return 0;
}

void displayQueueProcess(void) {
	tQueueElement *pElement = s_sQueue.pBeg;
	while(pElement != s_sQueue.pEnd) {
		const tNode *pNode = pElement->pNode;
		blitCopyMask(
			s_pBmBlobs[playerToTile(pNode->pPlayer)],
			0, pElement->ubFrame * FRAME_SIZE,
			s_pBfr->pBack, pNode->ubTileX * FRAME_SIZE, pNode->ubTileY * FRAME_SIZE,
			FRAME_SIZE, FRAME_SIZE, (const UWORD*) s_pBmBlobMask->Planes[0]
		);
		displayAdvanceFrameInElement(pElement);
		if(++pElement >= s_sQueue.pWrap) {
			pElement = &s_sQueue.pElements[0];
		}
	}
}

void displayAddNodeToQueue(tNode *pNode) {
	tQueueElement *pElement = s_sQueue.pEnd;
	pElement->pNode = pNode;
	pElement->ubFrame = 0;
	pElement->isDrawnOnce = 0;
	if(++s_sQueue.pEnd >= s_sQueue.pWrap) {
		s_sQueue.pEnd = &s_sQueue.pElements[0];
	}
}

void displayEnable(void) {
	bobNewAllocateBgBuffers();
	systemUnuse();
	viewLoad(s_pView);
}

void displayLag(void) {
	vPortWaitForEnd(s_pVp);
	vPortWaitForEnd(s_pVp);
	vPortWaitForEnd(s_pVp);
	vPortWaitForEnd(s_pVp);
}

void displayProcess(void) {
	displayDebugColor(0xFF0);
	displayDebugColor(0xF00);
	viewProcessManagers(s_pView);
	copProcessBlocks();
	displayDebugColor(s_uwColorBg);
	vPortWaitForEnd(s_pVp);
}

void displayDebugColor(UWORD uwColor) {
// #if defined(GAME_DEBUG)
	g_pCustom->color[0] = uwColor;
// #endif
}

static UBYTE s_ubCurrPlayer = 1;
static UBYTE s_ubWasEven = 0;

void displayUpdateHud(void) {
	static const UBYTE pPlayerColors[] = {8, 12, 16, 20};
	const tPlayer *pPlayer = playerFromIdx(s_ubCurrPlayer);
	UBYTE ubZeroBased = s_ubCurrPlayer - 1;
	const UBYTE ubMonitorPad = 7;
	UWORD uwMonitorX = HUD_OFFS_X + ubMonitorPad;
	UWORD uwMonitorY = ubZeroBased * HUD_MONITOR_SIZE + ubMonitorPad;

	blitRect(s_pBfr->pBack, uwMonitorX, uwMonitorY, 16, s_pFont->uwHeight, 6);
	if(pPlayer->pNodeCursor && pPlayer->pNodeCursor->pPlayer == pPlayer) {
		char szBfr[4];
		sprintf(szBfr, "%hhd", pPlayer->pNodeCursor->bCharges);
		fontFillTextBitMap(s_pFont, s_pBmLine, szBfr);
		fontDrawTextBitMap(
			s_pBfr->pBack, s_pBmLine, uwMonitorX, uwMonitorY,
			pPlayerColors[ubZeroBased], FONT_COOKIE
		);
	}

	if(s_ubWasEven) {
		if(++s_ubCurrPlayer > 4) {
			s_ubCurrPlayer = 1;
		}
	}
	s_ubWasEven = !s_ubWasEven;
}
