void main(void){
	binit(0, 0, 0);
	einit(Emouse);
	plinit(screen.ldepth);

	root=plframe(0, 0);
	pllabel(root, 0, "Hello, world!");
	plbutton(root, 0, "done", done);

	ereshaped(screen.r);
	for(;;) plmouse(root, emouse());
}
