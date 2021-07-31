% This is the plain MetaPost base that's look like what's described
% in The METAFONTbook.
string base_name, base_version; base_name="mfplain"; base_version="0.02";

message "Preloading the plain base, version "&base_version&": preliminaries,";

delimiters ();  % this makes parentheses behave like parentheses
def upto = step 1 until enddef; % syntactic sugar
def downto = step -1 until enddef;
def exitunless expr c = exitif not c enddef;
let relax = \;  % ignore the word `relax', as in TeX
let \\ = \; % double relaxation is like single
def ]] = ] ] enddef; % right brackets should be loners
def -- = {curl 1}..{curl 1} enddef;
def --- = .. tension infinity .. enddef;
def ... = .. tension atleast 1 .. enddef;

newinternal smoothing, autorounding, turningcheck, granularity;
warningcheck:=1;
smoothing:=1; autorounding:=2;  % These are ignored by MetaPost
turningcheck:=1; granularity:=1;

def gobble primary g = enddef;
primarydef g gobbled gg = enddef;
def hide(text t) = exitif numeric begingroup t;endgroup; enddef;
def ??? = hide(interim showstopping:=1; showdependencies) enddef;
def stop expr s = message s; gobble readstring enddef;

def interact = % sets up to make "show" commands stop
 hide(showstopping:=1; tracingonline:=1) enddef;

def loggingall =        % puts tracing info into the log
 tracingcommands:=3; tracingtitles:=1; tracingequations:=1;
 tracingcapsules:=1; tracingspecs:=1; tracingchoices:=1;
 tracingstats:=1; tracingoutput:=1; tracingmacros:=1; tracingrestores:=1;
 enddef;

def tracingall =        % turns on every form of tracing
 tracingonline:=1; showstopping:=1; loggingall enddef;

def tracingnone =       % turns off every form of tracing
 tracingcommands:=0; tracingtitles:=0; tracingequations:=0;
 tracingcapsules:=0; tracingspecs:=0; tracingchoices:=0;
 tracingstats:=0; tracingoutput:=0; tracingmacros:=0; tracingrestores:=0;
 enddef;

message " basic constants and mathematical macros,";

% numeric constants
newinternal eps,epsilon,infinity,_;
eps := .00049;    % this is a pretty small positive number
epsilon := 1/256/256;   % but this is the smallest
infinity := 4095.99998;    % and this is the largest
_ := -1; % internal constant to make macros unreadable but shorter

newinternal mitered, rounded, beveled, butt, squared;
mitered:=0; rounded:=1; beveled:=2; % linejoin types
butt:=0;    rounded:=1; squared:=2; % linecap types


% pair constants
pair right,left,up,down,origin;
origin=(0,0); up=-down=(0,1); right=-left=(1,0);

% path constants
path quartercircle,halfcircle,fullcircle,unitsquare;
fullcircle = makepath pencircle;
halfcircle = subpath (0,4) of fullcircle;
quartercircle = subpath (0,2) of fullcircle;
unitsquare=(0,0)--(1,0)--(1,1)--(0,1)--cycle;

% transform constants
transform identity;
for z=origin,right,up: z transformed identity = z; endfor

% color constants
color black, white, red, green, blue;
black = (0,0,0);
white = (1,1,1);
red = (1,0,0);
green = (0,1,0);
blue = (0,0,1);

% picture constants
picture blankpicture,unitpixel;
blankpicture=nullpicture;
unitpixel=nullpicture; addto unitpixel contour unitsquare;

% string constants
string ditto; ditto = char 34; % ASCII double-quote mark

% pen constants
def capsule_def(suffix s) primary u = def s = u enddef enddef;
pen pensquare,penrazor,penspeck;
pensquare = makepen(unitsquare shifted -(.5,.5));
penrazor = makepen((-.5,0)--(.5,0)--cycle);
penspeck=pensquare scaled eps;

% nullary operators
vardef whatever = save ?; ? enddef;

% unary operators
let abs = length;

vardef round primary u = u enddef;
vardef hround primary x = x enddef;
vardef vround primary y = y enddef;

vardef ceiling primary x = -floor(-x) enddef;

vardef byte primary s =
 if string s: ASCII fi s enddef;

vardef dir primary d = right rotated d enddef;

vardef unitvector primary z = z/abs z enddef;

vardef inverse primary T =
 transform T_; T_ transformed T = identity; T_ enddef;

vardef counterclockwise primary c =
 if turningnumber c <= 0: reverse fi c enddef;

vardef tensepath expr r =
 for k=0 upto length r - 1: point k of r --- endfor
 if cycle r: cycle else: point infinity of r fi enddef;

% binary operators

primarydef x mod y = (x-y*floor(x/y)) enddef;
primarydef x div y = floor(x/y) enddef;
primarydef w dotprod z = (xpart w * xpart z + ypart w * ypart z) enddef;

primarydef x**y = if y=2: x*x else: takepower y of x fi enddef;
def takepower expr y of x =
 if x>0: mexp(y*mlog x)
 elseif (x=0) and (y>0): 0
 else: 1
  if y=floor y:
   if y>=0: for n=1 upto y: *x endfor
   else: for n=_ downto y: /x endfor
   fi
  else: hide(errmessage "Undefined power: " & decimal x&"**"&decimal y)
  fi fi enddef;

vardef direction expr t of p =
 postcontrol t of p - precontrol t of p enddef;

vardef directionpoint expr z of p =
 a_:=directiontime z of p;
 if a_<0: errmessage("The direction doesn't occur"); fi
 point a_ of p enddef;

secondarydef p intersectionpoint q =
 begingroup save x_,y_; (x_,y_)=p intersectiontimes q;
 if x_<0: errmessage("The paths don't intersect"); origin
 else: .5[point x_ of p, point y_ of q] fi endgroup
enddef;

tertiarydef p softjoin q =
 begingroup c_:=fullcircle scaled 2join_radius shifted point 0 of q;
 a_:=ypart(c_ intersectiontimes p); b_:=ypart(c_ intersectiontimes q);
 if a_<0:point 0 of p{direction 0 of p} else: subpath(0,a_) of p fi
  ... if b_<0:{direction infinity of q}point infinity of q
   else: subpath(b_,infinity) of q fi endgroup enddef;
newinternal join_radius,a_,b_; path c_;

% special operators
vardef incr suffix $ = $:=$+1; $ enddef;
vardef decr suffix $ = $:=$-1; $ enddef;

def reflectedabout(expr w,z) =    % reflects about the line w..z
 transformed
  begingroup transform T_;
  w transformed T_ = w;  z transformed T_ = z;
  xxpart T_ = -yypart T_; xypart T_ = yxpart T_; % T_ is a reflection
  T_ endgroup enddef;

def rotatedaround(expr z, d) =    % rotates d degrees around z
 shifted -z rotated d shifted z enddef;
let rotatedabout = rotatedaround;   % for roundabout people

vardef min(expr u)(text t) = % t is a list of numerics, pairs, or strings
 save u_; setu_ u; for uu = t: if uu<u_: u_:=uu; fi endfor
 u_ enddef;

vardef max(expr u)(text t) = % t is a list of numerics, pairs, or strings
 save u_; setu_ u; for uu = t: if uu>u_: u_:=uu; fi endfor
 u_ enddef;

def setu_ primary u =
 if pair u: pair u_ elseif string u: string u_ fi;
 u_=u enddef;

def flex(text t) =           % t is a list of pairs
 hide(n_:=0; for z=t: z_[incr n_]:=z; endfor
  dz_:=z_[n_]-z_1)
 z_1 for k=2 upto n_-1: ...z_[k]{dz_} endfor ...z_[n_] enddef;
newinternal n_; pair z_[],dz_;

def superellipse(expr r,t,l,b,s)=
 r{up}...(s[xpart t,xpart r],s[ypart r,ypart t]){t-r}...
 t{left}...(s[xpart t,xpart l],s[ypart l,ypart t]){l-t}...
 l{down}...(s[xpart b,xpart l],s[ypart l,ypart b]){b-l}...
 b{right}...(s[xpart b,xpart r],s[ypart r,ypart b]){r-b}...cycle enddef;

vardef interpath(expr a,p,q) =
 for t=0 upto length p-1: a[point t of p, point t of q]
  ..controls a[postcontrol t of p, postcontrol t of q]
   and a[precontrol t+1 of p, precontrol t+1 of q] .. endfor
 if cycle p: cycle
 else: a[point infinity of p, point infinity of q] fi enddef;

vardef solve@#(expr true_x,false_x)= % @#(true_x)=true, @#(false_x)=false
 tx_:=true_x; fx_:=false_x;
 forever: x_:=.5[tx_,fx_]; exitif abs(tx_-fx_)<=tolerance;
 if @#(x_): tx_ else: fx_ fi :=x_; endfor
 x_ enddef; % now x_ is near where @# changes from true to false
newinternal tolerance, tx_,fx_,x_; tolerance:=.1;

message " macros for converting units,";

newinternal bpppix_, bp_per_pixel;  % drawing is done in `pixel' units
bpppix_:=0.02;

mm*bpppix_=2.83464;      pt*bpppix_=0.99626;
dd*bpppix_=1.06601;      bp*bpppix_=1;
cm*bpppix_=28.34645;     pc*bpppix_=11.95517;
cc*bpppix_=12.79213;     in*bpppix_=72;

mm#=2.84528;      pt#=1;        dd#=1.07001;      bp#=1.00375;
cm#=28.45276;     pc#=12;       cc#=12.84010;     in#=72.27;

newinternal hppp, vppp;
hppp:=pt;   vppp:=pt;

newinternal blacker, o_correction; % device-oriented corrections

def define_pixels(text t) =
 forsuffixes $=t: $:=$.#*pt; endfor enddef;
def define_blacker_pixels(text t) =
 forsuffixes $=t: $:=$.#*pt+blacker; endfor enddef;
def define_corrected_pixels(text t) =
 forsuffixes $=t: $:=$.#*pt*o_correction; endfor enddef;

def define_whole_pixels = define_pixels enddef;
def define_whole_vertical_pixels = define_pixels enddef;
def define_good_x_pixels = define_pixels enddef;
def define_good_y_pixels = define_pixels enddef;
def define_whole_blacker_pixels = define_blacker_pixels enddef;
def define_whole_vertical_blacker_pixels = define_blacker_pixels enddef;
def define_horizontal_corrected_pixels = define_corrected_pixels enddef;

def lowres_fix(text t) expr ratio = enddef;

message " macros and tables for various modes of operation,";

transform currenttransform;
aspect_ratio=1;
let o_=\; let _o_=\;

def mode_setup =
 if unknown mode: mode=proof; fi
 scantokens if string mode:("input "&mode) else: mode_name[mode] fi;
 if unknown mag: mag=1; fi
 bp_per_pixel:=bpppix_*mag;
 scantokens extra_setup; % the user's special last-minute adjustments
 clearit;
 pickup pencircle scaled (.4pt+blacker);
 enddef;
def smode = string mode; mode enddef;
string extra_setup, mode_name[];
extra_setup="";          % usually there's nothing special to do

vardef magstep primary m = mexp(46.67432m) enddef;

def mode_def suffix $ =
 $:=incr number_of_modes;
 mode_name[$]:=str$ & "_";
 expandafter quote def scantokens mode_name[$] enddef;
newinternal number_of_modes;


newinternal proofing; % <0 to supress output; >1 to do labels
color proofcolor;     % color for output when proofing>0
proofcolor =.3[white,black];
color background;
background = white;

% proof mode: for initial design of characters
mode_def proof =
 proofing:=2;                   % yes, we're making full proofs
 fontmaking:=0;                 % no, we're not making a font
 tracingtitles:=1;              % yes, show titles online
 blacker:=0;                    % no additional blackness
 o_correction:=1;               % no reduction in overshoot
 if unknown mag: mag=36; else: mag:=36mag; fi
 enddef;

% smoke mode: for label-free proofs to mount on the wall
mode_def smoke =
 proof_;                        % same as proof mode, except:
 proofing:=1;                   % yes, we're making unlabeled proofs
 proofcolor:=black;             % with solid black pixels
 let makebox=maketicks;         % make the boxes less obtrusive
 if unknown mag: mag=36; else: mag:=36mag; fi
 enddef;

% lowres mode: for certain devices that print 200 pixels per inch
mode_def lowres =
 proofing:=0;                   % no, we're not making proofs
 fontmaking:=1;                 % yes, we are making a font
 tracingtitles:=0;              % no, don't show titles at all
 blacker:=0;                    % no extra blackness with PostScript
 o_correction:=.4;              % but don't overshoot as much
 enddef;

localfont:=lowres;      % the mode most commonly used to make fonts

% It is not likely that additional modes are needed, but if they are,
% additional mode_def commands should be in another input file that
% gets loaded after the PLAIN base.  The auxiliary file should set
% base_version:=base_version&"/localname".


message " macros for drawing and filling,";

def pc_ =
  hide(if proofing>0: def pc_=do_pc_ enddef; else: def pc_= enddef; fi) pc_
enddef;
def do_pc_ = withcolor proofcolor enddef;

linejoin:=rounded;               % parameters that effect drawing
linecap:=rounded;
miterlimit:=10;

pen currentpen;
picture currentpicture;

def fill expr c = addto currentpicture contour c pc_ enddef;
def draw expr p =
  addto currentpicture
  if picture p:
    also p
  else:
    doublepath p withpen currentpen
  fi
  pc_
enddef;
def filldraw expr c =
  addto currentpicture contour c withpen currentpen
  pc_ enddef;
def drawdot expr z =
  addto currentpicture contour makepath currentpen shifted z
  pc_ enddef;

def unfill expr c = fill c withcolor background enddef;
def undraw expr p = draw p withcolor background enddef;
def unfilldraw expr c = filldraw c withcolor background enddef;
def undrawdot expr z = drawdot z withcolor background enddef;
def erase text t =
  def _e_ = withcolor background hide(def _e_=enddef;) enddef;
  t _e_
enddef;
def _e_= enddef;

def cutdraw text t =
  begingroup interim linecap:=butt; draw t _e_; endgroup enddef;

def pickup secondary q =
 if numeric q: numeric_pickup_ else: pen_pickup_ fi q enddef;
def numeric_pickup_ primary q =
 if unknown pen_[q]: errmessage "Unknown pen"; clearpen
 else: currentpen:=pen_[q];
  pen_lft:=pen_lft_[q];
  pen_rt:=pen_rt_[q];
  pen_top:=pen_top_[q];
  pen_bot:=pen_bot_[q];
  currentpen_path:=pen_path_[q] fi; enddef;
def pen_pickup_ primary q =
  currentpen:=q;
  pen_lft:=xpart penoffset down of currentpen;
  pen_rt:=xpart penoffset up of currentpen;
  pen_top:=ypart penoffset left of currentpen;
  pen_bot:=ypart penoffset right of currentpen;
  path currentpen_path; enddef;
newinternal pen_lft,pen_rt,pen_top,pen_bot,pen_count_;

vardef savepen = pen_[incr pen_count_]=currentpen;
 pen_lft_[pen_count_]=pen_lft;
 pen_rt_[pen_count_]=pen_rt;
 pen_top_[pen_count_]=pen_top;
 pen_bot_[pen_count_]=pen_bot;
 pen_path_[pen_count_]=currentpen_path;
 pen_count_ enddef;

def clearpen = currentpen:=nullpen;
 pen_lft:=pen_rt:=pen_top:=pen_bot:=0;
 path currentpen_path;
 enddef;
def clear_pen_memory =
 pen_count_:=0;
 numeric pen_lft_[],pen_rt_[],pen_top_[],pen_bot_[];
 pen currentpen,pen_[];
 path currentpen_path, pen_path_[];
 enddef;

vardef lft primary x = x + if pair x: (pen_lft,0) else: pen_lft fi enddef;
vardef rt primary x = x + if pair x: (pen_rt,0) else: pen_rt fi enddef;
vardef top primary y = y + if pair y: (0,pen_top) else: pen_top fi enddef;
vardef bot primary y = y + if pair y: (0,pen_bot) else: pen_bot fi enddef;
vardef good.x primary x = x enddef;
vardef good.y primary y = y enddef;
vardef good.lft primary z = z_ enddef;
vardef good.rt primary z = z_ enddef;
vardef good.top primary z = z_ enddef;
vardef good.bot primary z = z_ enddef;

vardef penpos@#(expr b,d) =
 (x@#r-x@#l,y@#r-y@#l)=(b,0) rotated d;
 x@#=.5(x@#l+x@#r); y@#=.5(y@#l+y@#r) enddef;

def penstroke text t =
 forsuffixes e = l,r: path_.e:=t; endfor
 fill path_.l -- reverse path_.r -- cycle enddef;
path path_.l,path_.r;

message " macros for proof labels and rules,";

string defaultfont;
newinternal defaultscale, labeloffset;
defaultfont = "cmr10";
defaultscale := 1;
labeloffset := 3;

vardef makelabel@#(expr s,z) = % puts string s near point z
  picture p;
  if known z:
    p = s infont defaultfont scaled (defaultscale/bp_per_pixel);
    draw p shifted (z + labeloffset/bp_per_pixel*laboff@# -
       (labxf@#*lrcorner p + labyf@#*ulcorner p
         + (1-labxf@#-labyf@#)*llcorner p
       )
    ) withcolor black;
    fill fullcircle scaled (3/bp_per_pixel) shifted z withcolor black;
  fi
enddef;

string lcode_;  % just in case someone refers to this
pair laboff, laboff.lft, laboff.rt, laboff.top, laboff.bot;
laboff.lft=(-1,0);   labxf.lft=1;   labyf.lft=.5;
laboff.rt =(1,0);    labxf.rt =0;   labyf.rt =.5;
laboff.bot=(0,-1);   labxf.bot=.5;  labyf.bot=1;
laboff.top=(0,1);    labxf.top=.5;  labyf.top=0;
laboff=laboff.top;   labxf=labxf.top; labyf=labyf.top;
% There could be fancy code to keep labels from overlapping!

vardef labels@#(text t) =
 if proofing>1: forsuffixes $=t:
  makelabel@#(str$,z$); endfor
 fi enddef;
vardef penlabels@#(text t) =
 if proofing>1: forsuffixes $$=l,,r: forsuffixes $=t:
  makelabel@#(str$.$$,z$.$$); endfor endfor
 fi enddef;

def range expr x = numtok[x] enddef;
def numtok suffix x=x enddef;
tertiarydef m thru n =
 m for x=m+1 step 1 until n: , numtok[x] endfor enddef;

def proofrule(expr w,z) =
  begingroup interim linecap:=squared;
  draw w..z withpen pencircle scaled (.4/bp_per_pixel) withcolor black;
  endgroup
enddef;
def screenrule(expr w,z) = enddef;
pen rulepen;

def makegrid(text xlist,ylist) =
 xmin_ := min(xlist); xmax_ := max(xlist);
 ymin_ := min(ylist); ymax_ := max(ylist);
 for x=xlist: proofrule((x,ymin_), (x,ymax_)); endfor
 for y=ylist: proofrule((xmin_,y), (xmax_,y)); endfor
 enddef;

vardef titlefont suffix $ = enddef;
vardef labelfont suffix $ = defaultfont:=str$ enddef;
vardef grayfont suffix $ = enddef;
vardef slantfont suffix $ = enddef;
def proofoffset primary z = enddef;
vardef proofrulethickness expr x =
 rulepen := pencircle scaled x enddef;

message " macros for character and font administration,";

def beginchar(expr c,w_sharp,h_sharp,d_sharp) =
 begingroup
 charcode:=if known c: byte c else: 0 fi;
 charwd:=w_sharp;      charht:=h_sharp;       chardp:=d_sharp;
 w:=charwd*pt; h:=charht*pt; d:=chardp*pt;
 charic:=0; clearxy; clearit; clearpen; scantokens extra_beginchar;
 enddef;

def italcorr expr x_sharp = if x_sharp>0: charic:=x_sharp fi enddef;

def change_width = enddef;

def endchar =
 scantokens extra_endchar;
 if proofing>0: makebox(proofrule); fi
 chardx:=w;     % desired width of the character in pixels
 shipit;
 endgroup enddef;

string extra_beginchar, extra_endchar;
extra_beginchar=extra_endchar="";

def makebox(text r) =
 for y=0,h,-d: r((0,y),(w,y)); endfor % horizontals
 for x=0,w:   r((x,-d),(x,h)); endfor % verticals
 if charic<>0: r((w+charic*pt,h),(w+charic*pt,.5h)); fi
 enddef;

def maketicks(text r) =
 for y=0,h,-d: r((0,y),(10,y)); r((w-10,y),(w,y)); endfor
 for x=0,w: r((x,10-d),(x,-d)); r((x,h-10),(x,h)); endfor
 if charic<>0: r((w+charic*pt,h-10),(w+charic*pt,h)); fi
 enddef;

def font_size expr x = designsize:=x enddef;
def font_slant expr x = fontdimen 1: x enddef;
def font_normal_space expr x = fontdimen 2: x enddef;
def font_normal_stretch expr x = fontdimen 3: x enddef;
def font_normal_shrink expr x = fontdimen 4: x enddef;
def font_x_height expr x = fontdimen 5: x enddef;
def font_quad expr x = fontdimen 6: x enddef;
def font_extra_space expr x = fontdimen 7: x enddef;

def font_identifier expr x = font_identifier_:=x enddef;
def font_coding_scheme expr x = font_coding_scheme_:=x enddef;
string font_identifier_, font_coding_scheme_;
font_identifier_=font_coding_scheme_="UNSPECIFIED";

message "and a few last-minute items.";

vardef z@#=(x@#,y@#) enddef;

def openit = enddef;
def showit = enddef;

def clearxy = save x,y enddef;
def clearit = currentpicture:=nullpicture enddef;
def shipit =
  if proofing>=0:
    shipout currentpicture transformed
      (identity shifted (xoffset,yoffset) scaled bp_per_pixel)
  fi
enddef;
def cullit = enddef;
newinternal xoffset, yoffset;

def screenchars = enddef;
def screenstrokes = enddef;
def imagerules = enddef;
def gfcorners = enddef;
def nodisplays = enddef;
def notransforms = enddef;

let bye = end; outer end,bye;

clear_pen_memory;     % initialize the `savepen' mechanism
mode_setup;           % establish proof mode as the default
numeric mode,mag;     % but leave mode and mag undefined
