#include <u.h>
#include <libc.h>
#include <draw.h>
#include <thread.h>
#include <mouse.h>
#include <jay.h>
#include "fns.h"

void
initdefaultconfig(){
	jayconfig = malloc(sizeof(Jayconfig));
	jayconfig->taskPanelColor = 0x8C7160FF;

	jayconfig->mainMenuColor = 0x363533FF;
	jayconfig->mainMenuHooverColor = 0x36A4BFFF;

	jayconfig->windowTitleColor = 0x4D4D4DFF;
	jayconfig->windowTitleFontColor = DWhite;
	jayconfig->windowInTopBorder = 0xC4CAC8FF;
	jayconfig->windowInBottomBorder = DPalegreygreen;
	jayconfig->windowSelectedColor = 0xCCCCCCFF;
	jayconfig->windowScrollBarFrontColor = DWhite;
	jayconfig->windowTextCursorColor = DWhite;
	jayconfig->windowBackgroundColor = DBlack;
	jayconfig->windowBackTextColor = 0x666666FF;
	jayconfig->windowFrontTextColor = DWhite;

	jayconfig->backgroundColor = DBlack;
	jayconfig->backgroundimgpath = "/usr/harvey/lib/background.img";

	jayconfig->menuBackColor = DPalegreyblue;
	jayconfig->menuHighColor = DGreyblue;
	jayconfig->menuBorderColor = jayconfig->menuHighColor;
	jayconfig->menuSelTextColor = DWhite;
	jayconfig->menuTextColor = DWhite;

	jayconfig->mainBackColor = DWhite;
	jayconfig->mainTextColor = DBlack;

	jayconfig->fontPath = "/lib/font/bit/go/Go-Mono/Go-Mono.14.font";
	jayconfig->doubleclickTime = 500;
}
