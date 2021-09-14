/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/system.h>
#include <ace/managers/key.h>
#include <ace/managers/joy.h>
#include <ace/utils/palette.h>
#include <ace/generic/screen.h>
#include "fade.h"
#include "germz.h"
#include "assets.h"
#include "color.h"

#define SLIDES_MAX 10
#define LINES_PER_SLIDE_MAX 5
#define SLIDE_SIZE 128
#define SLIDE_POS_X ((SCREEN_PAL_WIDTH - SLIDE_SIZE) / 2)
#define SLIDE_POS_Y ((SCREEN_PAL_HEIGHT - SLIDE_SIZE) / 4)
#define TEXT_POS_X (SCREEN_PAL_WIDTH / 2)
#define TEXT_POS_Y (SLIDE_POS_Y + SLIDE_SIZE + 16)
#define SLIDE_RECT_SIZE 8

static const char *s_pLines[][LINES_PER_SLIDE_MAX] = {
	{
		"The great entity created its offspring",
		"and scattered them across the universe",
		0,
	},
	{
		"They travelled through space and eons of time.",
		"Some of them perished on barren rocks",
		"failing to find any nutrition,",
		"but some survived through hibernation",
		0,
	},
	{
		"The more resilient ones have found their way",
		"to colorful planets full of life",
		0,
	},
	{
		"Decimated by the atmosphere,",
		"only strongest survived the landing",
		0,
	},
	{
		"Ignored by most,",
		"but welcomed by curious inhabitants",
		0,
	},
	{
		"Revived by their science",
		"and waiting for a chance...",
		0,
	},
	{
		"You finally seize the opportunity",
		"and enter the unwilling host",
		0,
	},
	{
		"Fight through its defenses",
		"and take over the whole organism!",
		0,
	},
};

static tView *s_pView;
static tVPort *s_pVp;
static tSimpleBufferManager *s_pBuffer;
static tFade *s_pFade;
static UBYTE s_ubSlideCount, s_ubCurrentSlide, s_ubCurrentLine;
static const char *s_szDirName;
static tState *s_pNextState;

static tBitMap *s_pSlides[SLIDES_MAX];

// see res/slide_anim.py
static const UBYTE s_pCoords[] = {
	0xFD, 0x42, 0x91, 0x8C, 0xB1, 0x5D, 0xC1, 0xC0,
	0xC3, 0x05, 0xD8, 0x00, 0xF9, 0x6E, 0x67, 0x9F,
	0xFE, 0x73, 0x65, 0x2B, 0x46, 0x23, 0xD6, 0x94,
	0x4F, 0xBB, 0xAE, 0x1C, 0x4A, 0x45, 0x99, 0x48,
	0xEC, 0x8A, 0x2D, 0xDE, 0x7B, 0x15, 0xE4, 0x0B,
	0xE8, 0x1F, 0xEF, 0x2F, 0x07, 0xC6, 0x89, 0x0D,
	0x83, 0x35, 0x66, 0x3D, 0x2A, 0x55, 0xCD, 0xE6,
	0xBA, 0xAC, 0xB8, 0x14, 0x84, 0x1E, 0x0A, 0x50,
	0xA9, 0x9D, 0x95, 0x93, 0xD5, 0x39, 0xA6, 0xD3,
	0x90, 0xF6, 0x6C, 0x44, 0xD9, 0x03, 0x31, 0x19,
	0x57, 0x8D, 0xA0, 0x24, 0xC5, 0xCA, 0x0E, 0xCF,
	0xFF, 0x75, 0x60, 0x82, 0x7C, 0x5B, 0x1B, 0x10,
	0x6A, 0xD1, 0x79, 0xD2, 0xB4, 0xAB, 0x77, 0x1A,
	0x53, 0x4E, 0x17, 0xF1, 0x5F, 0x13, 0x3E, 0xB5,
	0x76, 0x56, 0xC2, 0x1D, 0x7A, 0x49, 0xA7, 0x3F,
	0x37, 0x32, 0xA3, 0x30, 0xB6, 0x4C, 0x74, 0xE1,
	0x06, 0xE3, 0x6D, 0x34, 0x78, 0x6B, 0x09, 0x12,
	0xEA, 0x38, 0xAF, 0xBC, 0x8B, 0x62, 0xF2, 0xEE,
	0xF3, 0x54, 0x0C, 0xC4, 0x3A, 0x08, 0xD0, 0x98,
	0x9E, 0xDA, 0xBE, 0x51, 0x43, 0xCE, 0x18, 0x9C,
	0x41, 0x81, 0x9B, 0x80, 0xB0, 0xA8, 0x5A, 0xE2,
	0x26, 0x20, 0x72, 0xE9, 0x97, 0xAD, 0x64, 0x21,
	0xB2, 0x86, 0xCB, 0x92, 0xC7, 0x69, 0x85, 0xBF,
	0x3B, 0xE0, 0xCC, 0x63, 0xD4, 0x02, 0x5C, 0x2C,
	0x88, 0xD7, 0x11, 0x70, 0x71, 0xC8, 0xB7, 0xBD,
	0xFA, 0x7D, 0x40, 0xE5, 0xB3, 0x4D, 0x7E, 0xF4,
	0xED, 0xF0, 0x22, 0xEB, 0xF7, 0xDC, 0x96, 0xA2,
	0x52, 0xE7, 0xDB, 0x4B, 0x2E, 0x58, 0xA4, 0x5E,
	0x47, 0xDD, 0x27, 0x3C, 0xA5, 0x36, 0x61, 0x7F,
	0x04, 0xF5, 0x59, 0xA1, 0x87, 0xF8, 0xAA, 0x8F,
	0x68, 0x29, 0x28, 0xFB, 0x6F, 0x16, 0x8E, 0xDF,
	0xFC, 0x01, 0x9A, 0xC9, 0x33, 0x0F, 0xB9, 0x25,
};

static void drawSlide(void) {
	UWORD uwLineEraseCurr = TEXT_POS_Y;
	UWORD uwLineEraseEnd = uwLineEraseCurr + s_ubCurrentLine * (g_pFontSmall->uwHeight + 2);

	// Draw slide
	UWORD uwRect = 0;
	while(uwRect < sizeof(s_pCoords)) {
		for(UBYTE i = 0; i < 4; ++i) {
			UBYTE x = s_pCoords[uwRect] >> 4;
			UBYTE y = s_pCoords[uwRect] & 0xF;
			blitCopy(
				s_pSlides[s_ubCurrentSlide],
				x * SLIDE_RECT_SIZE, y * SLIDE_RECT_SIZE, s_pBuffer->pBack,
				SLIDE_POS_X + x * SLIDE_RECT_SIZE, SLIDE_POS_Y + y * SLIDE_RECT_SIZE,
				SLIDE_RECT_SIZE, SLIDE_RECT_SIZE, MINTERM_COOKIE
			);
			++uwRect;
		}

		// Erase old text
		if(s_ubCurrentLine && uwLineEraseCurr < uwLineEraseEnd) {
			blitRect(
				s_pBuffer->pBack, 0, uwLineEraseCurr,
				SCREEN_PAL_WIDTH, 1, 0
			);
			++uwLineEraseCurr;
		}

		vPortWaitForEnd(s_pVp);
	}

	// Draw first portion of text
	s_ubCurrentLine = 0;
	const char *szLine = s_pLines[s_ubCurrentSlide][s_ubCurrentLine];
	fontDrawStr(
		g_pFontSmall, s_pBuffer->pBack, TEXT_POS_X, TEXT_POS_Y, szLine,
		COLOR_P3_BRIGHT, FONT_LAZY | FONT_HCENTER, g_pTextBitmap
	);
}

static void cutsceneGsCreate(void) {
	s_pView = viewCreate(0,
		TAG_VIEW_GLOBAL_CLUT, 1,
	TAG_END);

	s_pVp = vPortCreate(0,
		TAG_VPORT_BPP, 5,
		TAG_VPORT_VIEW, s_pView,
	TAG_END);

	s_pBuffer = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
		TAG_SIMPLEBUFFER_VPORT, s_pVp,
		TAG_SIMPLEBUFFER_IS_DBLBUF, 0,
	TAG_END);

	// Load palette
	UWORD pPalette[32];
	paletteLoad("data/germz.plt", pPalette, 32);
	s_pFade = fadeCreate(s_pView, pPalette, 32);

	// Load slides
	char szPath[30];
	s_ubSlideCount = 0;
	for(s_ubSlideCount = 0; s_ubSlideCount < 10; ++s_ubSlideCount) {
		sprintf(szPath, "data/%s/%hhu.bm", s_szDirName, s_ubSlideCount);
		s_pSlides[s_ubSlideCount] = bitmapCreateFromFile(szPath, 0);
		if(!s_pSlides[s_ubSlideCount]) {
			break;
		}
	}

	// Load text array

	systemUnuse();

	// Draw first slide
	drawSlide();
	fadeSet(s_pFade, FADE_STATE_IN, 50, 0);
	viewLoad(s_pView);
}

static void onCutsceneFadeOut(void) {
	// Pop to previous state or change to next state
	if(!s_pNextState) {
		statePop(g_pStateMachineGame);
	}
	else {
		stateChange(g_pStateMachineGame, s_pNextState);
	}
}

static void cutsceneGsLoop(void) {
	tFadeState eFadeState = fadeProcess(s_pFade);

	UBYTE isEnabled34 = joyIsParallelEnabled();
	if(eFadeState != FADE_STATE_OUT && (
		keyUse(KEY_RETURN) || keyUse(KEY_LSHIFT) || keyUse(KEY_RSHIFT) ||
		joyUse(JOY1_FIRE) || joyUse(JOY2_FIRE) ||
		(isEnabled34 && (joyUse(JOY3_FIRE) || joyUse(JOY4_FIRE)))
	)) {
		// Advance line
		++s_ubCurrentLine;
		const char *szLine = s_pLines[s_ubCurrentSlide][s_ubCurrentLine];
		if(szLine) {
			// Draw next portion of text
			fontDrawStr(
				g_pFontSmall, s_pBuffer->pBack, TEXT_POS_X,
				TEXT_POS_Y + (g_pFontSmall->uwHeight + 2) * s_ubCurrentLine, szLine,
				COLOR_P3_BRIGHT, FONT_LAZY | FONT_HCENTER, g_pTextBitmap
			);
		}
		else {
			if(++s_ubCurrentSlide < s_ubSlideCount) {
				// Draw next slide
				drawSlide();
			}
			else {
				// Quit the cutscene
				fadeSet(s_pFade, FADE_STATE_OUT, 50, onCutsceneFadeOut);
			}
		}
	}
}

static void cutsceneGsDestroy(void) {
	viewLoad(0);
	systemUse();
	viewDestroy(s_pView);
	fadeDestroy(s_pFade);

	// Destroy slides
	for(UBYTE i = 0; i < s_ubSlideCount; ++i) {
		bitmapDestroy(s_pSlides[i]);
	}

	// Destroy text array

}

void cutsceneSetup(const char *szName, tState *pNextState) {
	s_szDirName = szName;
	s_ubCurrentSlide = 0;
	s_ubCurrentLine = 0;
	s_pNextState = pNextState;
}

tState g_sStateCutscene = {
	.cbCreate = cutsceneGsCreate, .cbLoop = cutsceneGsLoop,
	.cbDestroy = cutsceneGsDestroy
};
