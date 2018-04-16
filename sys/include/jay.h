
typedef struct Widget Widget;
typedef enum wtype wtype;

enum wtype{
  PANEL,
  LABEL
};

struct Widget {
  Rectangle r; //Size
  wtype t;
};
