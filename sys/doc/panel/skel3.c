void eventloop(void){
	Event e;
	for(;;){
		switch(event(&e)){
		case Ekeyboard:
			plkeyboard(e.kbdc);
			break;
		case Emouse:
			plmouse(root, e.mouse);
			break;
		}
	}
}
