/*
 * pictlist.c
 * Copyright (C) 2000,2001 A.J. van Os; Released under GPL
 *
 * Description:
 * Build, read and destroy a list of Word picture information
 */

#include <stdlib.h>
#include "antiword.h"


/* Variables needed to write the Picture Information List */
static picture_desc_type	*pAnchor = NULL;
static picture_desc_type	*pPictureLast = NULL;


/*
 * vDestroyPictInfoList - destroy the Picture Information List
 */
void
vDestroyPictInfoList(void)
{
	picture_desc_type	*pCurr, *pNext;

	DBG_MSG("vDestroyPictInfoList");

	/* Free the Picture Information List */
	pCurr = pAnchor;
	while (pCurr != NULL) {
		pNext = pCurr->pNext;
		pCurr = xfree(pCurr);
		pCurr = pNext;
	}
	pAnchor = NULL;
	/* Reset all control variables */
	pPictureLast = NULL;
} /* end of vDestroyPictInfoList */

/*
 * vAdd2PictInfoList - Add an element to the Picture Information List
 */
void
vAdd2PictInfoList(const picture_block_type *pPictureBlock)
{
	picture_desc_type	*pListMember;

	fail(pPictureBlock == NULL);

	NO_DBG_MSG("bAdd2PictInfoList");

	if (pPictureBlock->ulFileOffset == FC_INVALID) {
		/*
		 * This offset is really past the end of the file,
		 * so don't waste any memory by storing it.
		 */
		return;
	}
	if (pPictureBlock->ulFileOffsetPicture == FC_INVALID) {
		/*
		 * The place where this picture is supposed to be stored
		 * doesn't exist.
		 */
		return;
	}

	NO_DBG_HEX(pPictureBlock->ulFileOffset);
	NO_DBG_HEX(pPictureBlock->ulFileOffsetPicture);
	NO_DBG_HEX(pPictureBlock->ulPictureOffset);

	/* Create list member */
	pListMember = xmalloc(sizeof(picture_desc_type));
	/* Fill the list member */
	pListMember->tInfo = *pPictureBlock;
	pListMember->pNext = NULL;
	/* Add the new member to the list */
	if (pAnchor == NULL) {
		pAnchor = pListMember;
	} else {
		fail(pPictureLast == NULL);
		pPictureLast->pNext = pListMember;
	}
	pPictureLast = pListMember;
} /* end of vAdd2PictInfoList */

/*
 * Get the info with the given file offset from the Picture Information List
 */
ULONG
ulGetPictInfoListItem(ULONG ulFileOffset)
{
	picture_desc_type	*pCurr;

	for (pCurr = pAnchor; pCurr != NULL; pCurr = pCurr->pNext) {
		if (pCurr->tInfo.ulFileOffset == ulFileOffset) {
			return pCurr->tInfo.ulFileOffsetPicture;
		}
	}
	return FC_INVALID;
} /* end of ulGetPictInfoListItem */
