
#include <stdio.h>
#include "map.h"

		/* p0;p1 evaluated to shut off "not used" diags */
static proj Yaitoff(float p0,float p1){p0;p1;return aitoff();}
static proj Yalbers(float p0,float p1){return albers(p0,p1);}
static proj Yazequalarea(float p0,float p1){p0;p1;return azequalarea();}
static proj Yazequidistant(float p0,float p1){p0;p1;return azequidistant();}
static proj Ybicentric(float p0,float p1){p1;return bicentric(p0);}
static proj Ybonne(float p0,float p1){p1;return bonne(p0);}
static proj Yconic(float p0,float p1){p1;return conic(p0);}
static proj Ycylequalarea(float p0,float p1){p1;return cylequalarea(p0);}
static proj Ycylindrical(float p0,float p1){p0;p1;return cylindrical();}
static proj Yelliptic(float p0,float p1){p1;return elliptic(p0);}
static proj Yfisheye(float p0,float p1){p1;return fisheye(p0);}
static proj Ygall(float p0,float p1){p1;return gall(p0);}
static proj Yglobular(float p0,float p1){p0;p1;return globular();}
static proj Ygnomonic(float p0,float p1){p0;p1;return gnomonic();}
static proj Yguyou(float p0,float p1){p0;p1;return guyou();}
static proj Yharrison(float p0,float p1){return harrison(p0,p1);}
static proj Yhex(float p0,float p1){p0;p1;return hex();}
static proj Yhoming(float p0,float p1){p1;return homing(p0);}
static proj Ylagrange(float p0,float p1){p0;p1;return lagrange();}
static proj Ylambert(float p0,float p1){return lambert(p0,p1);}
static proj Ylaue(float p0,float p1){p0;p1;return laue();}
static proj Yloxodromic(float p0,float p1){p1;return loxodromic(p0);}
static proj Ymecca(float p0,float p1){p1;return mecca(p0);}
static proj Ymercator(float p0,float p1){p0;p1;return mercator();}
static proj Ymollweide(float p0,float p1){p0;p1;return mollweide();}
static proj Yortelius(float p0,float p1){return ortelius(p0,p1);}
static proj Yorthographic(float p0,float p1){p0;p1;return orthographic();}
static proj Yperspective(float p0,float p1){p1;return perspective(p0);}
static proj Ypolyconic(float p0,float p1){p0;p1;return polyconic();}
static proj Yrectangular(float p0,float p1){p1;return rectangular(p0);}
static proj Ysimpleconic(float p0,float p1){return simpleconic(p0,p1);}
static proj Ysinusoidal(float p0,float p1){p0;p1;return sinusoidal();}
static proj Ysp_albers(float p0,float p1){return sp_albers(p0,p1);}
static proj Ysp_mercator(float p0,float p1){p0;p1;return sp_mercator();}
static proj Ysquare(float p0,float p1){p0;p1;return square();}
static proj Ystereographic(float p0,float p1){p0;p1;return stereographic();}
static proj Ytetra(float p0,float p1){p0;p1;return tetra();}
static proj Ytrapezoidal(float p0,float p1){return trapezoidal(p0,p1);}
static proj Yvandergrinten(float p0,float p1){p0;p1;return vandergrinten();}
static proj Ywreath(float p0,float p1){return wreath(p0,p1);}

struct index index[] = {
	{"aitoff", Yaitoff, 0, picut, 0, 0},
	{"albers", Yalbers, 2, picut, 3, 0},
	{"azequalarea", Yazequalarea, 0, nocut, 1, 0},
	{"azequidistant", Yazequidistant, 0, nocut, 1, 0},
	{"bicentric", Ybicentric, 1, nocut, 0, 0},
	{"bonne", Ybonne, 1, picut, 0, 0},
	{"conic", Yconic, 1, picut, 0, 0},
	{"cylequalarea", Ycylequalarea, 1, picut, 3, 0},
	{"cylindrical", Ycylindrical, 0, picut, 0, 0},
	{"elliptic", Yelliptic, 1, nocut, 0, 0},
	{"fisheye", Yfisheye, 1, nocut, 0, 0},
	{"gall", Ygall, 1, picut, 3, 0},
	{"globular", Yglobular, 0, picut, 0, 0},
	{"gnomonic", Ygnomonic, 0, nocut, 0, 0},
	{"guyou", Yguyou, 0, guycut, 0, 0},
	{"harrison", Yharrison, 2, nocut, 0, 0},
	{"hex", Yhex, 0, hexcut, 0, 0},
	{"homing", Yhoming, 1, picut, 0, 0},
	{"lagrange", Ylagrange,0,picut,0, 0},
	{"lambert", Ylambert, 2, picut, 0, 0},
	{"laue", Ylaue, 0, nocut, 0, 0},
	{"loxodromic", Yloxodromic, 1, loxcut, 0, 0},
	{"mecca", Ymecca, 1, picut, 0, 0},
	{"mercator", Ymercator, 0, picut, 0, 0},
	{"mollweide", Ymollweide, 0, picut, 0, 0},
 	{"ortelius", Yortelius, 2, picut, 3, 0},
	{"orthographic", Yorthographic, 0, nocut, 0, 0},
	{"perspective", Yperspective, 1, nocut, 0, 0},
	{"polyconic", Ypolyconic, 0, picut, 0, 0},
	{"rectangular", Yrectangular, 1, picut, 3, 0},
	{"simpleconic", Ysimpleconic, 2, picut, 3, 0},
	{"sinusoidal", Ysinusoidal, 0, picut, 0, 0},
	{"sp_albers", Ysp_albers, 2, picut, 3, 1},
	{"sp_mercator", Ysp_mercator, 0, picut, 0, 1},
	{"square", Ysquare, 0, picut, 0, 0},
	{"stereographic", Ystereographic, 0, nocut, 0, 0},
	{"tetra", Ytetra, 0, tetracut, 0, 0},
	{"trapezoidal", Ytrapezoidal, 2, picut, 3, 0},
	{"vandergrinten", Yvandergrinten, 0, picut, 0, 0},
	{"wreath", Ywreath, 2, picut, 3, 0 },
	0
};
