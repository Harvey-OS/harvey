% Macros for boxes
def beginbox_(expr pp,sp)(suffix $)(text t) =
  pair $off, $c;
  string pproc_$,sproc_$; pproc_$=pp; sproc_$=sp;
  picture pic_$; pic_$ = nullpicture;
  for _p_=t:
    pic_$:=
      if picture _p_: _p_
      else: _p_ infont defaultfont scaled defaultscale
      fi;
  endfor
  $c = $off + .5[llcorner pic_$, urcorner pic_$]
enddef;


def endbox_(suffix cl, $) =
  if known pic_.prevbox: _dojoin(prevbox,$); fi
  def prevbox = $ enddef;
  expandafter def expandafter clearboxes expandafter =
    clearboxes cl($);
  enddef
enddef;


def boxjoin(text t) =
  def prevbox=_ enddef;
  def _dojoin(suffix a,b) = t enddef;
enddef;


extra_beginfig := extra_beginfig
  & "boxjoin();def clearboxes=save pic_,sproc_,pproc_;enddef;";
extra_endfig := extra_endfig & "clearboxes";


vardef fixsize(text t) =
  forsuffixes $=t:  scantokens sproc_$($);  endfor
enddef;

vardef fixpos(text t) =
  forsuffixes $=t:
    if unknown xpart $.off:  xpart $.off=0; fi
    if unknown ypart $.off:  ypart $.off=0; fi
  endfor
enddef;


vardef bpath@# =
  fixsize(@#); fixpos(@#);
  scantokens pproc_@#(@#)
enddef;

vardef pic@# =
  fixsize(@#); fixpos(@#);
  pic_@# shifted @#off
enddef;


def drawboxed(text t) =
  fixsize(t); fixpos(t);
  forsuffixes s=t: draw pic.s; draw bpath.s; endfor
enddef;

def drawunboxed(text t) =
  fixsize(t); fixpos(t);
  forsuffixes s=t: draw pic.s; endfor
enddef;

def drawboxes(text t) =
  forsuffixes s=t: draw bpath.s; endfor
enddef;



% Rectangular boxes

newinternal defaultdx, defaultdy;
defaultdx := defaultdy := 3bp;

vardef boxit@#(text tt) =
  beginbox_("boxpath_","sizebox_",@#,tt);
  pair @#sw, @#s, @#se, @#e, @#ne, @#n, @#nw, @#w;
  0 = xpart (@#nw-@#sw) = ypart(@#se-@#sw);
  0 = xpart(@#ne-@#se) = ypart(@#ne-@#nw);
  @#w = .5[@#nw,@#sw];
  @#s = .5[@#sw,@#se];
  @#e = .5[@#ne,@#se];
  @#n = .5[@#ne,@#nw];
  @#ne-@#c = @#c-@#sw = (@#dx,@#dy) + .5*(urcorner pic_@# - llcorner pic_@#);
  endbox_(clearb_,@#);
enddef;

def boxpath_(suffix $) =
  $.sw -- $.se -- $.ne -- $.nw -- cycle
enddef;

def sizebox_(suffix $) =
  if unknown $.dx: $.dx=defaultdx; fi
  if unknown $.dy: $.dy=defaultdy; fi
enddef;

vardef clearb_(suffix $) =
  numeric $sw, $s, $se, $e, $ne, $n, $nw, $w;
  numeric $c, $off, $dx, $dy;
enddef;



% Circular and oval boxes

newinternal circmargin; circmargin:=2pt;  % default clearance for picture corner

vardef circleit@#(text tt) =
  beginbox_("thecirc_","sizecirc_",@#,tt);
  pair @#n, @#s, @#e, @#w;
  @#e-@#c = @#c-@#w = (@#dx,0) + .5*(lrcorner pic_@#-llcorner pic_@#);
  @#n-@#c = @#c-@#s = (0,@#dy) + .5*(ulcorner pic_@#-llcorner pic_@#);
  endbox_(clearc_,@#);
enddef;

def thecirc_(suffix $) =
  $.e{up} ... $.n{left} ... $.w{down} ... $.s{right} ... cycle
enddef;

vardef clearc_(suffix $) =
  numeric $n, $s, $e, $w;
  numeric $c, $off, $dx, $dy;
enddef;

vardef sizecirc_(suffix $) =
  save a_, b_;
  (a_,b_) = .5*(urcorner pic_$ - llcorner pic_$);
  if unknown $dx:
    if unknown $dy:
      if unknown($dy-$dx): a_+$dx=b_+$dy; fi
      if a_+$dx=b_+$dy: a_+$dx = a_++b_ + circmargin;
      else: $dx =
        pathsel_(0, max(a_,b_+$dx-$dy), (a_+d,0){up}...(0,b_+d+$dy-$dx){left});
      fi
    else: $dx = pathsel_(0, a_, (a_+d,0){up}...(0,b_+$dy){left});
    fi
  elseif unknown $dy:
    $dy = pathsel_(0, b_, (a_+$dx,0){up}...(0,b_+d){left});
  fi
enddef;

vardef pathsel_(expr dlo,dhi)(text tt) =
  save f_, p_; path p_;
  p_ = origin..(a_,b_)+circmargin*unitvector(a_,b_);
  vardef f_(expr d) =
    xpart((tt) intersectiontimes p_) >= 0
  enddef;
  solve f_(dlo,dhi)
enddef;
