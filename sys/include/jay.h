
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
};

struct Widget {
  char *id;
  Rectangle r; //Size
  Point p; //Position
  wtype t; //widget type
  void *w; //The widget
  Widget *father; //Container
  Image *i;
  int visible;
  void (*hover)(Widget *w);
  void (*unhover)(Widget *w);
  void (*draw)(Widget *w);

  //For internal use
  void (*_hover)(Widget *w, Mousectl *m);
  void (*_unhover)(Widget *w);
  void (*_draw)(Widget *w, Image *dst);

  int hovered;
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
};

Widget *createWidget(char *id, Rectangle r, Point p, wtype t, void *w);
Widget *createPanel(char *id, Rectangle r, Point p);
