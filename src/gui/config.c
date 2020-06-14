#include "config.h"

tGuiConfig s_sConfig = {
	.ubColorLight = 12,
	.ubColorDark = 3,
	.ubColorFill = 7,
	.ubColorText = 13
};

tGuiConfig *guiGetConfig(void) {
	return &s_sConfig;
}
