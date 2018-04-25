Jayconfig *jayconfig;

Widget *createWidget(char *id, int height, int width, wtype t, void *w);
Widget *createWidget1(char *id, Rectangle r, wtype t, void *w);
Widget *createPanel1(char *id, Rectangle r, Point p);
WListElement *createWListElement(Widget *w);
WListElement *getWListElementByID(WListElement *list, char *id);
int addWListElement(WListElement *list, Widget *w);

void _simpleResize(Widget *w, Point d);
