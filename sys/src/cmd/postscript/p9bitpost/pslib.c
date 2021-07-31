#include <u.h>
#include <libc.h>
#include <draw.h>
#include <memdraw.h>
#include <bio.h>
#include "pslib.h"
/* implement PsLib;
/* 
/* include "sys.m";
/* 	sys: Sys;
/* 
/* include "draw.m";
/* 	draw : Draw;
/* Image, Display,Rect,Point : import draw;
/* 
/* include "bufio.m";
/* 	bufmod : Bufio;
/* 
/* include "tk.m";
/* 	tk: Tk;
/* 	Toplevel: import tk;
/* 
/* Iobuf : import bufmod;
/* 
/* include "string.m";
/* 	str : String;
/* 
/* include "daytime.m";
/* 	time : Daytime;
/* 
/* include "pslib.m";
/* 
/* ASCII,RUNE,IMAGE : con iota;
/* 
*/
struct iteminfo {
	int itype;
	int offset;		/* offset from the start of line. */
	int width;		/* width.... */
	int ascent;		/* ascent of the item */
	int font;		/* font */
	int line;		/* line its on */
	char *buf;	
};

struct lineinfo {
	int xorg;
	int yorg;
	int width;
	int height;
	int ascent;
};


/* font_arr := array[256] of {* => (-1,"")};
/* remap := array[20] of (string,string);
/* 
/* PXPI : con 100;
/* PTPI : con 100;
/* 
*/
char *noinit = "pslib not properly initialized";
/* 
*/
static int boxes;
static int debug;
static int totitems;
static int totlines;
static int curfont;
static char *def_font;
static int def_font_type;
static int curfonttype;
static int pagestart;
static int started;

static int bps;
static int width;
static int height;
static int iwidth;
static int iheight;
static int xstart;
static int ystart;
static double xmagnification = 1.0, ymagnification = 1.0;
static int rotation = 0;
static int landscape = 0;
static char *Patch = nil;

/* ctxt 		: ref Draw->Context;
/* t 		: ref Toplevel;
*/
char*
psinit(int box, int deb) { /* d: ref Toplevel, */
/* 	t=d; */
	debug = deb;
	totlines=0;
	totitems=0;
	pagestart=0;
	boxes=box; /* #box; */
	curfont=0;
/* 	e := loadfonts();
/* 	if (e != "")
/* 		return e;
*/
	started=1;
	return "";
}

/* stats() : (int,int,int)
/* {
/* 	return 	(totitems,totlines,curfont);
/* }
/* 
/* loadfonts() : string
/* {
/* 	input : string;
/* 	iob:=bufmod->open("/fonts/psrename",bufmod->OREAD);
/* 	if (iob==nil)
/* 		return sys->sprint("can't open /fonts/psrename: %r");
/* 	i:=0;
/* 	while((input=iob.gets('\n'))!=nil){
/* 		(tkfont,psfont):=str->splitl(input," ");
/* 		psfont=psfont[1:len psfont -1];
/* 		remap[i]=(tkfont,psfont);
/* 		i++;
/* 	}
/* 	return "";
/* }
/* 
*/
static char *username;

int
preamble(Biobuf *ioutb, Rectangle bb) {

	if (!started) return 1;
	username = getuser();
	if(bb.max.x == 0 && bb.max.y == 0) {
		bb.max.x = 612;
		bb.max.y = 792;
	}
	Bprint(ioutb, "%%!PS-Adobe-3.0\n");
	Bprint(ioutb, "%%%%Creator: PsLib 1.0 (%s)\n",username);
	Bprint(ioutb, "%%%%CreationDate: %s", ctime(time(nil)));
	Bprint(ioutb, "%%%%Pages: (atend) \n");
	Bprint(ioutb, "%%%%BoundingBox: %d %d %d %d\n", bb.min.x, bb.min.y, bb.max.x, bb.max.y);
	Bprint(ioutb, "%%%%EndComments\n");
	Bprint(ioutb, "%%%%BeginProlog\n");
	Bprint(ioutb, "/doimage {\n");
	Bprint(ioutb, "/grey exch def\n");
	Bprint(ioutb, "/bps exch def\n");
	Bprint(ioutb, "/width exch def\n");
	Bprint(ioutb, "/height exch def\n");
	Bprint(ioutb, "/xstart exch def\n");
	Bprint(ioutb, "/ystart exch def\n");
	Bprint(ioutb, "/iwidth exch def\n");
	Bprint(ioutb, "/ascent exch def\n");
	Bprint(ioutb, "/iheight exch def\n");
	Bprint(ioutb, "gsave\n");
	if(boxes)
		Bprint(ioutb, "xstart ystart iwidth iheight rectstroke\n");
/*	# if bps==8, use inferno colormap; else (bps < 8) it's grayscale or true color */
	Bprint(ioutb, "bps 8 eq grey false eq and {\n");
	Bprint(ioutb, " [/Indexed /DeviceRGB 255 <\n");
	Bprint(ioutb, "  ffffff ffffaa ffff55 ffff00 ffaaff ffaaaa ffaa55 ffaa00 ff55ff ff55aa ff5555 ff5500\n");
	Bprint(ioutb, "  ff00ff ff00aa ff0055 ff0000 ee0000 eeeeee eeee9e eeee4f eeee00 ee9eee ee9e9e ee9e4f\n");
	Bprint(ioutb, "  ee9e00 ee4fee ee4f9e ee4f4f ee4f00 ee00ee ee009e ee004f dd0049 dd0000 dddddd dddd93\n");
	Bprint(ioutb, "  dddd49 dddd00 dd93dd dd9393 dd9349 dd9300 dd49dd dd4993 dd4949 dd4900 dd00dd dd0093\n");
	Bprint(ioutb, "  cc0088 cc0044 cc0000 cccccc cccc88 cccc44 cccc00 cc88cc cc8888 cc8844 cc8800 cc44cc\n");
	Bprint(ioutb, "  cc4488 cc4444 cc4400 cc00cc aaffaa aaff55 aaff00 aaaaff bbbbbb bbbb5d bbbb00 aa55ff\n");
	Bprint(ioutb, "  bb5dbb bb5d5d bb5d00 aa00ff bb00bb bb005d bb0000 aaffff 9eeeee 9eee9e 9eee4f 9eee00\n");
	Bprint(ioutb, "  9e9eee aaaaaa aaaa55 aaaa00 9e4fee aa55aa aa5555 aa5500 9e00ee aa00aa aa0055 aa0000\n");
	Bprint(ioutb, "  990000 93dddd 93dd93 93dd49 93dd00 9393dd 999999 99994c 999900 9349dd 994c99 994c4c\n");
	Bprint(ioutb, "  994c00 9300dd 990099 99004c 880044 880000 88cccc 88cc88 88cc44 88cc00 8888cc 888888\n");
	Bprint(ioutb, "  888844 888800 8844cc 884488 884444 884400 8800cc 880088 55ff55 55ff00 55aaff 5dbbbb\n");
	Bprint(ioutb, "  5dbb5d 5dbb00 5555ff 5d5dbb 777777 777700 5500ff 5d00bb 770077 770000 55ffff 55ffaa\n");
	Bprint(ioutb, "  4fee9e 4fee4f 4fee00 4f9eee 55aaaa 55aa55 55aa00 4f4fee 5555aa 666666 666600 4f00ee\n");
	Bprint(ioutb, "  5500aa 660066 660000 4feeee 49dddd 49dd93 49dd49 49dd00 4993dd 4c9999 4c994c 4c9900\n");
	Bprint(ioutb, "  4949dd 4c4c99 555555 555500 4900dd 4c0099 550055 550000 440000 44cccc 44cc88 44cc44\n");
	Bprint(ioutb, "  44cc00 4488cc 448888 448844 448800 4444cc 444488 444444 444400 4400cc 440088 440044\n");
	Bprint(ioutb, "  00ff00 00aaff 00bbbb 00bb5d 00bb00 0055ff 005dbb 007777 007700 0000ff 0000bb 000077\n");
	Bprint(ioutb, "  333333 00ffff 00ffaa 00ff55 00ee4f 00ee00 009eee 00aaaa 00aa55 00aa00 004fee 0055aa\n");
	Bprint(ioutb, "  006666 006600 0000ee 0000aa 000066 222222 00eeee 00ee9e 00dd93 00dd49 00dd00 0093dd\n");
	Bprint(ioutb, "  009999 00994c 009900 0049dd 004c99 005555 005500 0000dd 000099 000055 111111 00dddd\n");
	Bprint(ioutb, "  00cccc 00cc88 00cc44 00cc00 0088cc 008888 008844 008800 0044cc 004488 004444 004400\n");
	Bprint(ioutb, "  0000cc 000088 000044 000000>\n");
	Bprint(ioutb, " ] setcolorspace\n");
	Bprint(ioutb, " /decodemat [0 255] def\n");
	Bprint(ioutb, "}\n");
/*	# else, bps != 8 */
	Bprint(ioutb, "{\n");
/* is it greyscale or is it 24-bit color? */
	Bprint(ioutb, " grey true eq {\n");
	Bprint(ioutb, "  [/DeviceGray] setcolorspace\n");
	Bprint(ioutb, "  /decodemat [1 0] def\n");
	Bprint(ioutb, " }\n");
	Bprint(ioutb, " {\n");
/* must be color */
	Bprint(ioutb, "  [/DeviceRGB] setcolorspace\n");
	Bprint(ioutb, "  /bps 8 def\n");
	Bprint(ioutb, "  /decodemat [1 0 1 0 1 0] def\n");
	Bprint(ioutb, " }\n");
	Bprint(ioutb, " ifelse\n");
	Bprint(ioutb, "}\n");
	Bprint(ioutb, "ifelse\n");
	Bprint(ioutb, "/xmagnification %g def\n", xmagnification);
	Bprint(ioutb, "/ymagnification %g def\n", ymagnification);
	Bprint(ioutb, "/rotation %d def\n", rotation);
	Bprint(ioutb, "xstart ystart translate rotation rotate\n");
	Bprint(ioutb, "iwidth xmagnification mul iheight ymagnification mul scale\n");
	Bprint(ioutb, "<<\n");
	Bprint(ioutb, " /ImageType 1\n");
	Bprint(ioutb, " /Width width \n");
	Bprint(ioutb, " /Height height \n");
	Bprint(ioutb, " /BitsPerComponent bps %% bits/sample\n");
	Bprint(ioutb, " /Decode decodemat %% Brazil/Inferno cmap or DeviceGray value\n");
	Bprint(ioutb, " /ImageMatrix [width 0 0 height neg 0 height]\n");
	Bprint(ioutb, " /DataSource currentfile /ASCII85Decode filter\n");
	Bprint(ioutb, ">> \n");
	Bprint(ioutb, "image\n");
	Bprint(ioutb, "grestore\n");
	Bprint(ioutb, "} def\n");
	Bprint(ioutb, "%%%%EndProlog\n");
	if (Patch != nil)
		Bprint(ioutb, "%s\n", Patch);
	return 0;	
}

int
trailer(Biobuf *ioutb ,int pages) {
	if(!started)
		return 1;
	Bprint(ioutb, "%%%%Trailer\n%%%%Pages: %d\n%%%%EOF\n", pages);
	return 0;
}

void
printnewpage(int pagenum, int end, Biobuf *ioutb)
{
	if (!started) return;
	if (end){			
/*		# bounding box */
		if (boxes){
			Bprint(ioutb, "18 18 moveto 594 18 lineto 594 774 lineto 18 774 lineto closepath stroke\n");
		}
		Bprint(ioutb, "showpage\n%%%%EndPage %d %d\n", pagenum, pagenum);
	} else 
		Bprint(ioutb, "%%%%Page: %d %d\n", pagenum, pagenum);
}

/* int
/* printimage(FILE *ioutb, struct lineinfo line, struct iteminfo imag) {
/* 	int RM;
/* 
/* 	RM=612-18;
/* 	class:=tk->cmd(t,"winfo class "+imag.buf);
/* #sys->print("Looking for [%s] of type [%s]\n",imag.buf,class);
/* 	if (line.xorg+imag.offset+imag.width>RM)
/* 		imag.width=RM-line.xorg-imag.offset;
/* 	case class {
/* 		"button" or "menubutton" =>
/* 			# try to get the text out and print it....
/* 			ioutb.puts(sys->sprint("%d %d moveto\n",line.xorg+imag.offset,
/* 							line.yorg));
/* 			msg:=tk->cmd(t,sys->sprint("%s cget -text",imag.buf));
/* 			ft:=tk->cmd(t,sys->sprint("%s cget -font",imag.buf));
/* 			sys->print("font is [%s]\n",ft);
/* 			ioutb.puts(sys->sprint("%d %d %d %d rectstroke\n",
/* 						line.xorg+imag.offset,line.yorg,imag.width,
/* 						line.height));
/* 			return (class,msg);
/* 		"label" =>
/* 			(im,im2,err) := tk->imageget(t,imag.buf);
/* 			if (im!=nil){
/* 				bps := 1<<im.ldepth;
/* 				ioutb.puts(sys->sprint("%d %d %d %d %d %d %d %d doimage\n",
/* 						im.r.dy(),line.ascent,im.r.dx(),line.yorg,
/* 						line.xorg+imag.offset,im.r.dy(), im.r.dx(), bps));
/* 				imagebits(ioutb,im);
/* 			}
/* 			return (class,"");
/* 		"entry" =>
/* 			ioutb.puts(sys->sprint("%d %d moveto\n",line.xorg+imag.offset,
/* 					line.yorg));
/* 			ioutb.puts(sys->sprint("%d %d %d %d rectstroke\n",
/* 					line.xorg+imag.offset,line.yorg,imag.width,
/* 					line.height));
/* 			return (class,"");
/* 		* =>
/* 			sys->print("Unhandled class [%s]\n",class);
/* 			return (class,"Error");
/* 		
/* 	}
/* 	return ("","");	
/* }
/* 
/* printline(ioutb: ref Iobuf,line : lineinfo,items : array of iteminfo)
/* {
/* 	xstart:=line.xorg;
/* 	wid:=xstart;
/* 	# items
/* 	if (len items == 0) return;
/* 	for(j:=0;j<len items;j++){
/* 		msg:="";
/* 		class:="";
/* 		if (items[j].itype==IMAGE)
/* 			(class,msg)=printimage(ioutb,line,items[j]);
/* 		if (items[j].itype!=IMAGE || class=="button"|| class=="menubutton"){
/* 			setfont(ioutb,items[j].font);
/* 			if (msg!=""){ 
/* 				# position the text in the center of the label
/* 				# moveto curpoint
/* 				# (msg) stringwidth pop xstart sub 2 div
/* 				ioutb.puts(sys->sprint("%d %d moveto\n",xstart+items[j].offset,
/* 						line.yorg+line.height-line.ascent));
/* 				ioutb.puts(sys->sprint("(%s) dup stringwidth pop 2 div",
/* 								msg));
/* 				ioutb.puts(" 0 rmoveto show\n");
/* 			}
/* 			else {
/* 				ioutb.puts(sys->sprint("%d %d moveto\n",
/* 					xstart+items[j].offset,line.yorg+line.height
/* 					-line.ascent));
/* 				ioutb.puts(sys->sprint("(%s) show\n",items[j].buf));
/* 			}
/* 		}
/* 		wid=xstart+items[j].offset+items[j].width;
/* 	}
/* 	if (boxes)
/* 		ioutb.puts(sys->sprint("%d %d %d %d rectstroke\n",line.xorg,line.yorg,
/* 									wid,line.height));
/* }
/* 
/* setfont(ioutb: ref Iobuf,font : int){
/* 	ftype : int;
/* 	fname : string;
/* 	if ((curfonttype&font)!=curfonttype){
/* 		for(f:=0;f<curfont;f++){
/* 			(ftype,fname)=font_arr[f];
/* 				if ((ftype&font)==ftype)
/* 					break;
/* 		}
/* 		if (f==curfont){
/* 			fname=def_font;
/* 			ftype=def_font_type;
/* 		}
/* 		ioutb.puts(sys->sprint("%s setfont\n",fname));
/* 		curfonttype=ftype;
/* 	}
/* }
/* 	
/* parseTkline(ioutb: ref Iobuf,input : string) : string
/* {
/* 	if (!started) return noinit;
/* 	thisline : lineinfo;
/* 	PS:=792-18-18;	# page size in points	
/* 	TM:=792-18;	# top margin in points
/* 	LM:=18;		# left margin 1/4 in. in
/* 	BM:=18;		# bottom margin 1/4 in. in
/* 	x : int;
/* 	(x,input)=str->toint(input,10);
/* 	thisline.xorg=(x*PTPI)/PXPI;
/* 	(x,input)=str->toint(input,10);
/* 	thisline.yorg=(x*PTPI)/PXPI;
/* 	(x,input)=str->toint(input,10);
/* 	thisline.width=(x*PTPI)/PXPI;
/* 	(x,input)=str->toint(input,10);
/* 	thisline.height=(x*PTPI)/PXPI;
/* 	(x,input)=str->toint(input,10);
/* 	thisline.ascent=(x*PTPI)/PXPI;
/* 	(x,input)=str->toint(input,10);
/* 	# thisline.numitems=x;
/* 	if (thisline.width==0 || thisline.height==0)
/* 		return "";
/* 	if (thisline.yorg+thisline.height-pagestart>PS){
/* 		pagestart=thisline.yorg;
/* 		return "newpage";
/* 		# must resend this line....
/* 	}
/* 	thisline.yorg=TM-thisline.yorg-thisline.height+pagestart;
/* 	thisline.xorg+=LM;
/* 	(items, err) :=getline(totlines,input);
/* 	if(err != nil)
/* 		return err;
/* 	totitems+=len items;
/* 	totlines++;
/* 	printline(ioutb,thisline,items);
/* 	return "";
/* }
/* 	
/* 
/* getfonts(input: string) : string
/* {
/* 	if (!started) return "Error";
/* 	tkfont,psfont : string;
/* 	j : int;
/* 	retval := "";
/* 	if (input[0]=='%')
/* 			return "";
/* 	# get a line of the form 
/* 	# 5::/fonts/lucida/moo.16.font
/* 	# translate it to...
/* 	# 32 f32.16
/* 	# where 32==1<<5 and f32.16 is a postscript function that loads the 
/* 	# appropriate postscript font (from remap)
/* 	# and writes it to fonts....
/* 	(bits,font):=str->toint(input,10);
/* 	if (bits!=-1)
/* 		bits=1<<bits;
/* 	else{
/* 		bits=1;
/* 		def_font_type=bits;
/* 		curfonttype=def_font_type;
/* 	}
/* 	font=font[2:];
/* 	for(i:=0;i<len remap;i++){
/* 		(tkfont,psfont)=remap[i];
/* 		if (tkfont==font)
/* 			break;
/* 	}
/* 	if (i==len remap)
/* 		psfont="Times-Roman";
/* 	(font,nil)=str->splitr(font,".");
/* 	(nil,font)=str->splitr(font[0:len font-1],".");
/* 	(fsize,nil):=str->toint(font,10);
/* 	fsize=(PTPI*3*fsize)/(2*PXPI);
/* 	enc_font:="f"+string bits+"."+string fsize;
/* 	ps_func:="/"+enc_font+" /"+psfont+" findfont "+string fsize+
/* 							" scalefont def\n";
/* 	sy_font:="sy"+string fsize;
/* 	xtra_func:="/"+sy_font+" /Symbol findfont "+string fsize+
/* 							" scalefont def\n";
/* 	for(i=0;i<len font_arr;i++){
/* 		(j,font)=font_arr[i];
/* 		if (j==-1) break;
/* 	}
/* 	if (j==len font_arr)
/* 		return "Error";
/* 	font_arr[i]=(bits,enc_font);
/* 	if (bits==1)
/* 		def_font=enc_font;
/* 	curfont++;
/* 	retval+= ps_func;
/* 	retval+= xtra_func;	
/* 	return retval;
/* }
/* 
/* deffont() : string
/* {
/* 	return def_font;
/* }
/* 	
/* getline(k : int,  input : string) : (array of iteminfo, string)
/* {
/* 	lineval,args : string;
/* 	j, nb : int;
/* 	lw:=0;
/* 	wid:=0;
/* 	flags:=0;
/* 	item_arr := array[32] of {* => iteminfo(-1,-1,-1,-1,-1,-1,"")};
/* 	curitem:=0;
/* 	while(input!=nil){
/* 		(nil,input)=str->splitl(input,"[");
/* 		if (input==nil)
/* 			break;
/* 		com:=input[1];
/* 		input=input[2:];
/* 		case com {
/* 			'A' =>
/* 				nb=0;
/* 				# get the width of the item
/* 				(wid,input)=str->toint(input,10);
/* 				wid=(wid*PTPI)/PXPI;
/* 				if (input[0]!='{')
/* 					return (nil, sys->sprint(
/* 						"line %d item %d Bad Syntax : '{' expected",
/* 							k,curitem));
/* 				# get the args.
/* 				(args,input)=str->splitl(input,"}");
/* 				# get the flags.
/* 				# assume there is only one int flag..
/* 				(flags,args)=str->toint(args[1:],16);
/* 				if (args!=nil && debug){
/* 					sys->print("line %d item %d extra flags=%s\n",
/* 							k,curitem,args);
/* 				}
/* 				if (flags<1024) flags=1;
/* 				item_arr[curitem].font=flags;
/* 				item_arr[curitem].offset=lw;
/* 				item_arr[curitem].width=wid;
/* 				lw+=wid;
/* 				for(j=1;j<len input;j++){
/* 					if ((input[j]==')')||(input[j]=='('))
/* 							lineval[len lineval]='\\';
/* 					if (input[j]=='[')
/* 						nb++;
/* 					if (input[j]==']')
/* 						if (nb==0)
/* 							break;
/* 						else 
/* 							nb--;
/* 					lineval[len lineval]=input[j];
/* 				}
/* 				if (j<len input)
/* 					input=input[j:];
/* 				item_arr[curitem].buf=lineval;
/* 				item_arr[curitem].line=k;
/* 				item_arr[curitem].itype=ASCII;
/* 				curitem++;
/* 				lineval="";
/* 			'R' =>
/* 				nb=0;
/* 				# get the width of the item
/* 				(wid,input)=str->toint(input,10);
/* 				wid=(wid*PTPI)/PXPI;
/* 				if (input[0]!='{')
/* 					return (nil, "Bad Syntax : '{' expected");
/* 				# get the args.
/* 				(args,input)=str->splitl(input,"}");
/* 				# get the flags.
/* 				# assume there is only one int flag..
/* 				(flags,args)=str->toint(args[1:],16);
/* 				if (args!=nil && debug){
/* 					sys->print("line %d item %d Bad Syntax args=%s",
/* 							k,curitem,args);
/* 				}
/* 				item_arr[curitem].font=flags;
/* 				item_arr[curitem].offset=lw;
/* 				item_arr[curitem].width=wid;
/* 				lw+=wid;
/* 				for(j=1;j<len input;j++){
/* 					if (input[j]=='[')
/* 						nb++;
/* 					if (input[j]==']')
/* 						if (nb==0)
/* 							break;
/* 						else 
/* 							nb--;
/* 					case input[j] {
/* 						8226 => # bullet
/* 							lineval+="\\267 ";
/* 						169 =>  # copyright
/* 							lineval+="\\251 ";
/* 							curitem++;			
/* 						* =>
/* 							lineval[len lineval]=input[j];
/* 					}
/* 				}
/* 				if (j>len input)
/* 					input=input[j:];
/* 				item_arr[curitem].buf=lineval;
/* 				item_arr[curitem].line=k;
/* 				item_arr[curitem].itype=RUNE;
/* 				curitem++;
/* 				lineval="";
/* 			'N' or 'C'=>
/* 				# next item
/* 				for(j=0;j<len input;j++)
/* 					if (input[j]==']')
/* 						break;
/* 				if (j>len input)
/* 					input=input[j:];
/* 			'T' =>
/* 				(wid,input)=str->toint(input,10);
/* 				wid=(wid*PTPI)/PXPI;
/* 				item_arr[curitem].offset=lw;
/* 				item_arr[curitem].width=wid;
/* 				lw+=wid;
/* 				lineval[len lineval]='\t';
/* 				# next item
/* 				for(j=0;j<len input;j++)
/* 					if (input[j]==']')
/* 						break;
/* 				if (j>len input)
/* 					input=input[j:];
/* 				item_arr[curitem].buf=lineval;
/* 				item_arr[curitem].line=k;
/* 				item_arr[curitem].itype=ASCII;
/* 				curitem++;
/* 				lineval="";
/* 			'W' =>
/* 				(wid,input)=str->toint(input,10);
/* 				wid=(wid*PTPI)/PXPI;
/* 				item_arr[curitem].offset=lw;
/* 				item_arr[curitem].width=wid;
/* 				item_arr[curitem].itype=IMAGE;
/* 				lw+=wid;
/* 				# next item
/* 				for(j=1;j<len input;j++){
/* 					if (input[j]==']')
/* 						break;
/* 					lineval[len lineval]=input[j];
/* 				}
/* 				item_arr[curitem].buf=lineval;
/* 				if (j>len input)
/* 					input=input[j:];
/* 				curitem++;
/* 				lineval="";
/* 			* =>
/* 				# next item
/* 				for(j=0;j<len input;j++)
/* 					if (input[j]==']')
/* 						break;
/* 				if (j>len input)
/* 					input=input[j:];
/* 				
/* 		}
/* 	}
/* 	return (item_arr[0:curitem], "");	
/* }
*/

void
cmap2ascii85(uchar *b, uchar *c) {
	int i;
	unsigned long i1;

/*	fprintf(stderr, "addr=0x%x %x %x %x %x\n", b, b[0], b[1], b[2], b[3]); */
	b--;	/* one-index b */
	c--;	/* one-index c */
	i1 = (b[1]<<24)+(b[2]<<16)+(b[3]<<8)+b[4];
	if(i1 == 0){
		c[1] = 'z';
		c[2] = '\0';
		return;
	}
	for(i=0; i<=4; i++){
		c[5-i] = '!' + (i1 % 85);
		i1 /= 85;
	}
	c[6] = '\0';
}

static uchar *arr = nil;
ulong	onesbits = ~0;
void
imagebits(Biobuf *ioutb, Memimage *im)
{
	int spb;
	int bitoff;
	int j, n, n4, i, bpl, nrest;
	int lsf;
	uchar c85[6], *data, *src, *dst;
	Memimage *tmp;
	Rectangle r;

	tmp = nil;
	if (debug)
		fprint(2, "imagebits, r=%d %d %d %d, depth=%d\n",
			im->r.min.x, im->r.min.y, im->r.max.x, im->r.max.y, im->depth);
	width = Dx(im->r);
	height = Dy(im->r);
	bps = im->depth;	/* # bits per sample */
	bitoff = 0;		/* # bit offset of beginning sample within first byte */
	if (bps < 8) {
		spb = 8 / bps;
		bitoff = (im->r.min.x % spb) * bps;
	}
	if (bitoff != 0) {
/* 		# Postscript image wants beginning of line at beginning of byte */
		r = im->r;
		r.min.x -= bitoff/im->depth;
		r.max.x -= bitoff/im->depth;
		tmp = allocmemimage(r, im->chan);
		if(tmp == nil){
			fprint(2, "p9bitpost: allocmemimage failed: %r\n");
			exits("alloc");
		}
		memimagedraw(tmp, r, im, im->r.min, nil, ZP, S);
		im = tmp;
	}
	lsf = 0;
	/* compact data to remove word-boundary padding */
	bpl = bytesperline(im->r, im->depth);
	n = bpl*Dy(im->r);
	data = malloc(n);
	if(data == nil){
		fprint(2, "p9bitpost: malloc failed: %r\n");
		exits("malloc");
	}
	for(i=0; i<Dy(im->r); i++){
		/* memmove(data+bpl*i, byteaddr(im, Pt(im->r.min.x, im->r.min.y+i)), bpl); with inversion */
		dst = data+bpl*i;
		src = byteaddr(im, Pt(im->r.min.x, im->r.min.y+i));
		for(j=0; j<bpl; j++)
			*dst++ = 255 - *src++;
	}
	n4 = (n / 4) * 4;
	for (i = 0; i < n4; i += 4){
		cmap2ascii85(data+i, c85);
		lsf += strlen((char *)c85);
		Bprint(ioutb, "%s", c85);
		if (lsf > 74) {
			Bprint(ioutb, "\n");
			lsf = 0;
		}
	}
	nrest = n - n4;
	if (nrest != 0) {
		uchar foo[4];

		for (i=0; i<nrest; i++)
			foo[i] = data[n4+i];
		for (i=nrest; i<4; i++)
			foo[i] = '\0';
		cmap2ascii85(foo, c85);
		if (strcmp((char *)c85, "z") == 0 )
			strcpy((char *)c85, "!!!!!");
		Bprint(ioutb, "%.*s", nrest+1, c85);
	}
	Bprint(ioutb, "\n~>");
	Bprint(ioutb, "\n");
	freememimage(tmp);
}

int
image2psfile(int fd, Memimage *im, int dpi) {
	Rectangle r;
	Rectangle bbox;
	int e;
	int xmargin = 36;
	int ymargin = 36;
	double paperaspectratio;
	double imageaspectratio;
	Biobuf ioutb;
	Memimage *tmp;

	if(im->depth >= 8 && im->chan != CMAP8 && im->chan != GREY8){
		/*
		 * the postscript libraries can only handle [1248]-bit grey, 8-bit cmap,
		 * and 24-bit color, so convert.
		 */
		tmp = allocmemimage(im->r, strtochan("b8g8r8"));
		if(tmp == nil)
			return 1;
		memimagedraw(tmp, tmp->r, im, im->r.min, nil, ZP, S);
		freememimage(im);
		im = tmp;
	}

	Binit(&ioutb, fd, OWRITE);
 	r = im->r;
	width = Dx(r);
	height = Dy(r);
	imageaspectratio = (double) width / (double) height;
	if (landscape) {
		paperaspectratio = ((double)paperlength - (ymargin * 2)) / ((double)paperwidth - (xmargin * 2));
		if (dpi > 0) {
			iwidth = width * 72 / dpi;
			iheight = height * 72 / dpi;
		} else if (imageaspectratio > paperaspectratio) {
			iwidth = paperlength - (ymargin * 2);
			iheight = iwidth / imageaspectratio;
		} else {
			iheight = paperwidth - (xmargin * 2);
			iwidth  = iheight * imageaspectratio;
		}
		xstart = paperwidth - xmargin - (iheight * ymagnification);
		ystart = paperlength - ymargin;
		rotation = -90;
	} else {
		paperaspectratio = ((double)paperwidth - (xmargin * 2)) / ((double)paperlength - (ymargin * 2));
		if (dpi > 0) {
			iwidth = width * 72 / dpi;
			iheight = height * 72 / dpi;
		} else if (imageaspectratio > paperaspectratio) {
			iwidth = paperwidth - (xmargin * 2);
			iheight = iwidth / imageaspectratio;
		} else {
			iheight = paperlength - (ymargin * 2);
			iwidth  = iheight * imageaspectratio;
		}
		xstart = xmargin;
		ystart = paperlength - ymargin - (iheight * ymagnification);
		rotation = 0;
	}
	bbox = Rect(xstart,ystart,xstart+iwidth,ystart+iheight);
	e = preamble(&ioutb, bbox);
	if(e != 0)
		return e;
	Bprint(&ioutb, "%%%%Page: 1\n%%%%BeginPageSetup\n");
	Bprint(&ioutb, "/pgsave save def\n");
	Bprint(&ioutb, "%%%%EndPageSetup\n");
	bps = im->depth;
	Bprint(&ioutb, "%d 0 %d %d %d %d %d %d %s doimage\n", iheight, iwidth, ystart, xstart, height, width, bps, im->flags&Fgrey ? "true" : "false");
 	imagebits(&ioutb, im);
	Bprint(&ioutb, "pgsave restore\nshowpage\n");
	e = trailer(&ioutb, 1);
	if(e != 0)
		return e;
	Bterm(&ioutb);
	return 0;
}

/* set local variables by string and pointer to its value
 * the variables are:
 *   int magnification
 *   int landscape
 *   char *Patch
 */
void
psopt(char *s, void *val)
{
	if(s == nil)
		return;
	if(strcmp("xmagnification", s) == 0)
		xmagnification = *((double *)val);
	if(strcmp("ymagnification", s) == 0)
		ymagnification = *((double *)val);
	if(strcmp("landscape", s) == 0)
		landscape = *((int *)val);
	if(strcmp("Patch", s) == 0)
		Patch = *((char **)val);
}
