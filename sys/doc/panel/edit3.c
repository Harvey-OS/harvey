void hitmenu(int button, int index){
	USED(button);
	switch(index){
	case 0:	/* cut */
		snarf();
		plepaste(edit, 0, 0);
		break;
	case 1:	/* paste */
		plepaste(edit, snarfbuf, nsnarfbuf);
		break;
	case 2:	/* snarf */
		snarf();
		break;
	case 3:	/* exit */
		exits(0);
	}
}
