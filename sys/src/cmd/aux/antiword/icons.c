/*
 * icons.c
 * Copyright (C) 1998-2001 A.J. van Os; Released under GPL
 *
 * Description:
 * Update window icons
 */

#include <string.h>
#include "wimpt.h"
#include "antiword.h"

void
vUpdateIcon(wimp_w tWindow, wimp_icon *pIcon)
{
	wimp_redrawstr	r;
	BOOL		bMore;

	r.w = tWindow;
	r.box = pIcon->box;
	wimpt_noerr(wimp_update_wind(&r, &bMore));
	while (bMore) {
		(void)wimp_ploticon(pIcon);
		wimpt_noerr(wimp_get_rectangle(&r, &bMore));
	}
} /* end of vUpdateIcon */

void
vUpdateRadioButton(wimp_w tWindow, wimp_i tIconNumber, BOOL bSelected)
{
	wimp_icon	tIcon;

	wimpt_noerr(wimp_get_icon_info(tWindow, tIconNumber, &tIcon));
	DBG_DEC(tIconNumber);
	DBG_HEX(tIcon.flags);
	if (bSelected ==
	    ((tIcon.flags & wimp_ISELECTED) == wimp_ISELECTED)) {
		/* No update needed */
		return;
	}
	wimpt_noerr(wimp_set_icon_state(tWindow, tIconNumber,
			bSelected ? wimp_ISELECTED : 0, wimp_ISELECTED));
	vUpdateIcon(tWindow, &tIcon);
} /* end of vUpdateRadioButton */

/*
 * vUpdateWriteable - update a writeable icon with a string
 */
void
vUpdateWriteable(wimp_w tWindow, wimp_i tIconNumber, char *szString)
{
	wimp_icon	tIcon;
	wimp_caretstr	tCaret;
	int		iLen;

	fail(szString == NULL);

	NO_DBG_DEC(tIconNumber);
	NO_DBG_MSG(szString);

	wimpt_noerr(wimp_get_icon_info(tWindow, tIconNumber, &tIcon));
	NO_DBG_HEX(tIcon.flags);
	if ((tIcon.flags & (wimp_ITEXT|wimp_INDIRECT)) !=
	    (wimp_ITEXT|wimp_INDIRECT)) {
		werr(1, "Icon %d must be indirected text", (int)tIconNumber);
		return;
	}
	strncpy(tIcon.data.indirecttext.buffer,
		szString,
		tIcon.data.indirecttext.bufflen - 1);
	/* Ensure the caret is behind the last character of the text */
	wimpt_noerr(wimp_get_caret_pos(&tCaret));
	if (tCaret.w == tWindow && tCaret.i == tIconNumber) {
		iLen = strlen(tIcon.data.indirecttext.buffer);
		if (tCaret.index != iLen) {
			tCaret.index = iLen;
			wimpt_noerr(wimp_set_caret_pos(&tCaret));
		}
	}
	wimpt_noerr(wimp_set_icon_state(tWindow, tIconNumber, 0,0));
	vUpdateIcon(tWindow, &tIcon);
} /* end of vUpdateWriteable */

/*
 * vUpdateWriteableNumber - update a writeable icon with a number
 */
void
vUpdateWriteableNumber(wimp_w tWindow, wimp_i tIconNumber, int iNumber)
{
	char	szTmp[12];

	(void)sprintf(szTmp, "%d", iNumber);
	vUpdateWriteable(tWindow, tIconNumber, szTmp);
} /* end of vUpdateWriteableNumber */
