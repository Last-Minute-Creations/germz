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
#include <ace/utils/ptplayer.h>
#include "game.h"
#include "build_ver.h"
#include "fade.h"
#include "germz.h"

#define MENU_COLOR_ACTIVE 16
#define MENU_COLOR_INACTIVE 17
#define MENU_COLOR_SHADOW 19
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

static const char *s_pMenuEnumSteer[] = {
	"Joy 1", "Joy 2", "Joy 3", "Joy 4", "WSAD", "Arrows", "CPU", "Idle", "Off"
};

static const char *s_pMenuCaptions[] = {
	"Infect",
	"Player 1",
	"Player 2",
	"Player 3",
	"Player 4",
	"Editor",
	"Credits",
	"Cure"
};

static tView *s_pView;
static tVPort *s_pVp;
static tSimpleBufferManager *s_pBfr;
static tBitMap *s_pBg, *s_pBgSub;
static tFade *s_pFadeMenu;

static void onStart(void);
static void onEditor(void);
static void onCredits(void);
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
		.ubDefault = PLAYER_STEER_JOY_1, .pEnumLabels = s_pMenuEnumSteer
	}},
	{OPTION_TYPE_UINT8, .isHidden = 0, .sOptUb = {
		.pVar = &s_pPlayerSteers[1], .ubMax = PLAYER_STEER_IDLE, .isCyclic = 1,
		.ubDefault = PLAYER_STEER_JOY_2, .pEnumLabels = s_pMenuEnumSteer
	}},
	{OPTION_TYPE_UINT8, .isHidden = 0, .sOptUb = {
		.pVar = &s_pPlayerSteers[2], .ubMax = PLAYER_STEER_IDLE, .isCyclic = 1,
		.ubDefault = PLAYER_STEER_OFF, .pEnumLabels = s_pMenuEnumSteer
	}},
	{OPTION_TYPE_UINT8, .isHidden = 0, .sOptUb = {
		.pVar = &s_pPlayerSteers[3], .ubMax = PLAYER_STEER_IDLE, .isCyclic = 1,
		.ubDefault = PLAYER_STEER_OFF, .pEnumLabels = s_pMenuEnumSteer
	}},
	{OPTION_TYPE_CALLBACK, .isHidden = 0, .sOptCb = {.cbSelect = onEditor}},
	{OPTION_TYPE_CALLBACK, .isHidden = 0, .sOptCb = {.cbSelect = onCredits}},
	{OPTION_TYPE_CALLBACK, .isHidden = 0, .sOptCb = {.cbSelect = onExit}},
};
#define MENU_POS_COUNT (sizeof(s_pOptions) / sizeof(tOption))

static void menuDrawPos(UBYTE ubPos, UWORD uwOffsTop) {
	UWORD uwOffsY = uwOffsTop + ubPos * (s_pFont->uwHeight + 2);
	blitCopy(
		s_pBg, 64, uwOffsY, s_pBfr->pBack, 64, uwOffsY,
		320 - (2 * 64), s_pFont->uwHeight + 1, MINTERM_COPY, 0xFF
	);

	char szBfr[50];
	const char *szText = 0;
	if(s_pOptions[ubPos].eOptionType == OPTION_TYPE_UINT8) {
		if(s_pOptions[ubPos].sOptUb.pEnumLabels) {
			sprintf(
				szBfr, "%s: %s", s_pMenuCaptions[ubPos],
				s_pOptions[ubPos].sOptUb.pEnumLabels[*s_pOptions[ubPos].sOptUb.pVar]
			);
		}
		else {
			sprintf(
				szBfr, "%s: %hhu", s_pMenuCaptions[ubPos],
				*s_pOptions[ubPos].sOptUb.pVar
			);
		}
		szText = szBfr;
	}
	else if(s_pOptions[ubPos].eOptionType == OPTION_TYPE_CALLBACK) {
		szText = s_pMenuCaptions[ubPos];
	}
	if(szText != 0) {
		fontFillTextBitMap(s_pFont, s_pTextBitmap, szText);
		fontDrawTextBitMap(s_pBfr->pBack, s_pTextBitmap, 139, uwOffsY + 1,
			MENU_COLOR_SHADOW, FONT_COOKIE
		);
		fontDrawTextBitMap(s_pBfr->pBack, s_pTextBitmap, 139, uwOffsY,
			(ubPos == s_ubActivePos) ? MENU_COLOR_ACTIVE : MENU_COLOR_INACTIVE,
			FONT_COOKIE
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

static void onFadeoutStart(void) {
	stateChange(g_pStateMachineGame, &g_sStateGame);
}

static void onFadeoutExit(void) {
	gameExit();
}

static void onExit(void) {
	fadeSet(s_pFadeMenu, FADE_STATE_OUT, 50, onFadeoutExit);
}

static void menuErrorMsg(const char *szMsg) {
	char szLine[80];
	const char *szLineStart = szMsg;
	UWORD uwOffsY = 0;
	UBYTE ubLineHeight = s_pFont->uwHeight + 1;
	blitCopy(
		s_pBg, 0, uwOffsY, s_pBfr->pBack, 0, uwOffsY, 320, 2 * ubLineHeight,
		MINTERM_COPY, 0xFF
	);

	while(szLineStart) {
		const char *szLineEnd = strchr(szLineStart, '\n');
		if(szLineEnd) {
			UWORD uwLineWidth = szLineEnd - szLineStart;
			memcpy(szLine, szLineStart, uwLineWidth);
			szLine[uwLineWidth] = '\0';
			szLineStart = szLineEnd + 1;
		}
		else {
			UWORD uwLineWidth = strlen(szLineStart);
			memcpy(szLine, szLineStart, uwLineWidth);
			szLine[uwLineWidth] = '\0';
			szLineStart = 0;
		}
		fontFillTextBitMap(s_pFont, s_pTextBitmap, szLine);
		fontDrawTextBitMap(
			s_pBfr->pBack, s_pTextBitmap, 320/2, uwOffsY,
			MENU_COLOR_ERROR, FONT_COOKIE | FONT_SHADOW | FONT_HCENTER
		);
		uwOffsY += ubLineHeight;
	}
}

static void startGame(UBYTE isEditor) {
	UBYTE pSteerToPlayer[PLAYER_STEER_AI] = {0};
	for(UBYTE i = 0; i < 4; ++i) {
		if((
			s_pPlayerSteers[i] == PLAYER_STEER_JOY_3 ||
			s_pPlayerSteers[i] == PLAYER_STEER_JOY_4
		) && !joyEnableParallel()) {
			menuErrorMsg(
				"Can't open parallel port for joystick adapter\n"
				"Joy 3 & 4 are not usable!"
			);
			return;
		}

		if(s_pPlayerSteers[i] < PLAYER_STEER_AI) {
			if(!pSteerToPlayer[s_pPlayerSteers[i]]) {
				pSteerToPlayer[s_pPlayerSteers[i]] = 1;
			}
			else {
				char szMsg[80];
				sprintf(
					szMsg, "Controller %s is bound to more than 1 player",
					s_pMenuEnumSteer[s_pPlayerSteers[i]]
				);
				menuErrorMsg(szMsg);
				return;
			}
		}
	}
	gameSetEditor(isEditor);
	fadeSet(s_pFadeMenu, FADE_STATE_OUT, 50, onFadeoutStart);
}

static void onStart(void) {
	startGame(0);
}

static void onEditor(void) {
	mapDataClear(&g_sMapData);
	startGame(1);
}

static void onFadeoutToCredits(void) {
	statePush(g_pStateMachineGame, &g_sStateCredits);
}

static void onCredits(void) {
	fadeSet(s_pFadeMenu, FADE_STATE_OUT, 50, onFadeoutToCredits);
}

static void menuInitialDraw(void) {
	blitCopy(s_pBg, 0, 0, s_pBfr->pBack, 0, 0, 320, 128, MINTERM_COPY, 0xFF);
	blitCopy(s_pBg, 0, 128, s_pBfr->pBack, 0, 128, 320, 128, MINTERM_COPY, 0xFF);
	for(UBYTE ubMenuPos = 0; ubMenuPos < MENU_POS_COUNT; ++ubMenuPos) {
		s_pOptions[ubMenuPos].isDirty = 1;
	}

	char szVersion[15];
	sprintf(szVersion, "v.%d.%d.%d", BUILD_YEAR, BUILD_MONTH, BUILD_DAY);
	fontFillTextBitMap(s_pFont, s_pTextBitmap, szVersion);
	fontDrawTextBitMap(s_pBfr->pBack, s_pTextBitmap, 320/2, 256 - 35, 18, FONT_HCENTER | FONT_COOKIE);

	fontFillTextBitMap(s_pFont, s_pTextBitmap, "A game by Last Minute Creations");
	fontDrawTextBitMap(s_pBfr->pBack, s_pTextBitmap, 320/2, 256 - 20, 17, FONT_HCENTER | FONT_COOKIE);

	fontFillTextBitMap(s_pFont, s_pTextBitmap, "lastminutecreations.itch.io/germz");
	fontDrawTextBitMap(s_pBfr->pBack, s_pTextBitmap, 320/2, 256 - 10, 18, FONT_HCENTER | FONT_COOKIE);
}

static void menuGsResume(void) {
	menuInitialDraw();
	fadeSet(s_pFadeMenu, FADE_STATE_IN, 50, 0);
}

static void menuGsCreate(void) {
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
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
		TAG_SIMPLEBUFFER_IS_DBLBUF, 0,
		TAG_END
	);
	s_pBg = bitmapCreateFromFile("data/menu_main.bm", 0);
	s_pBgSub = bitmapCreateFromFile("data/menu_sub.bm", 0);

	UWORD pPalette[32];
	paletteLoad("data/germz.plt", pPalette, 32);
	s_pFadeMenu = fadeCreate(s_pView, pPalette, 32);

	s_pFont = fontCreate("data/uni54.fnt");
	s_pTextBitmap = fontCreateTextBitMap(320, s_pFont->uwHeight);
	systemUnuse();

	menuInitialDraw();
	fadeSet(s_pFadeMenu, FADE_STATE_IN, 50, 0);
	viewLoad(s_pView);
	ptplayerEnableMusic(1);
}

static void menuGsLoop(void) {
	tFadeState eFadeState = fadeProcess(s_pFadeMenu);
	if(eFadeState == FADE_STATE_EVENT_FIRED) {
		return;
	}
	if(eFadeState != FADE_STATE_OUT && keyUse(KEY_ESCAPE)) {
		fadeSet(s_pFadeMenu, FADE_STATE_OUT, 50, onFadeoutExit);
		return;
	}

	for(UBYTE ubMenuPos = 0; ubMenuPos < MENU_POS_COUNT; ++ubMenuPos) {
		if(!s_pOptions[ubMenuPos].isHidden && s_pOptions[ubMenuPos].isDirty) {
			menuDrawPos(ubMenuPos, 100);
			s_pOptions[ubMenuPos].isDirty = 0;
		}
	}

	UBYTE isEnabled34 = joyIsParallelEnabled();
	if(eFadeState != FADE_STATE_OUT) {
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
	}

	// Do this now since menuEnter may change gamestate and deallocate s_pVp
	copProcessBlocks();
	vPortWaitForEnd(s_pVp);

	if(eFadeState != FADE_STATE_OUT && (
		keyUse(KEY_RETURN) || keyUse(KEY_LSHIFT) || keyUse(KEY_RSHIFT) ||
		joyUse(JOY1_FIRE) || joyUse(JOY2_FIRE) ||
		(isEnabled34 && (joyUse(JOY3_FIRE) || joyUse(JOY4_FIRE)))
	)) {
		// menuEnter may change gamestate, so do nothing past it
		menuEnter();
	}
}

static void menuGsDestroy(void) {
	viewLoad(0);
	systemUse();
	bitmapDestroy(s_pBg);
	bitmapDestroy(s_pBgSub);
	viewDestroy(s_pView);
	fontDestroyTextBitMap(s_pTextBitmap);
	fontDestroy(s_pFont);
	fadeDestroy(s_pFadeMenu);
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

//---------------------------------------------------------------------- CREDITS

static const char *s_pCreditsLines[] = {
	"GermZ by Last Minute Creations",
	"  Graphics: Softiron",
	"  Sound & Music: Luc3k",
	"  Code: KaiN",
	"Alpha tests: Sordan, Renton, Tomu\x85",
	"",
	"Game preview released on 2020.04.25 Retronizacja 3.9 Zoom meeting.",
	"",
	"GermZ source code is available on:",
	"  github.com/Last-Minute-Creations/germz",
	"This game uses following software:",
	"- Amiga C Engine, MPL2 license (github.com/AmigaPorts/ACE)",
	"- jsmn, MIT license (github.com/zserge/jsmn)",
	"- UTF-8 parser, MIT license (bjoern.hoehrmann.de/utf-8/decoder/dfa)",
	"See links for license details, sorry for not printing them here!",
	"",
	"Thanks for playing!",
};
#define CREDITS_LINES_COUNT (sizeof(s_pCreditsLines) / sizeof(s_pCreditsLines[1]))

static void onCreditsFadeout(void) {
	statePop(g_pStateMachineGame);
}

static void creditsGsCreate(void) {
	blitCopy(s_pBgSub, 0, 0, s_pBfr->pBack, 0, 0, 320, 128, MINTERM_COPY, 0xFF);
	blitCopy(s_pBgSub, 0, 128, s_pBfr->pBack, 0, 128, 320, 128, MINTERM_COPY, 0xFF);

	UBYTE ubLineWidth = s_pFont->uwHeight + 1;
	UWORD uwOffsY = 0;
	for(UBYTE ubLine = 0; ubLine < CREDITS_LINES_COUNT; ++ubLine) {
		// Draw only non-empty lines
		if(s_pCreditsLines[ubLine][0] != '\0') {
			fontFillTextBitMap(s_pFont, s_pTextBitmap, s_pCreditsLines[ubLine]);
			fontDrawTextBitMap(
				s_pBfr->pBack, s_pTextBitmap, 0, uwOffsY, MENU_COLOR_ACTIVE,
				FONT_COOKIE | FONT_SHADOW
			);
		}

		// Advance Y pos nonetheless
		uwOffsY += ubLineWidth;
	}
	fadeSet(s_pFadeMenu, FADE_STATE_IN, 50, 0);
}

static void creditsGsLoop(void) {
	tFadeState eFadeState = fadeProcess(s_pFadeMenu);
	UBYTE isEnabled34 = joyIsParallelEnabled();
	if(eFadeState != FADE_STATE_OUT && (
		keyUse(KEY_RETURN) || keyUse(KEY_ESCAPE) ||
		keyUse(KEY_RETURN) || keyUse(KEY_LSHIFT) || keyUse(KEY_RSHIFT) ||
		joyUse(JOY1_FIRE) || joyUse(JOY2_FIRE) ||
		(isEnabled34 && (joyUse(JOY3_FIRE) || joyUse(JOY4_FIRE)))
	)) {
		// menuEnter may change gamestate, so do nothing past it
		fadeSet(s_pFadeMenu, FADE_STATE_OUT, 50, onCreditsFadeout);
	}
}

//--------------------------------------------------------------- GAMESTATE DEFS

tState g_sStateMenu = {
	.cbCreate = menuGsCreate, .cbLoop = menuGsLoop, .cbDestroy = menuGsDestroy,
	.cbResume = menuGsResume
};

tState g_sStateCredits = {.cbCreate = creditsGsCreate, .cbLoop = creditsGsLoop};

