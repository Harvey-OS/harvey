
typedef struct Jayconfig Jayconfig;
typedef struct Widget Widget;
typedef struct WListElement WListElement;
typedef struct Panel Panel;
typedef struct Label Label;
typedef struct Border Border;
typedef enum wtype wtype;

enum wtype{
  PANEL,
  LABEL
};

struct Jayconfig{
	// Task Panel Config
	u32 taskPanelColor;

	// Main Menu Config
	u32 mainMenuColor;
	u32 mainMenuHooverColor;

	// Window Config
	u32 windowTitleColor;
	u32 windowTitleFontColor;
	u32 windowBackgroundColor;
	u32 windowInTopBorder;
	u32 windowInBottomBorder;
	u32 windowSelectedColor;
	u32 windowScrollBarFrontColor;
	u32 windowTextCursorColor;
	u32 windowBackTextColor;
	u32 windowFrontTextColor;

	//Background
	u32 backgroundColor;
	char *backgroundimgpath;

	//Menu
	u32 menuBackColor;
	u32 menuHighColor;
	u32 menuBorderColor;
	u32 menuTextColor;
	u32 menuSelTextColor;

  //Widgets
  u32 mainBackColor;
  u32 mainTextColor;
  char *fontPath;

  unsigned int doubleclickTime;
};

struct Widget {
  char *id;

  //Methods
  int (*addWidget)(Widget *me, Widget *new, Point pos);
  Widget *(*getWidget)(Widget *me, char *id);
  Widget *(*extractWidget)(Widget *me, char *id);
  int  (*listWidgets)(Widget *me, char ***list);
  void (*deleteWidget)(Widget *me, char *id);
  void (*setVisible)(Widget *w, int visible);
  void (*freeWidget)(Widget *w);

  //User Events:
  void (*hover)(Widget *w);
  void (*unhover)(Widget *w);
  void (*draw)(Widget *w);
  void (*resize)(Widget *w);
  void (*click)(Widget *w, Mouse *);
  void (*dclick)(Widget *w, Mouse *);
  void (*mpressdown)(Widget *w, Mouse *);
  void (*mpressup)(Widget *w, Mouse *);
  void (*change)(Widget *w);

  //For internal use
  void (*_hover)(Widget *w, Mouse *m);
  void (*_unhover)(Widget *w);
  void (*_draw)(Widget *w, Image *dst);
  void (*_redraw)(Widget *w);
  void (*_resize)(Widget *w, Point d); //d is the vector that represents the displacement
  void (*_click)(Widget *w, Mouse *);
  void (*_dclick)(Widget *w, Mouse *);
  void (*_mpressdown)(Widget *w, Mouse *);
  void (*_mpressup)(Widget *w, Mouse *);
  void (*_change)(Widget *w);

  int width; //ancho
  int height;//alto
  int hovered;
  int autosize;
  Rectangle r;
  Point p; //Real position
  Point pos; //Relative position
  wtype t; //widget type
  void *w; //The widget
  Widget *father; //Container
  Image *i;
  int visible;
  Widget *lh; //last hover
};

struct WListElement {
  WListElement *next;
  WListElement *prev;
  Widget *w;
};

struct Border{
  int size;
  int _3d; // True if border is 3D
  int up; // Up or Down 3D effect
};

struct Panel {
  Widget *w;
  u32 backColor;
  WListElement *l;
};

struct Label {
  Widget *w;
  char *t;
  Font *f;

  Border border;
  u32 backColor;
  u32 textColor;

  void (*setText)(Label *l, const char *text);
  char * (*gettext)(Label *l);
};

Widget *initjayapp(char *name);
void startjayapp(Widget * w);
void initdefaultconfig();
Border createBorder(int size, int _3D, int up);
Widget *createPanel(char *id, int height, int width, Point p);
Widget *createLabel(char *id, int height, int width);
