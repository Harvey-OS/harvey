void ereshaped(Rectangle r){
	screen.r=r;
	plpack(root, r);
	bitblt(&screen, r.min, &screen, r, Zero);
	pldraw(root, &screen);
}
