/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_MENU_LIST_H_
#define _GERMZ_MENU_LIST_H_

#include <ace/utils/font.h>

typedef enum _tMenuListOptionType {
	MENU_LIST_OPTION_TYPE_UINT8,
	MENU_LIST_OPTION_TYPE_CALLBACK
} tMenuListOptionType;

typedef enum _tMenuListDirty {
	MENU_LIST_DIRTY_NONE = 0,
	MENU_LIST_DIRTY_VAL_CHANGE = 1,
	MENU_LIST_DIRTY_SELECTION = 2,
} tMenuListDirty;

typedef struct _tMenuListStyle {
	UBYTE ubColorActive;
	UBYTE ubColorInactive;
	UBYTE ubColorShadow;
} tMenuListStyle;

typedef void (*tOptionSelectCb)(void);
typedef void (*tOptionValChangeCb)(void);

// All options are uint8_t, enums or numbers
typedef struct _tOption {
	tMenuListOptionType eOptionType;
	UBYTE isHidden;
	tMenuListDirty eDirty;
	const tMenuListStyle *pStyle;
	union {
		struct {
			UBYTE *pVar;
			UBYTE ubMax;
			UBYTE ubDefault;
			UBYTE isCyclic;
			const char **pEnumLabels;
			tOptionValChangeCb cbOnValChange;
		} sOptUb; ///< Params for uint8-based option
		struct {
			tOptionSelectCb cbSelect;
		} sOptCb; ///< Params for callback-based option
	};
} tOption;

void menuListInit(
	tOption *pOptions, const char **pOptionCaptions,
	UBYTE ubOptionCount, tFont *pFont, tTextBitMap *pTextBitmap,
	tBitMap *pBmBg, tBitMap *pBmBuffer, UWORD uwX, UWORD uwY,
	const tMenuListStyle *pStyle
);

void menuListDraw(void);

void menuListUndraw(void);

void menuListDrawPos(UBYTE ubPos);

void menuListUndrawPos(UBYTE ubPos);

UBYTE menuListNavigate(BYTE bDir);

UBYTE menuListToggle(BYTE bDelta);

UBYTE menuListEnter(void);

void menuListHidePos(UBYTE ubPos, UBYTE isHidden);

UBYTE menuListGetActive(void);

void menuListSetActive(UBYTE ubPos);

#endif // _GERMZ_MENU_LIST_H_
