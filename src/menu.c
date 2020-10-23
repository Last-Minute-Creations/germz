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
#include <ace/utils/bmframe.h>
#include "game.h"
#include "build_ver.h"
#include "fade.h"
#include "germz.h"
#include "assets.h"
#include "map_list.h"
#include "menu_list.h"
#include "gui/button.h"

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

static const tMenuListStyle s_sMenuStyleDefault = {
	.ubColorActive = MENU_COLOR_ACTIVE,
	.ubColorInactive = MENU_COLOR_INACTIVE,
	.ubColorShadow = MENU_COLOR_SHADOW
};

static const tMenuListStyle s_pMenuStylePlayers[4] = {
	{.ubColorActive = 10, .ubColorInactive = 11, .ubColorShadow = 13},
	{.ubColorActive = 14, .ubColorInactive = 15, .ubColorShadow = 17},
	{.ubColorActive = 18, .ubColorInactive = 19, .ubColorShadow = 21},
	{.ubColorActive = 22, .ubColorInactive = 23, .ubColorShadow = 25}
};

static const char *s_pMenuEnumSteer[] = {
	"JOY 1", "JOY 2", "JOY 3", "JOY 4", "WSAD", "ARROWS", "CPU", "IDLE", "OFF"
};

static const char *s_pMenuCaptions[] = {
	"CAMPAIGN",
	"BATTLE",
	"EDITOR",
	"CREDITS",
	"CURE"
};

static tView *s_pView;
static tVPort *s_pVp;
static tSimpleBufferManager *s_pBfr;
static tBitMap *s_pBg, *s_pBgSub, *s_pFrameDisplay, *s_pBgCurr;
static tFade *s_pFadeMenu;
static tCbFadeOnDone s_cbOnEscape;

static tStateManager *s_pStateMachineMenu;
static tState s_sStateMain, s_sStateBattle, s_sStateCredits, s_sStateSteer;

//------------------------------------------------------------------ PRIVATE FNS

static void onFadeoutEditorGameStart(void) {
	stateChange(g_pStateMachineGame, &g_sStateGame);
}

static void onFadeoutGameStart(void) {
	statePop(g_pStateMachineGame); // Pop from map select to main menu
	stateChange(g_pStateMachineGame, &g_sStateGame);
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
		fontDrawStr(
			g_pFontSmall, s_pBfr->pBack, 320/2, uwOffsY, szLine, MENU_COLOR_ERROR,
			FONT_COOKIE | FONT_SHADOW | FONT_HCENTER, g_pTextBitmap
		);
		uwOffsY += ubLineHeight;
	}
}

static void startGame(UBYTE isEditor, UBYTE ubPlayerMask) {
	gameSetEditor(isEditor);
	if(isEditor) {
		fadeSet(s_pFadeMenu, FADE_STATE_OUT, 50, onFadeoutEditorGameStart);
	}
	else {
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

			if(s_pPlayerSteers[i] < PLAYER_STEER_AI && BTST(ubPlayerMask, i)) {
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
		fadeSet(s_pFadeMenu, FADE_STATE_OUT, 50, onFadeoutGameStart);
	}
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
	s_pBgCurr = bitmapCreate(320, 256, 5, BMF_INTERLEAVED);
	s_pFrameDisplay = bitmapCreateFromFile("data/display.bm", 0);


	UWORD pPalette[32];
	paletteLoad("data/germz.plt", pPalette, 32);
	s_pFadeMenu = fadeCreate(s_pView, pPalette, 32);

	s_pStateMachineMenu = stateManagerCreate();
	systemUnuse();
	stateChange(s_pStateMachineMenu, &s_sStateMain);

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
		s_cbOnEscape();
		return;
	}
	if(eFadeState != FADE_STATE_OUT) {
		stateProcess(s_pStateMachineMenu);
	}

	copProcessBlocks();
	vPortWaitForEnd(s_pVp);
}

static void menuGsDestroy(void) {
	viewLoad(0);
	systemUse();
	stateManagerDestroy(s_pStateMachineMenu);
	bitmapDestroy(s_pBg);
	bitmapDestroy(s_pBgSub);
	bitmapDestroy(s_pBgCurr);
	bitmapDestroy(s_pFrameDisplay);
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

//------------------------------------------------------------- SUBSTATE: COMMON

// It needs to work on state changing instead of  pushing/popping since it needs
// to redraw each screen on transition.
static tState *s_pNextSubstate;

static void onFadeToSubstate(void) {
	stateChange(s_pStateMachineMenu, s_pNextSubstate);
}

static void fadeToSubstate(tState *pNextSubstate) {
	s_pNextSubstate = pNextSubstate;
	fadeSet(s_pFadeMenu, FADE_STATE_OUT, 50, onFadeToSubstate);
}

static void onCampaign(void) {

}

static void onEditor(void) {
	mapDataClear(&g_sMapData);
	startGame(1, 1);
}

static void onBattle(void) {
	fadeToSubstate(&s_sStateBattle);
}

static void onCredits(void) {
	fadeToSubstate(&s_sStateCredits);
}

static void onFadeoutToExit(void) {
	gameExit();
}

static void fadeToExit(void) {
	fadeSet(s_pFadeMenu, FADE_STATE_OUT, 50, onFadeoutToExit);
}

static tOption s_pOptions[] = {
	{MENU_LIST_OPTION_TYPE_CALLBACK, .isHidden = 0, .sOptCb = {.cbSelect = onCampaign}},
	{MENU_LIST_OPTION_TYPE_CALLBACK, .isHidden = 0, .sOptCb = {.cbSelect = onBattle}},
	{MENU_LIST_OPTION_TYPE_CALLBACK, .isHidden = 0, .sOptCb = {.cbSelect = onEditor}},
	{MENU_LIST_OPTION_TYPE_CALLBACK, .isHidden = 0, .sOptCb = {.cbSelect = onCredits}},
	{MENU_LIST_OPTION_TYPE_CALLBACK, .isHidden = 0, .sOptCb = {.cbSelect = fadeToExit}},
};
#define MENU_POS_COUNT (sizeof(s_pOptions) / sizeof(s_pOptions[0]))

static void fadeToMain(void) {
	fadeToSubstate(&s_sStateMain);
}

static void fadeToBattle(void) {
	fadeToSubstate(&s_sStateBattle);
}

static void menuSubstateLoop(void) {
	menuListDraw();

	UBYTE isEnabled34 = joyIsParallelEnabled();
	if(menuProcessList(isEnabled34)) {
		// menuEnter may change gamestate and deallocate s_pVp
		return;
	}
}

//-------------------------------------------------------------- SUBSTATE: STEER

#define STEER_MENU_OPTION_MAX 8
static const char *s_pMenuCaptionsSteer[STEER_MENU_OPTION_MAX];
static tOption s_pOptionsSteer[STEER_MENU_OPTION_MAX];
static UBYTE s_ubSteerOptionCount;

static void onStart(void) {
	startGame(0, g_sMapData.ubPlayerMask);
}

static void menuSteerGsCreate(void) {
	static const char *pPlayerLabels[] = {
		"PLAYER 1", "PLAYER 2", "PLAYER 3", "PLAYER 4"
	};

	// Prepare current bg
	blitCopy(s_pBgSub, 0, 0, s_pBgCurr, 0, 0, 320, 128, MINTERM_COPY);
	blitCopy(s_pBgSub, 0, 128, s_pBgCurr, 0, 128, 320, 128, MINTERM_COPY);
	bmFrameDraw(s_pFrameDisplay, s_pBgCurr, 48, 48, 14, 10, 16);

	// Copy current bg to vport's bitmap
	blitCopy(s_pBgCurr, 0, 0, s_pBfr->pBack, 0, 0, 320, 128, MINTERM_COPY);
	blitCopy(s_pBgCurr, 0, 128, s_pBfr->pBack, 0, 128, 320, 128, MINTERM_COPY);

	// Infect
	s_ubSteerOptionCount = 0;
	s_pOptionsSteer[s_ubSteerOptionCount] = (tOption){
		MENU_LIST_OPTION_TYPE_CALLBACK, .isHidden = 0, .sOptCb = {.cbSelect = onStart}
	};
	s_pMenuCaptionsSteer[s_ubSteerOptionCount++] = "INFECT";

	// Dummy
	s_pOptionsSteer[s_ubSteerOptionCount++] = (tOption){.isHidden = 1};

	// Players
	for(UBYTE i = 0; i < 4; ++i) {
		if(BTST(g_sMapData.ubPlayerMask, i)) {
			s_pOptionsSteer[s_ubSteerOptionCount] = (tOption){
				MENU_LIST_OPTION_TYPE_UINT8, .isHidden = 0, .sOptUb = {
				.pVar = &s_pPlayerSteers[i], .ubMax = PLAYER_STEER_IDLE, .isCyclic = 1,
				.ubDefault = PLAYER_STEER_JOY_1, .pEnumLabels = s_pMenuEnumSteer
			}, .pStyle = &s_pMenuStylePlayers[i]};
			s_pMenuCaptionsSteer[s_ubSteerOptionCount++] = pPlayerLabels[i];
		}
	}

	// Dummy
	s_pOptionsSteer[s_ubSteerOptionCount++] = (tOption){.isHidden = 1};

	// Back
	s_pOptionsSteer[s_ubSteerOptionCount] = (tOption){
		MENU_LIST_OPTION_TYPE_CALLBACK, .isHidden = 0,
		.sOptCb = {.cbSelect = fadeToBattle}
	};
	s_pMenuCaptionsSteer[s_ubSteerOptionCount++] = "BACK";

	menuListInit(
		s_pOptionsSteer, s_pMenuCaptionsSteer, s_ubSteerOptionCount,
		g_pFontBig, g_pTextBitmap, s_pBgCurr, s_pBfr->pBack, 64, 64,
		&s_sMenuStyleDefault
	);

	fadeSet(s_pFadeMenu, FADE_STATE_IN, 50, 0);
	s_cbOnEscape = fadeToBattle;
}

//------------------------------------------------------------- SUBSTATE: BATTLE

#define BATTLE_MENU_OPTION_MAX 6

static ULONG s_ullChangeTimer;
static const char *s_pMenuBattleCaptions[BATTLE_MENU_OPTION_MAX];
static tOption s_pOptionsBattle[BATTLE_MENU_OPTION_MAX];
static UBYTE s_ubBattleOptionCount;
static tListCtl *s_pMapList;
static const char *s_pLabelsMode[] = {"SOLO", "TEAMS"};
static const char *s_pLabelsTeam1[] = {"", "", ""}; // P1+P2, P1+P3, P1+P4
static const char *s_pLabelsTeam2[] = {"", "", ""}; // P3+P4, P2+P4, P2+P3
static UBYTE s_ubBattleMode;
static UBYTE s_ubTeamCfg;
static UBYTE s_ubOptionIdxTeam1, s_ubOptionIdxTeam2;

static void battleGsLoopMapSelect(void);

static void onBattleGoToSteer(void) {
	fadeToSubstate(&s_sStateSteer);
}

static void onMap(void) {
	s_sStateBattle.cbLoop = battleGsLoopMapSelect;
}

static void onModeChange(void) {
	if(s_ubBattleMode) {
		// Teams - allow selecting
		s_pOptionsBattle[s_ubOptionIdxTeam1].isHidden = 0;
		s_pOptionsBattle[s_ubOptionIdxTeam2].isHidden = 0;
	}
	else {
		// Solo - hide
		s_pOptionsBattle[s_ubOptionIdxTeam1].isHidden = 1;
		s_pOptionsBattle[s_ubOptionIdxTeam2].isHidden = 1;
	}
	s_pOptionsBattle[s_ubOptionIdxTeam1].eDirty = MENU_LIST_DIRTY_VAL_CHANGE;
	s_pOptionsBattle[s_ubOptionIdxTeam2].eDirty = MENU_LIST_DIRTY_VAL_CHANGE;
}

static void onTeamChange(void) {
	// Team changed - mark both for redraw since it's the same var
	s_pOptionsBattle[s_ubOptionIdxTeam1].eDirty = MENU_LIST_DIRTY_VAL_CHANGE;
	s_pOptionsBattle[s_ubOptionIdxTeam2].eDirty = MENU_LIST_DIRTY_VAL_CHANGE;
}

static void onTeamDraw(UBYTE ubIdx) {
	static const UBYTE pComboA[][2] = {{0, 1}, {0, 2}, {0, 3}}; // P1+P2, P1+P3, P1+P4
	static const UBYTE pComboB[][2] = {{2, 3}, {1, 3}, {1, 2}}; // P3+P4, P2+P4, P2+P3
	static const char *pPlayerNames[] = {"P1", "P2", "P3", "P4"};

	UBYTE ubActiveIdx = menuListGetActive();
	const UBYTE (*pCombo)[2] = (ubIdx == s_ubOptionIdxTeam1 ? pComboA : pComboB);

	UWORD uwX = 80 + 80;
	UWORD uwY = 133 + ubIdx * (g_pFontBig->uwHeight + 1); // HACK HACK HACK
	UBYTE ubPlayerX = pCombo[s_ubTeamCfg][0];
	UBYTE ubPlayerY = pCombo[s_ubTeamCfg][1];
	UBYTE ubColor = (
		ubActiveIdx == ubIdx ?
		s_pMenuStylePlayers[ubPlayerX].ubColorActive :
		s_pMenuStylePlayers[ubPlayerX].ubColorInactive
	);
	fontDrawStr(
		g_pFontBig, s_pBfr->pBack, uwX, uwY, pPlayerNames[ubPlayerX],
		ubColor, FONT_COOKIE, g_pTextBitmap
	);

	ubColor = (
		ubActiveIdx == ubIdx ?
		s_pMenuStylePlayers[ubPlayerY].ubColorActive :
		s_pMenuStylePlayers[ubPlayerY].ubColorInactive
	);
	fontDrawStr(
		g_pFontBig, s_pBfr->pBack, uwX + 30, uwY, pPlayerNames[ubPlayerY],
		ubColor, FONT_COOKIE, g_pTextBitmap
	);
}

static void battleRegenMenuList(UBYTE ubPlayerMask) {
	s_ubBattleOptionCount = 0;

	// Infect
	s_pOptionsBattle[s_ubBattleOptionCount] = (tOption){
		MENU_LIST_OPTION_TYPE_CALLBACK, .isHidden = 0, .sOptCb = {.cbSelect = onBattleGoToSteer}
	};
	s_pMenuBattleCaptions[s_ubBattleOptionCount++] = "INFECT";

	// Select map
	s_pOptionsBattle[s_ubBattleOptionCount] = (tOption){
		.eOptionType = MENU_LIST_OPTION_TYPE_CALLBACK, .isHidden = 0,
		.sOptCb = {.cbSelect = onMap}
	};
	s_pMenuBattleCaptions[s_ubBattleOptionCount++] = "SELECT MAP";

	// Game mode
	UBYTE isTeamsAllowed = (g_sMapData.ubPlayerMask == 0xF);
	s_pOptionsBattle[s_ubBattleOptionCount] = (tOption){
		.eOptionType = MENU_LIST_OPTION_TYPE_UINT8, .isHidden = 0, .sOptUb = {
			.isCyclic = 1, .pEnumLabels = s_pLabelsMode, .pVar = &s_ubBattleMode,
			.ubMax = 0, .ubDefault = 0, .cbOnValChange = onModeChange
		}
	};
	if(isTeamsAllowed) {
		// All players are available - allow team mode
		s_pOptionsBattle[s_ubBattleOptionCount].sOptUb.ubMax = 1;
	}
	s_pMenuBattleCaptions[s_ubBattleOptionCount++] = "GAME MODE";

	// Team 1
	s_ubOptionIdxTeam1 = s_ubBattleOptionCount;
	s_pOptionsBattle[s_ubBattleOptionCount] = (tOption){
		.eOptionType = MENU_LIST_OPTION_TYPE_UINT8, .isHidden = 1, .sOptUb = {
			.isCyclic = 1, .pEnumLabels = s_pLabelsTeam1, .pVar = &s_ubTeamCfg,
			.ubDefault = 0, .ubMax = 2, .cbOnValChange = onTeamChange,
			.cbOnValDraw = onTeamDraw
		}
	};
	if(isTeamsAllowed && s_ubBattleMode) {
		s_pOptionsBattle[s_ubBattleOptionCount].isHidden = 0;
	}
	s_pMenuBattleCaptions[s_ubBattleOptionCount++] = "TEAM 1";

	// Team 2
	s_ubOptionIdxTeam2 = s_ubBattleOptionCount;
	s_pOptionsBattle[s_ubBattleOptionCount] = (tOption){
		.eOptionType = MENU_LIST_OPTION_TYPE_UINT8, .isHidden = 1, .sOptUb = {
			.isCyclic = 1, .pEnumLabels = s_pLabelsTeam2, .pVar = &s_ubTeamCfg,
			.ubDefault = 0, .ubMax = 2, .cbOnValChange = onTeamChange,
			.cbOnValDraw = onTeamDraw
		}
	};
	if(isTeamsAllowed && s_ubBattleMode) {
		s_pOptionsBattle[s_ubBattleOptionCount].isHidden = 0;
	}
	s_pMenuBattleCaptions[s_ubBattleOptionCount++] = "TEAM 2";

	// Back
	s_pOptionsBattle[s_ubBattleOptionCount] = (tOption){
		MENU_LIST_OPTION_TYPE_CALLBACK, .isHidden = 0, .sOptCb = {.cbSelect = fadeToMain}
	};
	s_pMenuBattleCaptions[s_ubBattleOptionCount++] = "BACK";
}

static void menuUpdateMapInfo(UBYTE isUndraw, UBYTE isForce) {
	if(updateMapInfo(
		s_pMapList, s_pBgCurr, s_pBfr->pBack, &g_sMapData, 6
	) || isForce) {
		if(isUndraw) {
			menuListUndraw();
		}
		battleRegenMenuList(g_sMapData.ubPlayerMask);
		UBYTE ubActive = menuListGetActive();
		menuListInit(
			s_pOptionsBattle, s_pMenuBattleCaptions, s_ubBattleOptionCount,
			g_pFontBig, g_pTextBitmap, s_pBgCurr, s_pBfr->pBack, 80, 133,
			&s_sMenuStyleDefault
		);
		menuListSetActive(ubActive);
		menuListDraw();
	}
}

static void battleGsCreate(void) {
	// Prepare current bg
	blitCopy(s_pBgSub, 0, 0, s_pBgCurr, 0, 0, 320, 128, MINTERM_COPY);
	blitCopy(s_pBgSub, 0, 128, s_pBgCurr, 0, 128, 320, 128, MINTERM_COPY);
	bmFrameDraw(s_pFrameDisplay, s_pBgCurr, 32, 16, 16, 14, 16);

	// Copy current bg to vport's bitmap
	blitCopy(s_pBgCurr, 0, 0, s_pBfr->pBack, 0, 0, 320, 128, MINTERM_COPY);
	blitCopy(s_pBgCurr, 0, 128, s_pBfr->pBack, 0, 128, 320, 128, MINTERM_COPY);

	buttonListCreate(5, s_pBfr->pBack, g_pFontSmall, g_pTextBitmap);
	s_pMapList = mapListCreateCtl(s_pBfr->pBack, 45, 29, 120, 75);
	listCtlDraw(s_pMapList);
	buttonDrawAll();

	fadeSet(s_pFadeMenu, FADE_STATE_IN, 50, 0);

	s_cbOnEscape = fadeToMain;
	menuListSetActive(1);
	menuUpdateMapInfo(0, 1);
	s_ullChangeTimer = timerGet();
}

static void battleGsLoopMapSelect(void) {
	UBYTE isMapSelected = 0;
	UBYTE isEnabled34 = joyIsParallelEnabled();

	if(
		keyUse(KEY_UP) || keyUse(KEY_W) ||
		joyUse(JOY1_UP) || joyUse(JOY2_UP) ||
		(isEnabled34 && (joyUse(JOY3_UP) || joyUse(JOY4_UP)))
	) {
		listCtlSelectPrev(s_pMapList);
		clearMapInfo(s_pMapList, s_pBgCurr, s_pBfr->pBack);
		s_ullChangeTimer = timerGet();
	}
	else if(
		keyUse(KEY_DOWN) || keyUse(KEY_S) ||
		joyUse(JOY1_DOWN) || joyUse(JOY2_DOWN) ||
		(isEnabled34 && (joyUse(JOY3_DOWN) || joyUse(JOY4_DOWN)))
	) {
		listCtlSelectNext(s_pMapList);
		clearMapInfo(s_pMapList, s_pBgCurr, s_pBfr->pBack);
		s_ullChangeTimer = timerGet();
	}
	else if(
		keyUse(KEY_RETURN) || keyUse(KEY_LSHIFT) || keyUse(KEY_RSHIFT) ||
		joyUse(JOY1_FIRE) || joyUse(JOY2_FIRE) ||
		(isEnabled34 && (joyUse(JOY3_FIRE) || joyUse(JOY4_FIRE)))
	) {
		menuUpdateMapInfo(1, 0);
		isMapSelected = 1;
	}

	if(timerGetDelta(s_ullChangeTimer, timerGet()) >= 25) {
		menuUpdateMapInfo(1, 0);
		s_ullChangeTimer = timerGet();
	}

	if(isMapSelected || keyUse(KEY_ESCAPE)) {
		s_sStateBattle.cbLoop = menuSubstateLoop;
	}
}

static void battleGsDestroy(void) {
	listCtlDestroy(s_pMapList);
	buttonListDestroy();
}

//------------------------------------------------------------ SUBSTATE: CREDITS

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
			fontDrawStr(
				g_pFontSmall, s_pBfr->pBack, 0, uwOffsY, s_pCreditsLines[ubLine],
				MENU_COLOR_ACTIVE, FONT_COOKIE | FONT_SHADOW, g_pTextBitmap
			);
		}

		// Advance Y pos nonetheless
		uwOffsY += ubLineWidth;
	}
	fadeSet(s_pFadeMenu, FADE_STATE_IN, 50, 0);
	s_cbOnEscape = fadeToMain;
}

static void creditsGsLoop(void) {
	tFadeState eFadeState = fadeProcess(s_pFadeMenu);

	UBYTE isEnabled34 = joyIsParallelEnabled();
	if(eFadeState != FADE_STATE_OUT && (
		keyUse(KEY_RETURN) || keyUse(KEY_LSHIFT) || keyUse(KEY_RSHIFT) ||
		joyUse(JOY1_FIRE) || joyUse(JOY2_FIRE) ||
		(isEnabled34 && (joyUse(JOY3_FIRE) || joyUse(JOY4_FIRE)))
	)) {
		// menuEnter may change gamestate, so do nothing past it
		fadeToMain();
	}

	copProcessBlocks();
	vPortWaitForEnd(s_pVp);
}

//---------------------------------------------------------- SUBSTATE: MAIN MENU

static void mainGsCreate(void) {
	blitCopy(s_pBg, 0, 0, s_pBfr->pBack, 0, 0, 320, 128, MINTERM_COPY);
	blitCopy(s_pBg, 0, 128, s_pBfr->pBack, 0, 128, 320, 128, MINTERM_COPY);

	menuListInit(
		s_pOptions, s_pMenuCaptions, MENU_POS_COUNT,
		g_pFontBig, g_pTextBitmap, s_pBg, s_pBfr->pBack, 120, 120,
		&s_sMenuStyleDefault
	);

	char szVersion[15];
	sprintf(szVersion, "V.%d.%d.%d", BUILD_YEAR, BUILD_MONTH, BUILD_DAY);
	fontDrawStr(
		g_pFontSmall, s_pBfr->pBack, 320, 0, szVersion,
		MENU_COLOR_INACTIVE, FONT_RIGHT | FONT_COOKIE, g_pTextBitmap
	);

	fontDrawStr(
		g_pFontSmall, s_pBfr->pBack, 320/2, 256 - 20,
		"A game by Last Minute Creations",
		MENU_COLOR_INACTIVE, FONT_HCENTER | FONT_COOKIE, g_pTextBitmap
	);

	fontDrawStr(
		g_pFontSmall, s_pBfr->pBack, 320/2, 256 - 10,
		"lastminutecreations.itch.io/germz",
		MENU_COLOR_INACTIVE, FONT_HCENTER | FONT_COOKIE, g_pTextBitmap
	);

	s_cbOnEscape = fadeToExit;
	fadeSet(s_pFadeMenu, FADE_STATE_IN, 50, 0);
}

//--------------------------------------------------------------- GAMESTATE DEFS

tState g_sStateMenu = {
	.cbCreate = menuGsCreate, .cbLoop = menuGsLoop, .cbDestroy = menuGsDestroy
};

static tState s_sStateCredits = {
	.cbCreate = creditsGsCreate, .cbLoop = creditsGsLoop
};

static tState s_sStateBattle = {
	.cbCreate = battleGsCreate, .cbLoop = menuSubstateLoop,
	.cbDestroy = battleGsDestroy
};

static tState s_sStateMain = {
	.cbCreate = mainGsCreate, .cbLoop = menuSubstateLoop
};

static tState s_sStateSteer = {
	.cbCreate = menuSteerGsCreate, .cbLoop = menuSubstateLoop
};
