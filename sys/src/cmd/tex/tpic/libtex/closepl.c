#include "tex.h"
closepl()
{
	fprintf(TEXFILE, "    \\kern %6.3fin\n  }\\vss}%%\n", INCHES(e1->sidex));
	fprintf(TEXFILE, "  \\kern %6.3fin\n}\n", INCHES(e1->sidey));
}
