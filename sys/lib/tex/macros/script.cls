% script.sty -- by Frank Neukam  (01 Dec 1993)
% based on Leslie Lamport's report and book styles.

\NeedsTeXFormat{LaTeX2e}
\def\filedate{1993/12/01}
\def\fileversion{2.0-2e}
\ProvidesClass{script}[\filedate\space KPB LaTeX2e document class]

\typeout{Document Style `script' v2.0 by Frank Neukam <01 Dec 1993>}
\input{oldlfont.sty}

\def\tensfb{\fontfamily{\sfdefault}\fontseries{bx}\fontshape{n}\fontsize{10}{12pt}\selectfont}
\def\elvsfb{\fontfamily{\sfdefault}\fontseries{bx}\fontshape{n}\fontsize{11}{13.6pt}\selectfont}
\def\twlsfb{\fontfamily{\sfdefault}\fontseries{bx}\fontshape{n}\fontsize{12}{14.5pt}\selectfont}
\def\frtnsfb{\fontfamily{\sfdefault}\fontseries{bx}\fontshape{n}\fontsize{14.4}{18pt}\selectfont}
\def\svtnsfb{\fontfamily{\sfdefault}\fontseries{bx}\fontshape{n}\fontsize{17.28}{22pt}\selectfont}
\def\twtysfb{\fontfamily{\sfdefault}\fontseries{bx}\fontshape{n}\fontsize{20.74}{25pt}\selectfont}
\def\twfvsfb{\fontfamily{\sfdefault}\fontseries{bx}\fontshape{n}\fontsize{24.88}{30pt}\selectfont}

\def\@ptsize{1}
\DeclareOption{10pt}{\def\@ptsize{0}}
\DeclareOption{11pt}{\def\@ptsize{1}}
\DeclareOption{12pt}{\def\@ptsize{2}}
\DeclareOption{18pt}{\def\@ptsize{8}}
\newdimen\paperwidth
\newdimen\paperheight
\paperwidth 210mm
\paperheight 297mm
\DeclareOption{a3paper}{\paperwidth=297mm \paperheight=420mm}
\DeclareOption{a4paper}{\paperwidth=210mm \paperheight=297mm}
\DeclareOption{a5paper}{\paperwidth=148.5mm \paperheight=210mm}
\DeclareOption{a6paper}{\paperwidth=105mm \paperheight=148.5mm}
\DeclareOption{a3lpaper}{\paperwidth=420mm \paperheight=297mm}
\DeclareOption{a4lpaper}{\paperwidth=297mm \paperheight=210mm}
\DeclareOption{a5lpaper}{\paperwidth=210mm \paperheight=148.5mm}
\DeclareOption{a6lpaper}{\paperwidth=148.5mm \paperheight=105mm}
\DeclareOption{oneside}{\@twosidefalse \@mparswitchfalse}
\DeclareOption{twoside}{\@twosidetrue \@mparswitchtrue}
\DeclareOption{draft}{\overfullrule 5pt}
\DeclareOption{final}{\setlength\overfullrule{0pt}}
\ExecuteOptions{a4paper,12pt,oneside,final}
\ProcessOptions
\input{script1\@ptsize.sty}
\normalsize                            % RTL 21.04.94

\newif\if@hincl
\def\headincludeon{\@hincltrue}
\def\headincludeoff{\@hinclfalse}
\newif\if@hsl
\def\headseplineon{\@hsltrue \@hincltrue}
\def\headseplineoff{\@hslfalse \@hinclfalse}
\headseplineoff
\newif\if@fincl
\def\footincludeon{\@fincltrue}
\def\footincludeoff{\@finclfalse}
\newif\if@fsl
\def\footseplineon{\@fsltrue \@fincltrue}
\def\footseplineoff{\@fslfalse \@finclfalse}
\footseplineoff
\newdimen\sc@bcor
\newcount\sc@div
\newdimen\sc@hblk
\newdimen\sc@vblk
\newdimen\sc@temp
\def\typearea{\@ifnextchar [{\@typearea}{\@typearea[0mm]}}
\def\@typearea[#1]#2{
 \normalsize
 \sc@bcor #1
 \sc@div=#2
 \sc@hblk \paperwidth
 \advance \sc@hblk by -\sc@bcor
 \divide\sc@hblk by \sc@div
 \oddsidemargin -1in
 \evensidemargin -1in
 \if@twoside
  \advance\oddsidemargin by \sc@hblk
  \advance\oddsidemargin by \sc@bcor
  \advance\evensidemargin by 2\sc@hblk
 \else
  \advance\oddsidemargin by 1.5\sc@hblk
  \advance\oddsidemargin by \sc@bcor
  \evensidemargin\oddsidemargin
 \fi
 \textwidth \paperwidth
 \advance\textwidth by -\sc@bcor
 \advance\textwidth by -3\sc@hblk
 \headheight 15pt
 \headsep 1.5\baselineskip
 \footskip 3.5\baselineskip
 \sc@vblk \paperheight
 \divide\sc@vblk by \sc@div
 \sc@temp \paperheight
 \advance\sc@temp by -3\sc@vblk
 \if@hincl
  \advance\sc@temp by -\headheight
  \advance\sc@temp by -\headsep
 \fi
 \if@fincl
  \advance\sc@temp by -\footskip
 \fi
 \textheight \topskip
 \@whiledim\textheight<\sc@temp
  \do{\advance\textheight by \baselineskip}
 \topmargin \sc@vblk
 \advance\topmargin by -1in
 \if@hincl \else
  \advance\topmargin by -\headheight
  \advance\topmargin by -\headsep
 \fi
 \if@twoside
  \marginparwidth 1.5\sc@hblk
 \else
  \marginparwidth \sc@hblk
 \fi
 \marginparsep 1cc
 \marginparpush 0.45\baselineskip}
\def\areaset{\@ifnextchar [{\@areaset}{\@areaset[0mm]}}
\def\@areaset[#1]#2#3{
 \normalsize
 \sc@bcor #1
 \textwidth #2
 \textheight #3
 \sc@hblk \paperwidth
 \advance\sc@hblk by -\sc@bcor
 \advance\sc@hblk by -\textwidth
 \divide\sc@hblk by 3
 \oddsidemargin -1in
 \evensidemargin -1in
 \if@twoside
  \advance\oddsidemargin by \sc@hblk
  \advance\oddsidemargin by \sc@bcor
  \advance\evensidemargin by 2\sc@hblk
 \else
  \advance\oddsidemargin by 1.5\sc@hblk
  \advance\oddsidemargin by \sc@bcor
  \evensidemargin\oddsidemargin
 \fi
 \headheight 15pt
 \headsep 1.5\baselineskip
 \footskip 3.5\baselineskip
 \sc@vblk \paperheight
 \advance\sc@vblk by -\textheight
 \divide\sc@vblk by 3
 \if@hincl
  \advance\textheight by -\headheight
  \advance\textheight by -\headsep
 \fi
 \if@fincl
  \advance\textheight by -\footskip
 \fi
 \topmargin \sc@vblk
 \advance\topmargin by -1in
 \if@hincl \else
  \advance\topmargin by -\headheight
  \advance\topmargin by -\headsep
 \fi
 \if@twoside
  \marginparwidth 1.5\sc@hblk
 \else
  \marginparwidth \sc@hblk
 \fi
 \marginparsep 1cc
 \marginparpush 0.45\baselineskip}
\ifcase \@ptsize\relax \typearea{8} \or \typearea{10} \or \typearea{12} \fi

\parskip 0pt plus 1pt
\parindent 1em
\partopsep 3pt plus 2pt minus 2pt
\footnotesep 0.6\baselineskip
\skip\footins 0.75\baselineskip plus 4pt minus 2pt
\skip\@mpfootins = \skip\footins
\columnsep 1cc
\columnseprule 0pt
\floatsep 1\baselineskip plus 2pt minus 2pt
\textfloatsep 20pt plus 2pt minus 4pt
\intextsep 1\baselineskip plus 2pt minus 2pt
%\@maxsep 20pt                          % RTL 21.04.94
\dblfloatsep 1\baselineskip plus 2pt minus 2pt
\dbltextfloatsep 20pt plus 2pt minus 4pt
%\@dblmaxsep 20pt                       % RTL 21.04.94
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
\def\@listI{\leftmargin\leftmargini
 \topsep 0.8em plus 0.2em minus 0.4em
 \parsep 0.4em plus 2pt minus 1pt
 \itemsep \parsep}
\let\@listi\@listI
\@listi
\def\@listii{\leftmargin\leftmarginii
 \labelwidth\leftmarginii\advance\labelwidth-\labelsep
 \topsep 0.4em plus 2pt minus 1pt
 \parsep 0.2em plus 1pt minus 1pt
 \itemsep \parsep}
\def\@listiii{\leftmargin\leftmarginiii
 \labelwidth\leftmarginiii\advance\labelwidth-\labelsep
 \topsep 0.2em plus 1pt minus 1pt
 \parsep \z@
 \partopsep 1pt plus 0pt minus 1pt
 \itemsep \topsep}
\def\@listiv{\leftmargin\leftmarginiv
 \labelwidth\leftmarginiv\advance\labelwidth-\labelsep}
\def\@listv{\leftmargin\leftmarginv
 \labelwidth\leftmarginv\advance\labelwidth-\labelsep}
\def\@listvi{\leftmargin\leftmarginvi
 \labelwidth\leftmarginvi\advance\labelwidth-\labelsep}

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

\newcounter{part}
\newcounter {chapter}
\newcounter {section}[chapter]
\newcounter {subsection}[section]
\newcounter {subsubsection}[subsection]
\newcounter {paragraph}[subsubsection]
\newcounter {subparagraph}[paragraph]

\def\thepart {\Roman{part}}
\def\thechapter {\arabic{chapter}}
\def\thesection {\thechapter.\arabic{section}}
\def\thesubsection {\thesection.\arabic{subsection}}
\def\thesubsubsection {\thesubsection .\arabic{subsubsection}}
\def\theparagraph {\thesubsubsection.\arabic{paragraph}}
\def\thesubparagraph {\theparagraph.\arabic{subparagraph}}

\setcounter{secnumdepth}{2}
\setcounter{tocdepth}{2}

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

\def\descriptionlabel#1{\hspace\labelsep {\descfont #1}}
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

\def\marginline#1{\marginpar%
 {\if@twoside \ifodd\c@page \raggedright \else \raggedleft \fi
  \else \raggedright \fi #1}}

\def\titlepage{\@restonecolfalse\if@twocolumn\@restonecoltrue\onecolumn
 \else \newpage \fi \thispagestyle{empty}\c@page\z@}
\def\endtitlepage{\if@restonecol\twocolumn \else \newpage \fi}

\def\next@tpage{\newpage\thispagestyle{empty}\null}
\def\@extratitle{}
\long\def\extratitle#1{\def\@extratitle{#1}}
\def\@titlehead{}
\long\def\titlehead#1{\def\@titlehead{#1}}
\def\@subject{}
\def\subject#1{\def\@subject{#1}}
\def\@publishers{}
\def\publishers#1{\def\@publishers{#1}}
\def\@uppertitleback{}
\long\def\uppertitleback#1{\def\@uppertitleback{#1}}
\def\@lowertitleback{}
\long\def\lowertitleback#1{\def\@lowertitleback{#1}}
\def\@dedication{}
\def\dedication#1{\def\@dedication{#1}}
\def\maketitle{\@ifnextchar [{\@maketitle}{\@maketitle[1]}}
\def\@maketitle[#1]{\bgroup \@restonecolfalse
 \if@twocolumn \@restonecoltrue\onecolumn \else \newpage \fi
 \thispagestyle{empty}\null
 \setcounter{page}{#1}
 \let\footnotesize\small
 \let\footnoterule\relax
 \ifx\@extratitle\@empty \else
  \noindent\@extratitle \next@tpage \if@twoside \next@tpage \fi
 \fi
 \ifx\@titlehead\@empty \else
  \noindent\begin{minipage}[t]{\textwidth}
  \@titlehead
  \end{minipage}\par
 \fi
 \null \vfill
 \ifx\@subject\@empty \else
  {\centering \Large \@subject \par}
  \vskip 3em
 \fi
 {\centering \huge {\sectfont \@title \par}}
 \vskip 3em
 {\centering \Large \lineskip 0.75em
  \begin{tabular}[t]{c}\@author
  \end{tabular}\par}
 \vskip 1.5em
 {\centering \Large \@date \par}
 \vskip 0pt plus 3fill
 {\centering \Large \@publishers \par}
 \vskip 3em
 \par \@thanks
 \if@twoside \next@tpage
  \noindent\begin{minipage}[t]{\textwidth}
  \@uppertitleback
  \end{minipage}\par
  \vfill
  \noindent\begin{minipage}[b]{\textwidth}
  \@lowertitleback
  \end{minipage}
 \fi
 \ifx\@dedication\@empty \else
  \next@tpage \vfill
  {\centering \Large \@dedication \par}
  \vskip 0pt plus 3fill
  \if@twoside \next@tpage \fi
 \fi
 \if@restonecol\twocolumn \else \newpage \fi
 \egroup
 \setcounter{footnote}{0}
 \let\thanks\relax \gdef\@extratitle{}\gdef\@titlehead{}\gdef\@subject{}%
 \gdef\@thanks{}\gdef\@author{}\gdef\@title{}\gdef\@publishers{}%
 \gdef\@uppertitleback{}\gdef\@lowertitleback{}\gdef\@dedication{}%
 \let\maketitle\relax \let\@maketitle\relax}

\newif\if@abstrt
\def\abstracton{\@abstrttrue}
\def\abstractoff{\@abstrtfalse}
\def\abstractname{Abstract}
\def\abstract{\@restonecolfalse\if@twocolumn\@restonecoltrue\onecolumn
 \else \newpage \fi \thispagestyle{empty} \null \vfil
 \if@abstrt \begin{center} {{\sectfont \abstractname}} \end{center}\fi}
\def\endabstract{\par \vfil \null
 \if@twoside \next@tpage \fi
 \if@restonecol\twocolumn \else \newpage \fi}

\def\partname{Part}
\def\part{\cleardoublepage \thispagestyle{empty} \if@twocolumn \onecolumn
 \@tempswatrue \else \@tempswafalse \fi \null \vfil \secdef\@part\@spart}
\def\@part[#1]#2{\ifnum \c@secnumdepth >-2\relax \refstepcounter{part}
 \addcontentsline{toc}{part}{\thepart\hspace{1em}#1}\else
 \addcontentsline{toc}{part}{#1}\fi \markboth{}{}
 {\centering \ifnum \c@secnumdepth >-2\relax \huge {\sectfont\partname{}
 \thepart \par} \vskip 20pt \fi \Huge {\sectfont #2\par}}\@endpart}
\def\@spart#1{{\centering \Huge {\sectfont #1}\par}\@endpart}
\def\@endpart{\vfil\newpage \if@twoside \null \thispagestyle{empty}
 \newpage \fi \if@tempswa \twocolumn \fi}

\def\chaptername{Chapter}
\def\@chapapp{\chaptername}
\def\chaptermark#1{}
\def\chapter{\clearpage \thispagestyle{plain} \global\@topnum\z@
 \@afterindentfalse \secdef\@chapter\@schapter}
\def\@chapter[#1]#2{\ifnum \c@secnumdepth >\m@ne
  \refstepcounter{chapter}
  \typeout{\@chapapp\space\thechapter.}
  \addcontentsline{toc}{chapter}{\protect \numberline{\thechapter.}#1}
 \else
  \addcontentsline{toc}{chapter}{#1}\fi
 \chaptermark{#1}
 \addtocontents{lof}{\protect\addvspace{10pt}}
 \addtocontents{lot}{\protect\addvspace{10pt}}
 \if@twocolumn \@topnewpage[\@makechapterhead{#2}]
 \else \@makechapterhead{#2} \fi \@afterheading}
\def\@schapter#1{\if@twocolumn \@topnewpage[\@makeschapterhead{#1}]
 \else \@makeschapterhead{#1} \fi \@afterheading}
\def\@makechapterhead#1{\vspace*{50pt} {\Huge {\sectfont
 \@hangfrom{\ifnum \c@secnumdepth >\m@ne \thechapter. \fi}%
 {\raggedright \interlinepenalty \@M #1\par}} \nobreak \vskip 40pt}}
\def\@makeschapterhead#1{\vspace*{50pt} {\Huge {\sectfont \@hangfrom{}%
 {\raggedright \interlinepenalty \@M #1\par}}} \nobreak \vskip 40pt}

\def\addchap{\clearpage \thispagestyle{plain} \global\@topnum\z@
 \@afterindentfalse \secdef\@addchap\@saddchap}
\def\@addchap[#1]#2{\typeout{#2}
 \addcontentsline{toc}{chapter}{#1}
 \if@twoside \markboth{#1}{}\else \markright{#1} \fi
 \addtocontents{lof}{\protect\addvspace{10pt}}
 \addtocontents{lot}{\protect\addvspace{10pt}}
 \if@twocolumn \@topnewpage[\@makeschapterhead{#2}]
 \else \@makeschapterhead{#2} \fi \@afterheading}
\def\@saddchap#1{\markboth{}{}
 \if@twocolumn \@topnewpage[\@makeschapterhead{#1}]
 \else \@makeschapterhead{#1} \fi \@afterheading}

\def\appendixname{Appendix}
\def\appendix{\par
 \setcounter{chapter}{0}
 \setcounter{section}{0}
 \def\@chapapp{\appendixname}
 \def\thechapter{\Alph{chapter}}}

\def\section{\@startsection {section}{1}{\z@}{-3.5ex plus -1ex minus
 -.2ex}{2.3ex plus .2ex}{\Large \sectfont}}
\def\subsection{\@startsection{subsection}{2}{\z@}{-3.25ex plus -1ex minus
 -.2ex}{1.5ex plus .2ex}{\large \sectfont}}
\def\subsubsection{\@startsection{subsubsection}{3}{\z@}{-3.25ex plus -1ex
 minus -.2ex}{1.5ex plus .2ex}{\sectfont}}
\def\paragraph{\@startsection{paragraph}{4}{\z@}{3.25ex plus 1ex minus
 .2ex}{-1em}{\sectfont}}
\def\subparagraph{\@startsection{subparagraph}{4}{\parindent}{3.25ex plus
 1ex minus .2ex}{-1em}{\sectfont}}

\def\minisec#1{\@afterindentfalse \vskip 1.5ex {\parindent 0pt \raggedright
 \sectfont #1\par\nobreak}\@afterheading}

\def\contentsname{Contents}
\def\@pnumwidth{1.55em}
\def\@tocrmarg {2.55em}
\def\@dotsep{4.5}
\def\tableofcontents{\@restonecolfalse
 \if@twocolumn\@restonecoltrue\onecolumn\fi
 \chapter*{\contentsname \@mkboth{\contentsname}{\contentsname}}
 \@starttoc{toc}\if@restonecol\twocolumn\fi}
\def\l@part#1#2{\addpenalty{-\@highpenalty}
 \addvspace{2.25em plus 1pt} \begingroup
 \@tempdima 3em \parindent \z@ \rightskip \@pnumwidth \parfillskip
 -\@pnumwidth
 {\sectfont \leavevmode #1\hfil \hbox to\@pnumwidth{\hss #2}}\par
 \nobreak \global\@nobreaktrue \everypar{\global\@nobreakfalse\everypar{}}
 \endgroup}
\def\l@chapter#1#2{\addpenalty{-\@highpenalty}
 \vskip 1.0em plus 1pt \@tempdima 1.5em \begingroup
 \parindent \z@ \rightskip \@pnumwidth
 \parfillskip -\@pnumwidth
 \sectfont \leavevmode \advance\leftskip\@tempdima \hskip -\leftskip
 #1\nobreak\hfil\nobreak\hbox to\@pnumwidth{\hss #2}\par
 \penalty\@highpenalty \endgroup}
\def\l@section{\@dottedtocline{1}{1.5em}{2.3em}}
\def\l@subsection{\@dottedtocline{2}{3.8em}{3.2em}}
\def\l@subsubsection{\@dottedtocline{3}{7.0em}{4.1em}}
\def\l@paragraph{\@dottedtocline{4}{10em}{5em}}
\def\l@subparagraph{\@dottedtocline{5}{12em}{6em}}

\def\listfigurename{List of Figures}
\def\listoffigures{\@restonecolfalse
 \if@twocolumn\@restonecoltrue\onecolumn\fi
 \chapter*{\listfigurename \@mkboth{\listfigurename}{\listfigurename}}
 \@starttoc{lof}\if@restonecol\twocolumn\fi}
\def\l@figure{\@dottedtocline{1}{1.5em}{2.3em}}

\def\listtablename{List of Tables}
\def\listoftables{\@restonecolfalse
 \if@twocolumn\@restonecoltrue\onecolumn\fi
 \chapter*{\listtablename \@mkboth{\listtablename}{\listtablename}}
 \@starttoc{lot}\if@restonecol\twocolumn\fi}
\let\l@table\l@figure

\def\bibname{Bibliography}
\def\thebibliography#1{\chapter*{\bibname \@mkboth{\bibname}{\bibname}}
 \list{[\arabic{enumi}]}{\settowidth\labelwidth{[#1]}\leftmargin\labelwidth
 \advance\leftmargin\labelsep
 \usecounter{enumi}}
 \def\newblock{\hskip 0.11em plus 0.33em minus 0.07em}
 \sloppy\clubpenalty4000\widowpenalty4000
 \sfcode`\.=1000\relax}
\let\endthebibliography=\endlist

\def\indexname{Index}
\newif\if@restonecol
\def\theindex{\@restonecoltrue\if@twocolumn\@restonecolfalse\fi
 \columnseprule \z@
 \columnsep 35pt\twocolumn[\@makeschapterhead{\indexname}]
 \@mkboth{\indexname}{\indexname}
 \thispagestyle{plain}\parindent\z@
 \parskip\z@ plus 0.3pt\relax\let\item\@idxitem}
\def\@idxitem{\par\hangindent 40pt}
\def\subitem{\par\hangindent 40pt \hspace*{20pt}}
\def\subsubitem{\par\hangindent 40pt \hspace*{30pt}}
\def\endtheindex{\if@restonecol\onecolumn\else\clearpage\fi}
\def\indexspace{\par \vskip 10pt plus 5pt minus 3pt\relax}

\@addtoreset{footnote}{chapter}
\def\footnoterule{\kern-3\p@
 \hrule width 0.4\columnwidth
 \kern 2.6\p@}
\long\def\@makefntext#1{\noindent \sc@temp\columnwidth%
 \advance\sc@temp by -1.5em \hbox to 1em{\hss$^{\@thefnmark}$}\hskip .5em%
 \parbox[t]{\sc@temp}{\parindent 0pt\parskip 0pt\hskip -.5em #1}}

\@addtoreset{equation}{chapter}
\def\theequation{\thechapter.\arabic{equation}}

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

\long\def\@makecaption#1#2{\vskip 10pt
 \setbox\@tempboxa\hbox{{\captfont #1: #2}}
 \ifdim \wd\@tempboxa >\hsize \@hangfrom{{\captfont #1: }}%
 {\captfont #2\par}
 \else \hbox to\hsize{\hfil\box\@tempboxa\hfil}
 \fi}

\def\figurename{Figure}
\newcounter{figure}[chapter]
\def\thefigure{\thechapter.\@arabic\c@figure}
\def\fps@figure{tbp}
\def\ftype@figure{1}
\def\ext@figure{lof}
\def\fnum@figure{\figurename{} \thefigure}
\def\figure{\@float{figure}}
\let\endfigure\end@float
\@namedef{figure*}{\@dblfloat{figure}}
\@namedef{endfigure*}{\end@dblfloat}

\def\tablename{Table}
\newcounter{table}[chapter]
\def\thetable{\thechapter.\@arabic\c@table}
\def\fps@table{tbp}
\def\ftype@table{2}
\def\ext@table{lot}
\def\fnum@table{\tablename{} \thetable}
\def\table{\@float{table}}
\let\endtable\end@float
\@namedef{table*}{\@dblfloat{table}}
\@namedef{endtable*}{\end@dblfloat}

\mark{{}{}}
\if@twoside
 \def\ps@plain{\let\@mkboth\@gobbletwo
  \def\@evenhead{}%
  \def\@oddhead{}%
  \def\@evenfoot{\vbox{\hsize=\textwidth
   \if@fsl \hrule \vskip 3pt \fi
   \hbox to \textwidth{{\pnumfont\thepage\hfil}}}}%
  \def\@oddfoot{\vbox{\hsize=\textwidth
   \if@fsl \hrule \vskip 3pt \fi
   \hbox to \textwidth{{\pnumfont\hfil\thepage}}}}}
 \def\ps@headings{\let\@mkboth\markboth
  \def\@evenhead{\vbox{\hsize=\textwidth
   \hbox to \textwidth{{\headfont\leftmark\hfil}}
   \if@hsl \vskip 1.5pt \hrule \fi}}%
  \def\@oddhead{\vbox{\hsize=\textwidth
   \hbox to \textwidth{{\headfont\hfil\rightmark}}
   \if@hsl \vskip 1.5pt \hrule \fi}}%
  \def\@evenfoot{\vbox{\hsize=\textwidth
   \if@fsl \hrule \vskip 3pt \fi
   \hbox to \textwidth{{\pnumfont\thepage\hfil}}}}%
  \def\@oddfoot{\vbox{\hsize=\textwidth
   \if@fsl \hrule \vskip 3pt \fi
   \hbox to \textwidth{{\pnumfont\hfil\thepage}}}}%
  \def\chaptermark##1{\markboth {\ifnum \c@secnumdepth >\m@ne
   \thechapter. \ \fi ##1}{}}%
  \def\sectionmark##1{\markright {\ifnum \c@secnumdepth >\z@
   \thesection\ \ \fi ##1}}%
  \def\subsectionmark##1{}}
 \def\ps@myheadings{\let\@mkboth\@gobbletwo
  \def\@evenhead{\vbox{\hsize=\textwidth
   \hbox to \textwidth{{\headfont\leftmark\hfil}}
   \if@hsl \vskip 1.5pt \hrule \fi}}%
  \def\@oddhead{\vbox{\hsize=\textwidth
   \hbox to \textwidth{{\headfont\hfil\rightmark}}
   \if@hsl \vskip 1.5pt \hrule \fi}}%
  \def\@evenfoot{\vbox{\hsize=\textwidth
   \if@fsl \hrule \vskip 3pt \fi
   \hbox to \textwidth{{\pnumfont\thepage\hfil}}}}%
  \def\@oddfoot{\vbox{\hsize=\textwidth
   \if@fsl \hrule \vskip 3pt \fi
   \hbox to \textwidth{{\pnumfont\hfil\thepage}}}}%
  \def\chaptermark##1{}%
  \def\sectionmark##1{}%
  \def\subsectionmark##1{}}
\else
 \def\ps@plain{\let\@mkboth\@gobbletwo
  \def\@evenhead{}%
  \def\@oddhead{}%
  \def\@evenfoot{}%
  \def\@oddfoot{\vbox{\hsize=\textwidth
   \if@fsl \hrule \vskip 3pt \fi
   \hbox to \textwidth{{\pnumfont\hfil\thepage\hfil}}}}}
 \def\ps@headings{\let\@mkboth\markboth
  \def\@evenhead{}%
  \def\@oddhead{\vbox{\hsize=\textwidth
   \hbox to \textwidth{{\headfont\hfil\rightmark\hfil}}
   \if@hsl \vskip 1.5pt \hrule \fi}}%
  \def\@evenfoot{}%
  \def\@oddfoot{\vbox{\hsize=\textwidth
   \if@fsl \hrule \vskip 3pt \fi
   \hbox to \textwidth{{\pnumfont\hfil\thepage\hfil}}}}%
  \def\chaptermark##1{\markright {\ifnum \c@secnumdepth >\m@ne
   \thechapter. \ \fi ##1}}%
  \def\sectionmark##1{}%
  \def\subsectionmark##1{}}
 \def\ps@myheadings{\let\@mkboth\@gobbletwo
  \def\@evenhead{}%
  \def\@oddhead{\vbox{\hsize=\textwidth
   \hbox to \textwidth{{\headfont\hfil\rightmark\hfil}}
   \if@hsl \vskip 1.5pt \hrule \fi}}%
  \def\@evenfoot{}%
  \def\@oddfoot{\vbox{\hsize=\textwidth
   \if@fsl \hrule \vskip 3pt \fi
   \hbox to \textwidth{{\pnumfont\hfil\thepage\hfil}}}}%
  \def\chaptermark##1{}%
  \def\sectionmark##1{}%
  \def\subsectionmark##1{}}
\fi

\def\today{\ifcase\month\or
 January\or February\or March\or April\or May\or June\or
 July\or August\or September\or October\or November\or December\fi
 \space\number\day, \number\year}

\@abstrttrue
\ps@headings
\pagenumbering{arabic}
%\onecolumn                             % RTL 21.04.94
\if@twoside \else \raggedbottom \fi
\frenchspacing
\def\captfont{\rm}
\def\descfont{\sfb}
\def\headfont{\it}
\def\pnumfont{\rm}
\def\sectfont{\sfb}
