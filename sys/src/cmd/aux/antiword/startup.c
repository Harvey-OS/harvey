/*
 * startup.c
 * Copyright (C) 1998-2001 A.J. van Os; Released under GPL
 *
 * Description:
 * Try to force a single startup of !Antiword
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "kernel.h"
#include "swis.h"
#include "wimp.h"
#include "wimpt.h"
#include "antiword.h"


#if !defined(TaskManager_EnumerateTasks)
#define TaskManager_EnumerateTasks	0x042681
#endif /* TaskManager_EnumerateTasks */

/*
 * bIsMatch - decide whether the two strings match
 *
 * like strcmp, but this one ignores case
 */
static BOOL
bIsMatch(const char *szStr1, const char *szStr2)
{
	const char	*pcTmp1, *pcTmp2;

	for (pcTmp1 = szStr1, pcTmp2 = szStr2;
	     *pcTmp1 != '\0';
	     pcTmp1++, pcTmp2++) {
		if (toupper(*pcTmp1) != toupper(*pcTmp2)) {
			return FALSE;
		}
	}
	return *pcTmp2 == '\0';
} /* end of bIsMatch */

/*
 * tGetTaskHandle - get the task handle of the given task
 *
 * returns the task handle when found, otherwise 0
 */
static wimp_t
tGetTaskHandle(const char *szTaskname, int iOSVersion)
{
	_kernel_swi_regs	regs;
	_kernel_oserror		*e;
	const char	*pcTmp;
	int	iIndex;
	int	aiBuffer[4];
	char	szTmp[21];

	if (iOSVersion < 310) {
		/*
		 * SWI TaskManager_EnumerateTasks does not
		 * exist in earlier versions of RISC OS
		 */
		return 0;
	}

	(void)memset((void *)&regs, 0, sizeof(regs));
	regs.r[0] = 0;

	do {
		/* Get info on the next task */
		regs.r[1] = (int)aiBuffer;
		regs.r[2] = sizeof(aiBuffer);
		e = _kernel_swi(TaskManager_EnumerateTasks, &regs, &regs);
		if (e != NULL) {
			werr(1, "TaskManager_EnumerateTasks error %d: %s",
				e->errnum, e->errmess);
			exit(EXIT_FAILURE);
		}
		/* Copy the (control character terminated) task name */
		for (iIndex = 0, pcTmp = (const char *)aiBuffer[1];
		     iIndex < elementsof(szTmp);
		     iIndex++, pcTmp++) {
			if (iscntrl(*pcTmp)) {
				szTmp[iIndex] = '\0';
				break;
			}
			szTmp[iIndex] = *pcTmp;
		}
		szTmp[elementsof(szTmp) - 1] = '\0';
		if (bIsMatch(szTmp, szTaskname)) {
			/* Task found */
			return (wimp_t)aiBuffer[0];
		}
	} while (regs.r[0] >= 0);

	/* Task not found */
	return 0;
} /* end of tGetTaskHandle */

int
main(int argc, char **argv)
{
	wimp_msgstr	tMsg;
	wimp_t	tTaskHandle;
	size_t	tArgLen;
	int	iVersion;
	char	szCommand[512];

	iVersion = wimpt_init("StartUp");

	if (argc > 1) {
		tArgLen = strlen(argv[1]);
	} else {
		tArgLen = 0;
	}
	if (tArgLen >= sizeof(tMsg.data.dataload.name)) {
		werr(1, "Input filename too long");
		return EXIT_FAILURE;
	}

	tTaskHandle = tGetTaskHandle("antiword", iVersion);

	if (tTaskHandle == 0) {
		/* Antiword is not active */
		strcpy(szCommand, "chain:<Antiword$Dir>.!Antiword");
		if (argc > 1) {
			strcat(szCommand, " ");
			strcat(szCommand, argv[1]);
		}
#if defined(DEBUG)
		strcat(szCommand, " ");
		strcat(szCommand, "2><Antiword$Dir>.Debug");
#endif /* DEBUG */
		system(szCommand);
		/* If we reach here something has gone wrong */
		return EXIT_FAILURE;
	}

	/* Antiword is active */
	if (argc > 1) {
		/* Send the argument to Antiword */
		tMsg.hdr.size = ROUND4(sizeof(tMsg) -
					sizeof(tMsg.data.dataload.name) +
					1 + tArgLen);
		tMsg.hdr.your_ref = 0;
		tMsg.hdr.action = wimp_MDATALOAD;
		tMsg.data.dataload.w = -1;
		tMsg.data.dataload.size = 0;
		tMsg.data.dataload.type = FILETYPE_MSWORD;
		strcpy(tMsg.data.dataload.name, argv[1]);
		wimpt_noerr(wimp_sendmessage(wimp_ESEND,
						&tMsg, tTaskHandle));
		return EXIT_SUCCESS;
	} else {
		/* Give an error message and return */
		werr(1, "Antiword is already running");
		return EXIT_FAILURE;
	}
} /* end of main */
