
typedef struct Jayconfig Jayconfig;
typedef struct Widget Widget;
typedef struct WListElement WListElement;
typedef struct Panel Panel;
typedef struct Label Label;
typedef enum wtype wtype;

enum wtype{
  PANEL,
  LABEL
};

struct Jayconfig{
	// Task Panel Config
	uint32_t taskPanelColor;

	// Main Menu Config
	uint32_t mainMenuColor;
	uint32_t mainMenuHooverColor;

	// Window Config
	uint32_t windowTitleColor;
	uint32_t windowTitleFontColor;
	uint32_t windowBackgroundColor;
	uint32_t windowInTopBorder;
	uint32_t windowInBottomBorder;
	uint32_t windowSelectedColor;
	uint32_t windowScrollBarFrontColor;
	uint32_t windowTextCursorColor;
	uint32_t windowBackTextColor;
	uint32_t windowFrontTextColor;

	//Background
	uint32_t backgroundColor;
	char *backgroundimgpath;

	//Menu
	uint32_t menuBackColor;
	uint32_t menuHighColor;
	uint32_t menuBorderColor;
	uint32_t menuTextColor;
	uint32_t menuSelTextColor;

  //Widgets
  uint32_t mainBackColor;
  uint32_t mainTextColor;

  char *fontPath;
};

struct Widget {
  char *id;
  Rectangle r;
  Point p; //Real position
  Point pos; //Relative position
  wtype t; //widget type
  void *w; //The widget
  Widget *father; //Container
  Image *i;
  int visible;
  void (*hover)(Widget *w);
  void (*unhover)(Widget *w);
  void (*draw)(Widget *w);
  void (*resize)(Widget *w);

  //For internal use
  void (*_hover)(Widget *w, Mousectl *m);
  void (*_unhover)(Widget *w);
  void (*_draw)(Widget *w, Image *dst);
  void (*_redraw)(Widget *w);
  void (*_resize)(Widget *w, Point d); //d is the vector that represents the displacement

  int (*addWidget)(Widget *me, Widget *new, Point pos);
  int width; //ancho
  int height;//alto

  int hovered;
  int autosize;

  Widget *lh; //last hover
};

struct WListElement {
  WListElement *next;
  WListElement *prev;
  Widget *w;
};

struct Panel {
  Widget *w;
  uint32_t backColor;
  WListElement *l;
};

struct Label {
  Widget *w;
  char *t;
  Font *f;

  int border;
  int d3; // 3D Border true/false
  int up; // If d3=true then up defines the efect
  uint32_t backColor;
  uint32_t textColor;

  void (*setText)(Label *l, const char *text);
  char * (*gettext)(Label *l);
};

Widget *initjayapp(char *name);
void startjayapp(Widget * w);
void initdefaultconfig();
Widget *createPanel(char *id, int height, int width, Point p);
Widget *createLabel(char *id, int height, int width);
