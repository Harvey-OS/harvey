% This file gives the macros for plain MetaPost
% It contains all the features of plain METAFONT except those specific to
% font-making.  (See The METAFONTbook by D.E. Knuth).
% There are also a number macros labeling figures etc.
string base_name, base_version; base_name="plain"; base_version="0.05";

message "Preloading the plain mem file, version "&base_version;

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

def gobble primary g = enddef;
primarydef g gobbled gg = enddef;
def hide(text t) = exitif numeric begingroup t;endgroup; enddef;
def ??? = hide(interim showstopping:=1; showdependencies) enddef;
def stop expr s = message s; gobble readstring enddef;

warningcheck:=1;

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


%% basic constants and mathematical macros

% numeric constants
newinternal eps,epsilon,infinity,_;
eps := .00049;    % this is a pretty small positive number
epsilon := 1/256/256;   % but this is the smallest
infinity := 4095.99998;    % and this is the largest

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
color black, white, red, green, blue, background;
black = (0,0,0);
white = (1,1,1);
red = (1,0,0);
green = (0,1,0);
blue = (0,0,1);
background = white;   % The user can reset this

% picture constants
picture blankpicture,evenly;
blankpicture=nullpicture; % `display blankpicture...'
evenly=nullpicture;
  addto evenly doublepath (0,6)..(3,6) withpen pencircle; % `dashed evenly'

% string constants
string ditto; ditto = char 34; % ASCII double-quote mark

% pen constants
pen pensquare,penrazor,penspeck;
pensquare = makepen(unitsquare shifted -(.5,.5));
penrazor = makepen((-.5,0)--(.5,0)--cycle);
penspeck=pensquare scaled eps;

% nullary operators
vardef whatever = save ?; ? enddef;

% unary operators
let abs = length;

vardef round primary u =
 if numeric u: floor(u+.5)
 elseif pair u: (hround xpart u, vround ypart u)
 else: u fi enddef;

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

vardef center primary p =
  save pp; picture pp;
  if picture p: pp=p;
  else: pp=nullpicture;
    if path p:
      addto pp if cycle p: contour else: doublepath fi p;
    fi
  fi
  .5[llcorner pp, urcorner pp]
enddef;



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


path cuttings;  % what got cut off

secondarydef a cutbefore b =  % tries to cut as little as possible
  begingroup save t;
  (t, whatever) = a intersectiontimes b;
  if t<0:
    cuttings:=point 0 of a;
    a
  else: cuttings:= subpath (0,t) of a;
    subpath (t,length a) of a
  fi
  endgroup
enddef;

tertiarydef a cutafter b =
  reverse (reverse a  cutbefore  b)
  hide(cuttings:=reverse cuttings)
enddef;



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
newinternal tolerance, tx_,fx_,x_; tolerance:=.01;

vardef buildcycle(text ll) =
  save ta_, tb_, k_, i_, pp_; path pp_[];
  k_=0;
  for q=ll: pp_[incr k_]=q; endfor
  i_=k_;
  for i=1 upto k_:
    (ta_[i], length pp_[i_]-tb_[i_]) =
      pp_[i] intersectiontimes reverse pp_[i_];
    if ta_[i]<0:
      errmessage("Paths "& decimal i &" and "& decimal i_ &" don't intersect");
    fi
    i_ := i;
  endfor
  for i=1 upto k_: subpath (ta_[i],tb_[i]) of pp_[i] .. endfor
    cycle
enddef;



%% units of measure

mm=2.83464;      pt=0.99626;        dd=1.06601;      bp:=1;
cm=28.34645;     pc=11.95517;       cc=12.79213;     in:=72;

vardef magstep primary m = mexp(46.67432m) enddef;



%% macros for drawing and filling

def drawoptions(text t) =
  def _op_ = t enddef
enddef;

linejoin:=rounded;               % parameters that effect drawing
linecap:=rounded;
miterlimit:=10;

drawoptions();

pen currentpen;
picture currentpicture;

def fill expr c = addto currentpicture contour c _op_ enddef;
def draw expr p =
  addto currentpicture
  if picture p:
    also p
  else:
    doublepath p withpen currentpen
  fi
  _op_
enddef;
def filldraw expr c =
  addto currentpicture contour c withpen currentpen
  _op_ enddef;
def drawdot expr z =
  addto currentpicture contour makepath currentpen shifted z
  _op_ enddef;

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



%% High level drawing commands

path ahcirc; newinternal ahangle;
ahcirc = fullcircle scaled 8;   % default arrowhead length 4bp
ahangle := 45;                  % default head angle 45 degrees

vardef arrowhead expr p =
  save q,e; path q; pair e;
  e = point length p of p;
  q = gobble(p shifted -e cutafter ahcirc)  cuttings;
  (q rotated -.5ahangle & reverse q rotated .5ahangle -- cycle)  shifted e
enddef;

path _apth;
def drawarrow expr p = _apth:=p; _finarr enddef;
def drawdblarrow expr p = _apth:=p; _findarr enddef;

def _finarr text t =
  draw _apth t;
  filldraw arrowhead _apth  t
enddef;

def _findarr text t =
  draw _apth t;
  fill arrowhead _apth withpen currentpen  t;
  fill arrowhead  reverse _apth  withpen currentpen  t
enddef;



%% macros for labels

newinternal bboxmargin; bboxmargin:=2pt;

vardef bbox primary p =
  llcorner p-(bboxmargin,bboxmargin) -- lrcorner p+(bboxmargin,-bboxmargin)
  -- urcorner p+(bboxmargin,bboxmargin) -- ulcorner p+(-bboxmargin,bboxmargin)
  -- cycle
enddef;

string defaultfont;
newinternal defaultscale, labeloffset;
defaultfont = "cmr10";
defaultscale := 1;
labeloffset := 3pt;

vardef thelabel@#(expr s,z) =  % Position s near z
  save p; picture p;
  if picture s:  p=s
  else:    p = s infont defaultfont scaled defaultscale
  fi;
  p shifted (z + labeloffset*laboff@# -
     (labxf@#*lrcorner p + labyf@#*ulcorner p
       + (1-labxf@#-labyf@#)*llcorner p
     )
  )
enddef;

def label = draw thelabel enddef;
vardef makelabel@#(expr s,z) =
  label@#(s,z);
  fill fullcircle scaled 3bp shifted z;
enddef;

pair laboff, laboff.lft, laboff.rt, laboff.top, laboff.bot;
pair laboff.ulft, laboff.llft, laboff.urt, laboff.lrt;
laboff    =(0,0);    labxf    =.5;  labyf    =.5;
laboff.lft=(-1,0);   labxf.lft=1;   labyf.lft=.5;
laboff.rt =(1,0);    labxf.rt =0;   labyf.rt =.5;
laboff.bot=(0,-1);   labxf.bot=.5;  labyf.bot=1;
laboff.top=(0,1);    labxf.top=.5;  labyf.top=0;
laboff.ulft=(-.7,.7);labxf.ulft=1;  labyf.ulft=0;
laboff.urt=(.7,.7);  labxf.urt=0;   labyf.urt=0;
laboff.llft=-(.7,.7);labxf.llft=1;  labyf.llft=1;
laboff.lrt=(.7,-.7); labxf.lrt=0;   labyf.lrt=1;

vardef labels@#(text t) =
 forsuffixes $=t:
   makelabel@#(str$,z$); endfor
 enddef;
vardef penlabels@#(text t) =
 forsuffixes $$=l,,r: forsuffixes $=t:
   makelabel@#(str$.$$,z$.$$); endfor endfor
 enddef;

def range expr x = numtok[x] enddef;
def numtok suffix x=x enddef;
tertiarydef m thru n =
 m for x=m+1 step 1 until n: , numtok[x] endfor enddef;



%% Overall adminstration

string extra_beginfig, extra_endfig;
extra_beginfig = extra_endfig = "";

def beginfig(expr c) =
  begingroup
  charcode:=c;
  clearxy; clearit; clearpen;
  pickup defaultpen;
  scantokens extra_beginfig;
enddef;

def endfig =
  scantokens extra_endfig;
  shipit;
  endgroup
enddef;


%% last-minute items

vardef z@#=(x@#,y@#) enddef;

def clearxy = save x,y enddef;
def clearit = currentpicture:=nullpicture enddef;
def shipit = shipout currentpicture enddef;

let bye = end; outer end,bye;

clear_pen_memory;     % initialize the `savepen' mechanism
clearit;

newinternal defaultpen;
pickup pencircle scaled .5bp;  % set default line width
defaultpen := savepen;
