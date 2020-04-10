/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "menu.h"
#include <ace/types.h>
#include <ace/managers/system.h>
#include <ace/managers/game.h>
#include <ace/managers/key.h>
#include <ace/managers/joy.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/font.h>
#include <ace/utils/palette.h>
#include "game.h"
#include "build_ver.h"

#define MENU_COLOR_ACTIVE 13
#define MENU_COLOR_INACTIVE 14
#define MENU_COLOR_ERROR 9

typedef enum _tPlayerSteer {
	PLAYER_STEER_JOY_1,
	PLAYER_STEER_JOY_2,
	PLAYER_STEER_JOY_3,
	PLAYER_STEER_JOY_4,
	PLAYER_STEER_KEY_WSAD,
	PLAYER_STEER_KEY_ARROWS,
	PLAYER_STEER_AI,
	PLAYER_STEER_IDLE,
	PLAYER_STEER_OFF,
} tPlayerSteer;

static UBYTE s_pPlayerSteers[4] = {
	PLAYER_STEER_JOY_1, PLAYER_STEER_JOY_2,
	PLAYER_STEER_KEY_WSAD, PLAYER_STEER_KEY_ARROWS
};

static const char *g_pMenuEnumSteer[] = {
	"Joy 1", "Joy 2", "Joy 3", "Joy 4", "WSAD", "Arrows", "CPU", "Idle", "Off"
};

static const char *g_pMenuCaptions[] = {
	"Start game",
	"Player 1",
	"Player 2",
	"Player 3",
	"Player 4",
	"Exit to workbench"
};

static tView *s_pView;
static tVPort *s_pVp;
static tSimpleBufferManager *s_pBfr;

static void onStart(void);
static void onExit(void);

//------------------------------------------------------------------------- MENU

typedef enum _tOptionType {
	OPTION_TYPE_UINT8,
	OPTION_TYPE_CALLBACK
} tOptionType;

typedef void (*tOptionSelectCb)(void);

// All options are uint8_t, enums or numbers
typedef struct _tOption {
	tOptionType eOptionType;
	UBYTE isHidden;
	UBYTE isDirty;
	union {
		struct {
			UBYTE *pVar;
			UBYTE ubMax;
			UBYTE ubDefault;
			UBYTE isCyclic;
			const char **pEnumLabels;
		} sOptUb;
		struct {
			tOptionSelectCb cbSelect;
		} sOptCb;
	};
} tOption;

static UBYTE s_ubActivePos;
static tFont *s_pFont;
static tTextBitMap *s_pTextBitmap;

static tOption s_pOptions[] = {
	{OPTION_TYPE_CALLBACK, .isHidden = 0, .sOptCb = {.cbSelect = onStart}},
	{OPTION_TYPE_UINT8, .isHidden = 0, .sOptUb = {
		.pVar = &s_pPlayerSteers[0], .ubMax = PLAYER_STEER_IDLE, .isCyclic = 1,
		.ubDefault = PLAYER_STEER_JOY_1, .pEnumLabels = g_pMenuEnumSteer
	}},
	{OPTION_TYPE_UINT8, .isHidden = 0, .sOptUb = {
		.pVar = &s_pPlayerSteers[1], .ubMax = PLAYER_STEER_IDLE, .isCyclic = 1,
		.ubDefault = PLAYER_STEER_JOY_2, .pEnumLabels = g_pMenuEnumSteer
	}},
	{OPTION_TYPE_UINT8, .isHidden = 0, .sOptUb = {
		.pVar = &s_pPlayerSteers[2], .ubMax = PLAYER_STEER_IDLE, .isCyclic = 1,
		.ubDefault = PLAYER_STEER_OFF, .pEnumLabels = g_pMenuEnumSteer
	}},
	{OPTION_TYPE_UINT8, .isHidden = 0, .sOptUb = {
		.pVar = &s_pPlayerSteers[3], .ubMax = PLAYER_STEER_IDLE, .isCyclic = 1,
		.ubDefault = PLAYER_STEER_OFF, .pEnumLabels = g_pMenuEnumSteer
	}},
	{OPTION_TYPE_CALLBACK, .isHidden = 0, .sOptCb = {.cbSelect = onExit}},
};
#define MENU_POS_COUNT (sizeof(s_pOptions) / sizeof(tOption))

static void menuDrawPos(UBYTE ubPos, UWORD uwOffsTop) {
	UWORD uwOffsY = uwOffsTop + ubPos * (s_pFont->uwHeight);
	blitRect(s_pBfr->pBack, 0, uwOffsY, 320, s_pFont->uwHeight, 0);

	char szBfr[50];
	const char *szText = 0;
	if(s_pOptions[ubPos].eOptionType == OPTION_TYPE_UINT8) {
		if(s_pOptions[ubPos].sOptUb.pEnumLabels) {
			sprintf(
				szBfr, "%s: %s", g_pMenuCaptions[ubPos],
				s_pOptions[ubPos].sOptUb.pEnumLabels[*s_pOptions[ubPos].sOptUb.pVar]
			);
		}
		else {
			sprintf(
				szBfr, "%s: %hhu", g_pMenuCaptions[ubPos],
				*s_pOptions[ubPos].sOptUb.pVar
			);
		}
		szText = szBfr;
	}
	else if(s_pOptions[ubPos].eOptionType == OPTION_TYPE_CALLBACK) {
		szText = g_pMenuCaptions[ubPos];
	}
	if(szText != 0) {
		fontFillTextBitMap(s_pFont, s_pTextBitmap, szText);
		fontDrawTextBitMap(s_pBfr->pBack, s_pTextBitmap, 320 / 2, uwOffsY,
			(ubPos == s_ubActivePos) ? MENU_COLOR_ACTIVE : MENU_COLOR_INACTIVE,
			FONT_LAZY | FONT_HCENTER | FONT_COOKIE | FONT_SHADOW
		);
	}
}

static UBYTE menuNavigate(BYTE bDir) {
	WORD wNewPos = s_ubActivePos;

	// Find next non-hidden pos
	do {
		wNewPos += bDir;
	} while(0 < wNewPos && wNewPos < (WORD)MENU_POS_COUNT && s_pOptions[wNewPos].isHidden);

	if(wNewPos < 0 || wNewPos >= (WORD)MENU_POS_COUNT) {
		// Out of bounds - cancel
		return 0;
	}

	// Update active pos and mark as dirty
	s_pOptions[s_ubActivePos].isDirty = 1;
	s_pOptions[wNewPos].isDirty = 1;
	s_ubActivePos = wNewPos;
	return 1;
}

static UBYTE menuToggle(BYTE bDelta) {
	if(s_pOptions[s_ubActivePos].eOptionType == OPTION_TYPE_UINT8) {
		WORD wNewVal = *s_pOptions[s_ubActivePos].sOptUb.pVar + bDelta;
		if(wNewVal < 0 || wNewVal > s_pOptions[s_ubActivePos].sOptUb.ubMax) {
			if(s_pOptions[s_ubActivePos].sOptUb.isCyclic) {
				wNewVal = wNewVal < 0 ? s_pOptions[s_ubActivePos].sOptUb.ubMax : 0;
			}
			else {
				return 0; // Out of bounds on non-cyclic option
			}
		}
		*s_pOptions[s_ubActivePos].sOptUb.pVar = wNewVal;
		s_pOptions[s_ubActivePos].isDirty = 1;
		return 1;
	}
	return 0;
}

static UBYTE menuEnter(void) {
	if(s_pOptions[s_ubActivePos].eOptionType == OPTION_TYPE_CALLBACK) {
		s_pOptions[s_ubActivePos].sOptCb.cbSelect();
		return 1;
	}
	return 0;
}

//------------------------------------------------------------------ PRIVATE FNS

static void onExit(void) {
	gameClose();
}

static void onStart(void) {
	for(UBYTE i = 0; i < 4; ++i) {
		if((
			s_pPlayerSteers[i] == PLAYER_STEER_JOY_3 ||
			s_pPlayerSteers[i] == PLAYER_STEER_JOY_4
		) && !joyEnableParallel()) {
			fontFillTextBitMap(s_pFont, s_pTextBitmap, "Can't open parallel port for joystick adapter");
			fontDrawTextBitMap(
				s_pBfr->pBack, s_pTextBitmap, 320/2, 200,
				MENU_COLOR_ERROR, FONT_COOKIE | FONT_SHADOW | FONT_HCENTER
			);
			fontFillTextBitMap(s_pFont, s_pTextBitmap, "Joy3 & 4 are not usable!");
			fontDrawTextBitMap(
				s_pBfr->pBack, s_pTextBitmap, 320/2, 210,
				MENU_COLOR_ERROR, FONT_COOKIE | FONT_SHADOW | FONT_HCENTER
			);
			return;
		}
	}
	gameChangeState(gameGsCreate, gameGsLoop, gameGsDestroy);
}

void menuGsCreate(void) {
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
		TAG_SIMPLEBUFFER_IS_DBLBUF, 0,
		TAG_END
	);

	paletteLoad("data/germz.plt", s_pVp->pPalette, 32);
	s_pFont = fontCreate("data/uni54.fnt");
	s_pTextBitmap = fontCreateTextBitMap(320, s_pFont->uwHeight);

	for(UBYTE ubMenuPos = 0; ubMenuPos < MENU_POS_COUNT; ++ubMenuPos) {
		s_pOptions[ubMenuPos].isDirty = 1;
	}

	char szVersion[15];
	sprintf(szVersion, "v.%d.%d.%d", BUILD_YEAR, BUILD_MONTH, BUILD_DAY);
	fontFillTextBitMap(s_pFont, s_pTextBitmap, szVersion);
	fontDrawTextBitMap(s_pBfr->pBack, s_pTextBitmap, 320/2, 256 - 40, 18, FONT_HCENTER | FONT_COOKIE);

	fontFillTextBitMap(s_pFont, s_pTextBitmap, "Gfx: Softiron, Sfx: Luc3k, Code: KaiN");
	fontDrawTextBitMap(s_pBfr->pBack, s_pTextBitmap, 320/2, 256 - 30, 18, FONT_HCENTER | FONT_COOKIE);

	// fontFillTextBitMap(s_pFont, s_pTextBitmap, "Revision 2020 Party version - final coming soon!");
	fontFillTextBitMap(s_pFont, s_pTextBitmap, "Alpha version - don't spread before release!");
	fontDrawTextBitMap(s_pBfr->pBack, s_pTextBitmap, 320/2, 256 - 10, 17, FONT_HCENTER | FONT_COOKIE);

	systemUnuse();
	viewLoad(s_pView);
}

void menuGsLoop(void) {
	if(keyUse(KEY_ESCAPE)) {
		gameClose();
		return;
	}

	for(UBYTE ubMenuPos = 0; ubMenuPos < MENU_POS_COUNT; ++ubMenuPos) {
		if(!s_pOptions[ubMenuPos].isHidden && s_pOptions[ubMenuPos].isDirty) {
			menuDrawPos(ubMenuPos, 64);
			s_pOptions[ubMenuPos].isDirty = 0;
		}
	}

	UBYTE isEnabled34 = joyIsParallelEnabled();
	if(
		keyUse(KEY_UP) || keyUse(KEY_W) ||
		joyUse(JOY1_UP) || joyUse(JOY2_UP) ||
		(isEnabled34 && (joyUse(JOY3_UP) || joyUse(JOY4_UP)))
	) {
		menuNavigate(-1);
	}
	else if(
		keyUse(KEY_DOWN) || keyUse(KEY_S) ||
		joyUse(JOY1_DOWN) || joyUse(JOY2_DOWN) ||
		(isEnabled34 && (joyUse(JOY3_DOWN) || joyUse(JOY4_DOWN)))
	) {
		menuNavigate(+1);
	}
	else if(
		keyUse(KEY_LEFT) || keyUse(KEY_A) ||
		joyUse(JOY1_LEFT) || joyUse(JOY2_LEFT) ||
		(isEnabled34 && (joyUse(JOY3_LEFT) || joyUse(JOY4_LEFT)))
	) {
		menuToggle(-1);
	}
	else if(
		keyUse(KEY_RIGHT) || keyUse(KEY_D) ||
		joyUse(JOY1_RIGHT) || joyUse(JOY2_RIGHT) ||
		(isEnabled34 && (joyUse(JOY3_RIGHT) || joyUse(JOY4_RIGHT)))
	) {
		menuToggle(+1);
	}
	else if(
		keyUse(KEY_RETURN) || keyUse(KEY_LSHIFT) || keyUse(KEY_RSHIFT) ||
		joyUse(JOY1_FIRE) || joyUse(JOY2_FIRE) ||
		(isEnabled34 && (joyUse(JOY3_FIRE) || joyUse(JOY4_FIRE)))
	) {
		menuEnter();
	}

	copProcessBlocks();
	vPortWaitForEnd(s_pVp);
}

void menuGsDestroy(void) {
	viewLoad(0);
	systemUse();
	viewDestroy(s_pView);
	fontDestroyTextBitMap(s_pTextBitmap);
	fontDestroy(s_pFont);
}

UBYTE menuIsPlayerActive(UBYTE ubPlayerIdx) {
	if(s_pPlayerSteers[ubPlayerIdx] == PLAYER_STEER_OFF) {
		return 0;
	}
	return 1;
}

tSteer menuGetSteerForPlayer(UBYTE ubPlayerIdx) {
	switch(s_pPlayerSteers[ubPlayerIdx]) {
		case PLAYER_STEER_JOY_1:
			return steerInitJoy(JOY1);
		case PLAYER_STEER_JOY_2:
			return steerInitJoy(JOY2);
		case PLAYER_STEER_JOY_3:
			return steerInitJoy(JOY3);
		case PLAYER_STEER_JOY_4:
			return steerInitJoy(JOY4);
		case PLAYER_STEER_KEY_ARROWS:
			return steerInitKey(KEYMAP_ARROWS);
		case PLAYER_STEER_KEY_WSAD:
			return steerInitKey(KEYMAP_WSAD);
		case PLAYER_STEER_AI:
			return steerInitAi(ubPlayerIdx);
	}
	return steerInitIdle();
}
