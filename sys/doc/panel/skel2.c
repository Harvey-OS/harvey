void main(void){
	binit(0, 0, 0);
	einit(Emouse|Ekeyboard);
	plinit(screen.ldepth);

	root=mkpanels();

	ereshaped(screen.r);

	eventloop();
}
