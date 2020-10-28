/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GUI_CONFIG_H_
#define _GUI_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ace/types.h>

typedef enum _tFillStyle {
	FILL_STYLE_NONE,
	FILL_STYLE_FLAT,
	FILL_STYLE_3D
} tFillStyle;

typedef struct _tGuiConfig {
	UBYTE ubColorLight;
	UBYTE ubColorDark;
	UBYTE ubColorFill;
	UBYTE ubColorText;
	tFillStyle eFill;
	// TODO move font here instead of params
} tGuiConfig;

tGuiConfig *guiGetConfig(void);

#ifdef __cplusplus
}
#endif

#endif // _GUI_CONFIG_H_
