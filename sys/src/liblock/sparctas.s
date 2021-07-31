/*
 *	tas uses LDSTUB
 */
	TEXT	tas(SB),$-4

	TAS	(R7),R7
	RETURN
