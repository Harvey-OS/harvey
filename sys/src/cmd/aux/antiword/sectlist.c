/*
 * sectlist.c
 * Copyright (C) 2001,2002 A.J. van Os; Released under GPL
 *
 * Description:
 * Build, read and destroy list(s) of Word section information
 */

#include <stddef.h>
#include <string.h>
#include "antiword.h"


/*
 * Private structure to hide the way the information
 * is stored from the rest of the program
 */
typedef struct section_mem_tag {
	section_block_type	tInfo;
	ULONG			ulTextOffset;
	struct section_mem_tag	*pNext;
} section_mem_type;

/* Variables needed to write the Section Information List */
static section_mem_type	*pAnchor = NULL;
static section_mem_type	*pSectionLast = NULL;


/*
 * vDestroySectionInfoList - destroy the Section Information List
 */
void
vDestroySectionInfoList(void)
{
	section_mem_type	*pCurr, *pNext;

	DBG_MSG("vDestroySectionInfoList");

	/* Free the Section Information List */
	pCurr = pAnchor;
	while (pCurr != NULL) {
		pNext = pCurr->pNext;
		pCurr = xfree(pCurr);
		pCurr = pNext;
	}
	pAnchor = NULL;
	/* Reset all control variables */
	pSectionLast = NULL;
} /* end of vDestroySectionInfoList */

/*
 * vAdd2SectionInfoList - Add an element to the Section Information List
 */
void
vAdd2SectionInfoList(const section_block_type *pSection, ULONG ulTextOffset)
{
	section_mem_type	*pListMember;

	fail(pSection == NULL);

	/* Create list member */
	pListMember = xmalloc(sizeof(section_mem_type));
	/* Fill the list member */
	pListMember->tInfo = *pSection;
	pListMember->ulTextOffset = ulTextOffset;
	pListMember->pNext = NULL;
	/* Add the new member to the list */
	if (pAnchor == NULL) {
		pAnchor = pListMember;
	} else {
		fail(pSectionLast == NULL);
		pSectionLast->pNext = pListMember;
	}
	pSectionLast = pListMember;
} /* vAdd2SectionInfoList */

/*
 * vGetDefaultSection - fill the section struct with default values
 */
void
vGetDefaultSection(section_block_type *pSection)
{
	(void)memset(pSection, 0, sizeof(*pSection));
	pSection->bNewPage = TRUE;
} /* end of vGetDefaultSection */

/*
 * vDefault2SectionInfoList - Add a default to the Section Information List
 */
void
vDefault2SectionInfoList(ULONG ulTextOffset)
{
	section_block_type	tSection;

	vGetDefaultSection(&tSection);
	vAdd2SectionInfoList(&tSection, ulTextOffset);
} /* end of vDefault2SectionInfoList */

/*
 * pGetSectionInfo - get the section information
 */
const section_block_type *
pGetSectionInfo(const section_block_type *pOld, ULONG ulTextOffset)
{
	const section_mem_type	*pCurr;

	if (pOld == NULL || ulTextOffset == 0) {
		if (pAnchor == NULL) {
			/* There are no records, make one */
			vDefault2SectionInfoList(0);
			fail(pAnchor == NULL);
		}
		/* The first record */
		NO_DBG_MSG("First record");
		return &pAnchor->tInfo;
	}
	for (pCurr = pAnchor; pCurr != NULL; pCurr = pCurr->pNext) {
		if (ulTextOffset == pCurr->ulTextOffset ||
		    ulTextOffset + 1 == pCurr->ulTextOffset) {
			NO_DBG_HEX(pCurr->ulTextOffset);
			return &pCurr->tInfo;
		}
	}
	return pOld;
} /* end of pGetSectionInfo */
