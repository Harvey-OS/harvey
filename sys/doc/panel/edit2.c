Panel *mkpanels(void){
	Panel *scrl, *pop, *menu, *root;
	root=plgroup(0, EXPAND);
	scrl=plscrollbar(root, PACKW);
	menu=plmenu(0, 0, buttons, FILLX, hitmenu);
	pop=plpopup(root, EXPAND, 0, menu, menu);
	edit=pledit(pop, EXPAND, Pt(0,0),
		text, sizeof(text)/sizeof(Rune)-1, 0);
	plscroll(edit, 0, scrl);
	plgrabkb(edit);
	return root;
}
