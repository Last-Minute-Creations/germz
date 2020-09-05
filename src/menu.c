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
#include "assets.h"
#include "map_list.h"
#include "menu_list.h"

#define MENU_COLOR_ACTIVE 18
#define MENU_COLOR_INACTIVE 19
#define MENU_COLOR_SHADOW 21
#define MENU_COLOR_ERROR 11

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
	"JOY 1", "JOY 2", "JOY 3", "JOY 4", "WSAD", "ARROWS", "CPU", "IDLE", "OFF"
};

static const char *s_pMenuCaptions[] = {
	"INFECT",
	"EDITOR",
	"CREDITS",
	"CURE"
};

static tView *s_pView;
static tVPort *s_pVp;
static tSimpleBufferManager *s_pBfr;
static tBitMap *s_pBg, *s_pBgSub;
static tFade *s_pFadeMenu;
static tCbFadeOnDone s_cbOnEscape;

static void onInfect(void);
static void onEditor(void);
static void onCredits(void);
static void onExit(void);

//------------------------------------------------------------------------- MENU

static tOption s_pOptions[] = {
	{MENU_LIST_OPTION_TYPE_CALLBACK, .isHidden = 0, .sOptCb = {.cbSelect = onInfect}},
	{MENU_LIST_OPTION_TYPE_CALLBACK, .isHidden = 0, .sOptCb = {.cbSelect = onEditor}},
	{MENU_LIST_OPTION_TYPE_CALLBACK, .isHidden = 0, .sOptCb = {.cbSelect = onCredits}},
	{MENU_LIST_OPTION_TYPE_CALLBACK, .isHidden = 0, .sOptCb = {.cbSelect = onExit}},
};
#define MENU_POS_COUNT (sizeof(s_pOptions) / sizeof(s_pOptions[0]))

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
	UBYTE ubLineHeight = g_pFontSmall->uwHeight + 1;
	blitCopy(
		s_pBg, 0, uwOffsY, s_pBfr->pBack, 0, uwOffsY, 320, 2 * ubLineHeight,
		MINTERM_COPY
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
		fontFillTextBitMap(g_pFontSmall, g_pTextBitmap, szLine);
		fontDrawTextBitMap(
			s_pBfr->pBack, g_pTextBitmap, 320/2, uwOffsY,
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

static void onEditor(void) {
	mapDataClear(&g_sMapData);
	startGame(1);
}

static void onFadeoutToCredits(void) {
	statePush(g_pStateMachineGame, &g_sStateCredits);
}

static void onFadeoutToInfect(void) {
	statePush(g_pStateMachineGame, &g_sStateInfect);
}

static void onInfect(void) {
	fadeSet(s_pFadeMenu, FADE_STATE_OUT, 50, onFadeoutToInfect);
}

static void onCredits(void) {
	fadeSet(s_pFadeMenu, FADE_STATE_OUT, 50, onFadeoutToCredits);
}

static void menuInitialDraw(void) {
	blitCopy(s_pBg, 0, 0, s_pBfr->pBack, 0, 0, 320, 128, MINTERM_COPY);
	blitCopy(s_pBg, 0, 128, s_pBfr->pBack, 0, 128, 320, 128, MINTERM_COPY);
	menuListInit(
		s_pOptions, s_pMenuCaptions, MENU_POS_COUNT,
		g_pFontBig, g_pTextBitmap, s_pBg, s_pBfr->pBack, 120, 100,
		MENU_COLOR_ACTIVE, MENU_COLOR_INACTIVE, MENU_COLOR_SHADOW
	);

	char szVersion[15];
	sprintf(szVersion, "V.%d.%d.%d", BUILD_YEAR, BUILD_MONTH, BUILD_DAY);
	fontFillTextBitMap(g_pFontSmall, g_pTextBitmap, szVersion);
	fontDrawTextBitMap(
		s_pBfr->pBack, g_pTextBitmap, 320, 0, MENU_COLOR_INACTIVE,
		FONT_RIGHT | FONT_COOKIE
	);

	fontFillTextBitMap(g_pFontSmall, g_pTextBitmap, "A game by Last Minute Creations");
	fontDrawTextBitMap(s_pBfr->pBack, g_pTextBitmap, 320/2, 256 - 20, MENU_COLOR_INACTIVE, FONT_HCENTER | FONT_COOKIE);

	fontFillTextBitMap(g_pFontSmall, g_pTextBitmap, "lastminutecreations.itch.io/germz");
	fontDrawTextBitMap(s_pBfr->pBack, g_pTextBitmap, 320/2, 256 - 10, MENU_COLOR_INACTIVE, FONT_HCENTER | FONT_COOKIE);
}

static void menuGsResume(void) {
	menuInitialDraw();
	fadeSet(s_pFadeMenu, FADE_STATE_IN, 50, 0);
}

static UBYTE menuProcessList(UBYTE isEnabled34) {
	UBYTE isEarlyReturn = 0;
	if(
		keyUse(KEY_UP) || keyUse(KEY_W) ||
		joyUse(JOY1_UP) || joyUse(JOY2_UP) ||
		(isEnabled34 && (joyUse(JOY3_UP) || joyUse(JOY4_UP)))
	) {
		menuListNavigate(-1);
	}
	else if(
		keyUse(KEY_DOWN) || keyUse(KEY_S) ||
		joyUse(JOY1_DOWN) || joyUse(JOY2_DOWN) ||
		(isEnabled34 && (joyUse(JOY3_DOWN) || joyUse(JOY4_DOWN)))
	) {
		menuListNavigate(+1);
	}
	else if(
		keyUse(KEY_LEFT) || keyUse(KEY_A) ||
		joyUse(JOY1_LEFT) || joyUse(JOY2_LEFT) ||
		(isEnabled34 && (joyUse(JOY3_LEFT) || joyUse(JOY4_LEFT)))
	) {
		menuListToggle(-1);
	}
	else if(
		keyUse(KEY_RIGHT) || keyUse(KEY_D) ||
		joyUse(JOY1_RIGHT) || joyUse(JOY2_RIGHT) ||
		(isEnabled34 && (joyUse(JOY3_RIGHT) || joyUse(JOY4_RIGHT)))
	) {
		menuListToggle(+1);
	}
	else 	if(
		keyUse(KEY_RETURN) || keyUse(KEY_LSHIFT) || keyUse(KEY_RSHIFT) ||
		joyUse(JOY1_FIRE) || joyUse(JOY2_FIRE) ||
		(isEnabled34 && (joyUse(JOY3_FIRE) || joyUse(JOY4_FIRE)))
	) {
		// menuEnter may change gamestate, so do nothing past it
		menuListEnter();
		isEarlyReturn = 1;
	}
	return isEarlyReturn;
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

	systemUnuse();

	menuInitialDraw();
	fadeSet(s_pFadeMenu, FADE_STATE_IN, 50, 0);
	viewLoad(s_pView);
	ptplayerEnableMusic(1);

	s_cbOnEscape = onFadeoutExit;
}

static void menuGsLoop(void) {
	tFadeState eFadeState = fadeProcess(s_pFadeMenu);
	if(eFadeState == FADE_STATE_EVENT_FIRED) {
		return;
	}
	if(eFadeState != FADE_STATE_OUT && keyUse(KEY_ESCAPE)) {
		fadeSet(s_pFadeMenu, FADE_STATE_OUT, 50, s_cbOnEscape);
		return;
	}

	menuListDraw();

	UBYTE isEnabled34 = joyIsParallelEnabled();
	if(eFadeState != FADE_STATE_OUT) {
		if(menuProcessList(isEnabled34)) {
			// menuEnter may change gamestate and deallocate s_pVp
			return;
		}
	}

	copProcessBlocks();
	vPortWaitForEnd(s_pVp);
}

static void menuGsDestroy(void) {
	viewLoad(0);
	systemUse();
	bitmapDestroy(s_pBg);
	bitmapDestroy(s_pBgSub);
	viewDestroy(s_pView);
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

static void onSubmenuFadeout(void) {
	statePop(g_pStateMachineGame);
}

//----------------------------------------------------------------------- INFECT

static void onStart(void) {
	startGame(0);
}

static void onMap(void) {

}

static void onBack(void) {
	fadeSet(s_pFadeMenu, FADE_STATE_OUT, 50, onSubmenuFadeout);
}

static const char *s_pMenuInfectCaptions[] = {
	"START",
	"SELECT MAP",
	"PLAYER 1",
	"PLAYER 2",
	"PLAYER 3",
	"PLAYER 4",
	"BACK"
};

static tOption s_pOptionsInfect[] = {
	{MENU_LIST_OPTION_TYPE_CALLBACK, .isHidden = 0, .sOptCb = {.cbSelect = onStart}},
	{MENU_LIST_OPTION_TYPE_CALLBACK, .isHidden = 0, .sOptCb = {.cbSelect = onMap}},
	{MENU_LIST_OPTION_TYPE_UINT8, .isHidden = 0, .sOptUb = {
		.pVar = &s_pPlayerSteers[0], .ubMax = PLAYER_STEER_IDLE, .isCyclic = 1,
		.ubDefault = PLAYER_STEER_JOY_1, .pEnumLabels = s_pMenuEnumSteer
	}},
	{MENU_LIST_OPTION_TYPE_UINT8, .isHidden = 0, .sOptUb = {
		.pVar = &s_pPlayerSteers[1], .ubMax = PLAYER_STEER_IDLE, .isCyclic = 1,
		.ubDefault = PLAYER_STEER_JOY_2, .pEnumLabels = s_pMenuEnumSteer
	}},
	{MENU_LIST_OPTION_TYPE_UINT8, .isHidden = 0, .sOptUb = {
		.pVar = &s_pPlayerSteers[2], .ubMax = PLAYER_STEER_IDLE, .isCyclic = 1,
		.ubDefault = PLAYER_STEER_OFF, .pEnumLabels = s_pMenuEnumSteer
	}},
	{MENU_LIST_OPTION_TYPE_UINT8, .isHidden = 0, .sOptUb = {
		.pVar = &s_pPlayerSteers[3], .ubMax = PLAYER_STEER_IDLE, .isCyclic = 1,
		.ubDefault = PLAYER_STEER_OFF, .pEnumLabels = s_pMenuEnumSteer
	}},
	{MENU_LIST_OPTION_TYPE_CALLBACK, .isHidden = 0, .sOptCb = {.cbSelect = onBack}},
};
#define INFECT_MENU_POS_COUNT (sizeof(s_pOptionsInfect) / sizeof(s_pOptionsInfect[0]))

static tListCtl *s_pMapList;

static void infectGsCreate(void) {
	blitCopy(s_pBgSub, 0, 0, s_pBfr->pBack, 0, 0, 320, 128, MINTERM_COPY);
	blitCopy(s_pBgSub, 0, 128, s_pBfr->pBack, 0, 128, 320, 128, MINTERM_COPY);
	menuListInit(
		s_pOptionsInfect, s_pMenuInfectCaptions, INFECT_MENU_POS_COUNT,
		g_pFontBig, g_pTextBitmap, s_pBgSub, s_pBfr->pBack, 80, 140,
		MENU_COLOR_ACTIVE, MENU_COLOR_INACTIVE, MENU_COLOR_SHADOW
	);

	s_pMapList = mapListCreateCtl(s_pBfr->pBack, 5, 5, 160, 128);
	listCtlDraw(s_pMapList);
	mapListDrawPreview(
		&g_sMapData, s_pBfr->pBack, 165 + ((320 - 165) - 16 * 4) / 2, 50
	);

	fadeSet(s_pFadeMenu, FADE_STATE_IN, 50, 0);

	s_cbOnEscape = onSubmenuFadeout;
}

static void infectGsDestroy(void) {
	listCtlDestroy(s_pMapList);
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
	"",
	"This game uses following open source code:",
	"- Amiga C Engine, MPL2 license (github.com/AmigaPorts/ACE)",
	"- jsmn, MIT license (github.com/zserge/jsmn)",
	"- UTF-8 parser, MIT license (bjoern.hoehrmann.de/utf-8/decoder/dfa)",
	"See links for license details, sorry for not printing them here!",
	"",
	"Thanks for playing!",
};
#define CREDITS_LINES_COUNT (sizeof(s_pCreditsLines) / sizeof(s_pCreditsLines[1]))

static void creditsGsCreate(void) {
	blitCopy(s_pBgSub, 0, 0, s_pBfr->pBack, 0, 0, 320, 128, MINTERM_COPY);
	blitCopy(s_pBgSub, 0, 128, s_pBfr->pBack, 0, 128, 320, 128, MINTERM_COPY);

	UBYTE ubLineWidth = g_pFontSmall->uwHeight + 1;
	UWORD uwOffsY = 0;
	for(UBYTE ubLine = 0; ubLine < CREDITS_LINES_COUNT; ++ubLine) {
		// Draw only non-empty lines
		if(s_pCreditsLines[ubLine][0] != '\0') {
			fontFillTextBitMap(g_pFontSmall, g_pTextBitmap, s_pCreditsLines[ubLine]);
			fontDrawTextBitMap(
				s_pBfr->pBack, g_pTextBitmap, 0, uwOffsY, MENU_COLOR_ACTIVE,
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
		onBack();
	}

	copProcessBlocks();
	vPortWaitForEnd(s_pVp);
}

//--------------------------------------------------------------- GAMESTATE DEFS

tState g_sStateMenu = {
	.cbCreate = menuGsCreate, .cbLoop = menuGsLoop, .cbDestroy = menuGsDestroy,
	.cbResume = menuGsResume
};

tState g_sStateCredits = {.cbCreate = creditsGsCreate, .cbLoop = creditsGsLoop};
tState g_sStateInfect = {
	.cbCreate = infectGsCreate, .cbLoop = menuGsLoop,
	.cbDestroy = infectGsDestroy
};

