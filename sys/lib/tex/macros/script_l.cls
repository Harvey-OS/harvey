% script_l.sty -- by Frank Neukam  (01 Dec 1993)
% based on Leslie Lamport's letter style.

\NeedsTeXFormat{LaTeX2e}
\def\filedate{1993/12/01}
\def\fileversion{2.0-2e}
\ProvidesClass{script_l}[\filedate\space KPB LaTeX2e document class]

\typeout{Document Style `script_l' v2.0 by Frank Neukam <01 Dec 1993>}
\input{oldlfont.sty}

\expandafter\ifx\csname language\endcsname\relax
 \errhelp{Use script_l v1.0 or install TeX3 or MLTeX.}
 \errmessage{script_l v2.0 can only be used with TeX3 or MLTeX!}
\fi

\def\tensfb{\fontfamily{\sfdefault}\fontseries{bx}\fontshape{n}\fontsize{10}{12pt}\selectfont}
\def\elvsfb{\fontfamily{\sfdefault}\fontseries{bx}\fontshape{n}\fontsize{11}{13.6pt}\selectfont}
\def\twlsfb{\fontfamily{\sfdefault}\fontseries{bx}\fontshape{n}\fontsize{12}{14.5pt}\selectfont}
\def\frtnsfb{\fontfamily{\sfdefault}\fontseries{bx}\fontshape{n}\fontsize{14.4}{18pt}\selectfont}
\def\svtnsfb{\fontfamily{\sfdefault}\fontseries{bx}\fontshape{n}\fontsize{17.28}{22pt}\selectfont}
\def\twtysfb{\fontfamily{\sfdefault}\fontseries{bx}\fontshape{n}\fontsize{20.74}{25pt}\selectfont}
\def\twfvsfb{\fontfamily{\sfdefault}\fontseries{bx}\fontshape{n}\fontsize{24.88}{30pt}\selectfont}

\def\@ptsize{2}
\DeclareOption{10pt}{\def\@ptsize{0}}
\DeclareOption{11pt}{\def\@ptsize{1}}
\DeclareOption{12pt}{\def\@ptsize{2}}
\DeclareOption{oneside}{\@twosidefalse \@mparswitchfalse}
\DeclareOption{twoside}{\@twosidetrue \@mparswitchtrue}
\DeclareOption{draft}{\overfullrule 5pt}
\DeclareOption{final}{\setlength\overfullrule{0pt}}
\ExecuteOptions{12pt,oneside,final}
\ProcessOptions
\input{script1\@ptsize.sty}
%\normalsize            % RTL 21.04.94

\let\sc@document=\document
\def\document{\sc@document
 \def\captionsenglish{%
  \def\yourrefname{Your ref.}
  \def\yourmailname{Your letter of}
  \def\myrefname{Our ref.}
  \def\customername{Customer no.}
  \def\invoicename{Invoice no.}
  \def\subjectname{Subject}
  \def\ccname{cc}
  \def\enclname{encl}
  \def\headtoname{To}
  \def\datename{Date}
  \def\pagename{Page}}
 \let\captionsUSenglish=\captionsenglish
 \def\captionsgerman{%
  \def\yourrefname{Ihr Zeichen}
  \def\yourmailname{Ihr Schreiben vom}
  \def\myrefname{Unser Zeichen}
  \def\customername{Kundennummer}
  \def\invoicename{Rechnungsnummer}
  \def\subjectname{}
  \def\ccname{Kopien an}
  \def\enclname{Anlagen}
  \def\headtoname{An}
  \def\datename{Datum}
  \def\pagename{Seite}}
 \let\captionsaustrian=\captionsgerman
 \def\captionsfrench{%
  \def\yourrefname{Vos r\'ef\'erences}
  \def\yourmailname{Votre lettre du}
  \def\myrefname{Nos r\'ef\'erences}
  \def\customername{Num\'ero de client}
  \def\invoicename{Num\'ero de facture}
  \def\subjectname{Concernant}
  \def\ccname{Copie \`a}
  \def\enclname{Annexes}
  \def\headtoname{A}
  \def\datename{Date}
  \def\pagename{Page}}
 \def\dateenglish{\def\today{\number\day/\number\month/\number\year}}
 \def\dateUSenglish{\def\today{\number\month/\number\day/\number\year}}
 \def\dategerman{\def\today{\number\day.\,\number\month.\,\number\year}}
 \let\dateaustrian=\dategerman
 \let\datefrench=\dategerman
 \ifcase\language
  \captionsUSenglish
  \dateUSenglish
 \or
  \captionsgerman
  \dategerman
 \or
  \captionsfrench
  \datefrench
 \fi}

\oddsidemargin 0in
\evensidemargin 0in
\marginparwidth 0.9in
\marginparsep 0.1in
\marginparpush 0.45\baselineskip
\topmargin -15.5mm
\headheight 22mm
\headsep 9mm
%\footheight 20mm
\footskip 30mm
\textheight 217mm
\textwidth 159.2mm

\parskip 1\baselineskip
\parindent 0pt
\smallskipamount=0.5\parskip
\medskipamount=\parskip
\bigskipamount=2\parskip
\footnotesep 0.8\baselineskip
\skip\footins 0.75\baselineskip plus 2pt minus 4pt
\skip\@mpfootins = \skip\footins
\columnsep 1cc
\columnseprule 0pt
\floatsep 1\baselineskip plus 2pt minus 2pt
\textfloatsep 20pt plus 2pt minus 4pt
\intextsep 1\baselineskip plus 2pt minus 2pt
%\@maxsep 20pt
\dblfloatsep 1\baselineskip plus 2pt minus 2pt
\dbltextfloatsep 20pt plus 2pt minus 4pt
%\@dblmaxsep 20pt
\@fptop 0pt plus 1fil
\@fpsep 0.7\baselineskip plus 2fil
\@fpbot 0pt plus 1fil
\@dblfptop 0pt plus 1fil
\@dblfpsep 0.7\baselineskip plus 2fil
\@dblfpbot 0pt plus 1fil

\arraycolsep 5pt
\tabcolsep 6pt
\arrayrulewidth 0.4pt
\doublerulesep 2pt
\fboxsep 3pt
\fboxrule 0.4pt
\tabbingsep \labelsep

\topsep 0.25\baselineskip
\partopsep 0pt
\itemsep 0.25\baselineskip
\parsep 1\baselineskip %0.25\baselineskip
\labelsep .5em
\leftmargini 2.5em
\leftmarginii 2.2em
\leftmarginiii 1.87em
\leftmarginiv 1.7em
\leftmarginv 1em
\leftmarginvi 1em
\leftmargin\leftmargini
\labelwidth\leftmargini
\advance\labelwidth-\labelsep
\def\@listI{\leftmargin\leftmargini}
\let\@listi\@listI
\def\@listii{\leftmargin\leftmarginii
 \labelwidth\leftmarginii\advance\labelwidth-\labelsep}
\def\@listiii{\leftmargin\leftmarginiii
 \labelwidth\leftmarginiii\advance\labelwidth-\labelsep}
\def\@listiv{\leftmargin\leftmarginiv
 \labelwidth\leftmarginiv\advance\labelwidth-\labelsep}
\def\@listv{\leftmargin\leftmarginv
 \labelwidth\leftmarginv\advance\labelwidth-\labelsep}
\def\@listvi{\leftmargin\leftmarginvi
 \labelwidth\leftmarginvi\advance\labelwidth-\labelsep}
\@listi

\@lowpenalty 51
\@medpenalty 151
\@highpenalty 301
\@beginparpenalty -\@lowpenalty
\@endparpenalty -\@lowpenalty
\@itempenalty -\@lowpenalty

\def\theenumi{\arabic{enumi}}
\def\theenumii{\alph{enumii}}
\def\theenumiii{\roman{enumiii}}
\def\theenumiv{\Alph{enumiv}}
\def\labelenumi{\theenumi.}
\def\labelenumii{\theenumii)}
\def\labelenumiii{\theenumiii.}
\def\labelenumiv{\theenumiv.}
\def\p@enumii{\theenumi}
\def\p@enumiii{\theenumi\theenumii)}
\def\p@enumiv{\p@enumiii\theenumiii}
\def\labelitemi{$\bullet$}
\def\labelitemii{\bf --}
\def\labelitemiii{$\triangleright$}
\def\labelitemiv{$\cdot$}

\def\verse{\let\\=\@centercr
 \list{}{\itemsep\z@ \itemindent -1.5em\listparindent \itemindent
 \rightmargin\leftmargin\advance\leftmargin 1.5em}\item[]}
\let\endverse\endlist

\def\quotation{\list{}{\listparindent 1em
 \itemindent\listparindent
 \rightmargin\leftmargin\parsep 0pt plus 1pt}\item[]}
\let\endquotation=\endlist

\def\quote{\list{}{\rightmargin\leftmargin}\item[]}
\let\endquote=\endlist

\def\descriptionlabel#1{\hspace\labelsep \sfb #1}
\def\description{\list{}{\labelwidth\z@ \itemindent-\leftmargin
 \let\makelabel\descriptionlabel}}
\let\enddescription\endlist
\newdimen\descriptionmargin
\descriptionmargin=3em

\def\labelinglabel#1{#1\hfil\sc@septext}
\def\labeling{\@ifnextchar [{\@labeling}{\@labeling[]}}
\def\@labeling[#1]#2{\def\sc@septext{#1}
 \list{}{\settowidth{\labelwidth}{#2#1}
 \leftmargin\labelwidth \advance\leftmargin by \labelsep
 \let\makelabel\labelinglabel}}
\let\endlabeling\endlist

\def\fromname{}
\def\frombranch{}                       % RTL 21.04.94
\def\fromsig{}
\def\fromaddress{}
\def\fromplace{}
\def\fromlocation{}
\def\frombackaddress{}
\def\telephonenum{}
\def\varyourref{}
\def\varyourmail{}
\def\varmyref{}
\def\varcustomer{}
\def\varinvoice{}
\def\varrefitemi{}
\def\varrefitemii{}
\def\varrefitemiii{}
\def\varrefitemnamei{}
\def\varrefitemnameii{}
\def\varrefitemnameiii{}
\def\@specialmail{}
\def\@title{}
\def\@subject{}

\def\name#1{\def\fromname{#1}}
\def\branch#1{\def\frombranch{#1}}              % RTL 21.04.94
\def\signature#1{\def\fromsig{#1}}
\long\def\address#1{\def\fromaddress{#1}}
\def\place#1{\def\fromplace{#1}}
\long\def\location#1{\def\fromlocation{#1}}
\def\backaddress#1{\def\frombackaddress{#1}}
\def\telephone#1{\def\telephonenum{#1}}
\def\yourref#1{\def\varyourref{#1}}
\def\yourmail#1{\def\varyourmail{#1}}
\def\myref#1{\def\varmyref{#1}}
\def\customer#1{\def\varcustomer{#1}}
\def\invoice#1{\def\varinvoice{#1}}
\def\refitemi#1{\def\varrefitemi{#1}}
\def\refitemii#1{\def\varrefitemii{#1}}
\def\refitemiii#1{\def\varrefitemiii{#1}}
\def\refitemnamei#1{\def\varrefitemnamei{#1}}
\def\refitemnameii#1{\def\varrefitemnameii{#1}}
\def\refitemnameiii#1{\def\varrefitemnameiii{#1}}
\def\specialmail#1{\def\@specialmail{#1}}
\def\title#1{\def\@title{#1}}
\def\subject#1{\def\@subject{{\bf #1}}}
\long\def\firsthead#1{\def\@firsthead{#1}}
\long\def\firstfoot#1{\def\@firstfoot{#1}}
\long\def\nexthead#1{\def\@nexthead{#1}}
\long\def\nextfoot#1{\def\@nextfoot{#1}}

\newif\if@fold
\newif\if@afield
\newif\if@subj
\def\foldmarkson{\@foldtrue}
\def\foldmarksoff{\@foldfalse}
\def\addrfieldon{\@afieldtrue}
\def\addrfieldoff{\@afieldfalse}
\def\subjecton{\@subjtrue}
\def\subjectoff{\@subjfalse}

\newdimen\sc@temp
\newdimen\sc@@temp
\newdimen\foldhskip
\newdimen\foldvskipi
\newdimen\foldvskipii
\newdimen\foldvskipiii
\newdimen\addrvskip
\newdimen\addrindent
\newdimen\addrwidth
\newdimen\addrheight
\newdimen\locwidth
\newdimen\refvskip
\newdimen\sigindent

\foldhskip 3.5mm
\foldvskipi 62mm % war: 65mm RTL
\foldvskipii 40.5mm
\foldvskipiii 58.5mm
\addrvskip 7.5mm
\addrindent 0mm
\addrwidth 70mm
\addrheight 35mm
\locwidth\textwidth
\advance\locwidth by -\addrwidth
\divide\locwidth by 2
\refvskip 5.5mm
\sigindent 0mm

\def\adrentry#1#2#3#4#5#6#7#8{\ifx #8\@empty \else
 \ifx #2\@empty
  \expandafter\def\csname #8\endcsname{#1\\#3}
 \else
  \expandafter\def\csname #8\endcsname{#2 #1\\#3}
 \fi \fi}
\def\adrchar#1{}

\long\def\letter#1{\newpage
 \if@twoside\ifodd\c@page\else \thispagestyle{empty}\null\newpage \fi\fi
 \c@page\@ne \interlinepenalty=200 \@processto{#1}}
\def\stopletter{}
\def\endletter{\stopletter\@@par\pagebreak\@@par}
\long\def\@processto#1{\expandafter\@xproc #1\\@@@\ifx\toaddress\@empty
 \else\expandafter\@yproc #1@@@\fi}
\long\def\@xproc #1\\#2@@@{\def\toname{#1}\def\toaddress{#2}}
\long\def\@yproc #1\\#2@@@{\def\toaddress{#2}}

\def\stopbreaks{\interlinepenalty \@M
 \def\par{\@@par\nobreak}\let\\=\@nobreakcr
 \let\vspace\@nobreakvspace}
\def\@nobreakvspace{\@ifstar{\@nobreakvspacex}{\@nobreakvspacex}}
\def\@nobreakvspacex#1{\ifvmode\nobreak\vskip #1\relax\else
 \@bsphack\vadjust{\nobreak\vskip #1}\@esphack\fi}
\def\@nobreakcr{\vadjust{\penalty\@M}\@ifstar{\@xnewline}{\@xnewline}}
\def\startbreaks{\let\\=\@normalcr
 \interlinepenalty 200\def\par{\@@par\penalty 200}}

\def\@foldmarks{\if@fold \bgroup
 \reversemarginpar\vspace{\foldvskipi}
 \marginpar{\hspace{\foldhskip}\rule{2mm}{.2pt}} \vspace{\foldvskipii}
 \marginpar{\hspace{\foldhskip}\rule{4mm}{.2pt}} \vspace{\foldvskipiii}
 \marginpar{\hspace{\foldhskip}\rule{2mm}{.2pt}}
 \vspace{-\foldvskipiii}\vspace{-\foldvskipii}\vspace{-\foldvskipi}
 \egroup \fi}

\def\@addrfield{\bgroup
 \setbox0\vbox{\hsize\addrwidth
  \ifx\frombackaddress\@empty \else
%  \underline{\scriptsize \sf \frombackaddress} \fi}
  {\scriptsize \sf \frombackaddress} \fi}
 \setbox1\vbox{\hsize\addrwidth
  \ifx\@specialmail\@empty \else \underline{\@specialmail} \fi}
 \setbox2\vbox{\hsize\addrwidth
  \toname \\ \toaddress}
 \vskip\addrvskip \hskip\addrindent
 \vbox to \addrheight{%
  \ifx\frombackaddress\@empty \else \box0 \fi
  \ifx\@specialmail\@empty \else \vfil\box1 \fi
  \vfil\box2\vfil}\egroup}

\def\@locfield{\begin{minipage}[b]{\locwidth}\fromlocation\end{minipage}}

\newif\if@ref
\def\@datefield{\@reffalse
 \ifx\varyourref\@empty \else \@reftrue \fi
 \ifx\varyourmail\@empty \else \@reftrue \fi
 \ifx\varmyref\@empty \else \@reftrue \fi
 \ifx\varcustomer\@empty \else \@reftrue \fi
 \ifx\varinvoice\@empty \else \@reftrue \fi
 \ifx\varrefitemi\@empty \else \@reftrue \fi
 \ifx\varrefitemii\@empty \else \@reftrue \fi
 \ifx\varrefitemiii\@empty \else \@reftrue \fi
 \vskip -\parskip
 \vskip \refvskip
 \if@ref
  \ifx\varyourref\@empty \else
   \settowidth{\sc@temp}{\varyourref}
   \setbox0\hbox{\scriptsize \sf \yourrefname}
   \ifdim \sc@temp > \wd0 \sc@@temp\sc@temp \else \sc@@temp\wd0 \fi
   \parbox[t]{\sc@@temp}{\noindent \box0\par \varyourref}\hfill
  \fi
  \ifx\varyourmail\@empty \else
   \settowidth{\sc@temp}{\varyourmail}
   \setbox0\hbox{\scriptsize \sf \yourmailname}
   \ifdim \sc@temp > \wd0 \sc@@temp\sc@temp \else \sc@@temp\wd0 \fi
   \parbox[t]{\sc@@temp}{\noindent \box0\par \varyourmail}\hfill
  \fi
  \ifx\varmyref\@empty \else
   \settowidth{\sc@temp}{\varmyref}
   \setbox0\hbox{\scriptsize \sf \myrefname}
   \ifdim \sc@temp > \wd0 \sc@@temp\sc@temp \else \sc@@temp\wd0 \fi
   \parbox[t]{\sc@@temp}{\noindent \box0\par \varmyref}\hfill
  \fi
  \ifx\varcustomer\@empty \else
   \settowidth{\sc@temp}{\varcustomer}
   \setbox0\hbox{\scriptsize \sf \customername}
   \ifdim \sc@temp > \wd0 \sc@@temp\sc@temp \else \sc@@temp\wd0 \fi
   \parbox[t]{\sc@@temp}{\noindent \box0\par \varcustomer}\hfill
  \fi
  \ifx\varinvoice\@empty \else
   \settowidth{\sc@temp}{\varinvoice}
   \setbox0\hbox{\scriptsize \sf \invoicename}
   \ifdim \sc@temp > \wd0 \sc@@temp\sc@temp \else \sc@@temp\wd0 \fi
   \parbox[t]{\sc@@temp}{\noindent \box0\par \varinvoice}\hfill
  \fi
  \ifx\varrefitemi\@empty \else
   \settowidth{\sc@temp}{\varrefitemi}
   \setbox0\hbox{\scriptsize \sf \varrefitemnamei}
   \ifdim \sc@temp > \wd0 \sc@@temp\sc@temp \else \sc@@temp\wd0 \fi
   \parbox[t]{\sc@@temp}{\noindent \box0\par \varrefitemi}\hfill
  \fi
  \ifx\varrefitemii\@empty \else
   \settowidth{\sc@temp}{\varrefitemii}
   \setbox0\hbox{\scriptsize \sf \varrefitemnameii}
   \ifdim \sc@temp > \wd0 \sc@@temp\sc@temp \else \sc@@temp\wd0 \fi
   \parbox[t]{\sc@@temp}{\noindent \box0\par \varrefitemii}\hfill
  \fi
  \ifx\varrefitemiii\@empty \else
   \settowidth{\sc@temp}{\varrefitemiii}
   \setbox0\hbox{\scriptsize \sf \varrefitemnameiii}
   \ifdim \sc@temp > \wd0 \sc@@temp\sc@temp \else \sc@@temp\wd0 \fi
   \parbox[t]{\sc@@temp}{\noindent \box0\par \varrefitemiii}\hfill
  \fi
  \ifx\@date\@empty \else
   \settowidth{\sc@temp}{\@date}
   \setbox0\hbox{\scriptsize \sf \datename}
   \ifdim \sc@temp > \wd0 \sc@@temp\sc@temp \else \sc@@temp\wd0 \fi
   \parbox[t]{\sc@@temp}{\noindent \box0\par \@date}
  \fi
  \par
  \vspace{2.5\baselineskip}
 \else
  {\ifx\@date\@empty \else \raggedleft\fromplace\@date\par \fi}
  \vspace{1.5\baselineskip}
 \fi}

\def\maketitle{\ifx\@title\@empty \else
 {\centering \LARGE \sfb \@title\par} \vspace{1\baselineskip} \fi}

\def\@subjfield{\if@subj%
  \ifx\@subject\@empty\else\subjectname: \fi
 \fi \@subject \par \vspace{1\baselineskip}}

\def\opening#1{\thispagestyle{firstpage} \null \@foldmarks
 \if@afield \@addrfield \hfill \@locfield \par \fi
 \@datefield
 \maketitle
 \@subjfield
 #1\par\vspace{0.5\baselineskip}\nobreak}

\long\def\closing#1{\par\nobreak\vspace{0.5\baselineskip}
 \stopbreaks \noindent \sc@temp\textwidth \advance\sc@temp by
 -\sigindent \hspace{\sigindent}%
 \parbox{\sc@temp}{\raggedright \ignorespaces #1\\[4ex]
 \ifx\@empty\fromsig \fromname \else \fromsig \fi\strut}\par
 \vspace{1.5\baselineskip}}

\def\ps{\par\startbreaks}

\def\cc#1{\par\noindent \parbox[t]{\textwidth}{\@hangfrom{\ccname: }%
 \ignorespaces #1\strut}\par}

\def\encl#1{\par\noindent \parbox[t]{\textwidth}{\@hangfrom{\enclname: }%
 \ignorespaces #1\strut}\par}

\def\footnoterule{\kern-1\p@
 \hrule width 0.4\columnwidth
 \kern .6\p@}
\long\def\@makefntext#1{\noindent \sc@temp\columnwidth%
 \advance\sc@temp by -1.5em \hbox to 1em{\hss$^{\@thefnmark}$}\hskip .5em%
 \parbox[t]{\sc@temp}{\parindent 0pt\parskip 0pt\hskip -.5em #1}}

\def\theequation{\arabic{equation}}

\setcounter{topnumber}{2}
\def\topfraction{.7}
\setcounter{bottomnumber}{1}
\def\bottomfraction{.3}
\setcounter{totalnumber}{3}
\def\textfraction{.2}
\def\floatpagefraction{.5}
\setcounter{dbltopnumber}{2}
\def\dbltopfraction{.7}
\def\dblfloatpagefraction{.5}

\def\@firsthead{\parbox[b]{\textwidth}{\begin{center}
 {\sc \ignorespaces \fromname}\\[-8pt] \rule{\textwidth}{.4pt}\\
 \ignorespaces \fromaddress \end{center}}}
\def\@firstfoot{}
\def\@nexthead{\parbox[b]{\textwidth}{{%
 \sc \ignorespaces \fromname}\\[8pt] \headtoname\ \ignorespaces \toname
 \hfill \datename:\ \@date \hfill \pagename\
 \thepage\\[-8pt] \rule{\textwidth}{.4pt}}}
\def\@nextfoot{}

\mark{{}{}}
\def\ps@plain{\def\@evenhead{\tensfb\hfil-- \thepage\ --\hfil}%
 \let\@oddhead\@evenhead%
 \def\@evenfoot{}%
 \def\@oddfoot{}}
\def\ps@firstpage{%
 \def\@evenhead{\@firsthead}%
 \let\@oddhead\@evenhead%
 \def\@evenfoot{\@firstfoot}%
 \let\@oddfoot\@evenfoot}
\def\ps@headings{%
 \def\@evenhead{\@nexthead}%
 \let\@oddhead\@evenhead%
 \def\@evenfoot{\@nextfoot}%
 \let\@oddfoot\@evenfoot}

\@foldtrue
\@afieldtrue
\@subjfalse
\ps@plain
\pagenumbering{arabic}
\raggedbottom
\frenchspacing
