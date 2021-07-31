%% 
%% This is file `refrep.cls', generated 
%% on <1995/2/26> with the docstrip utility (2.2i).
%% 
%% The original source files were:
%% 
%% refman.dtx  (with options: `refrep')
%% 
%% Copyright (C) 1988-1994 by H. Partl. 
%% Copyright (C) 1994 by Axel Kielhorn. All rights reserved. 
%% For additional copyright information see further down in this file. 
%% 
%% This file is to be used with the LaTeX2e system. 
%% ------------------------------------------------ 
%% 
%% This system is distributed in the hope that it will be useful, 
%% but WITHOUT ANY WARRANTY; without even the implied warranty of 
%% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
%% 
%% 
%% IMPORTANT NOTICE: 
%% 
%% For error reports in case of UNCHANGED versions see upload file. 
%% 
%% Please do not request updates from us directly. Distribution is 
%% done through Mail-Servers and TeX organizations. 
%% 
%% You are not allowed to change this file. 
%% 
%% You are allowed to distribute this file under the condition that 
%% it is distributed together with all files mentioned in 00readme.l2e. 
%% 
%% If you receive only some of these files from someone, complain! 
%% 
%% You are NOT ALLOWED to distribute this file alone. You are NOT 
%% ALLOWED to take money for the distribution or use of either this 
%% file or a changed version, except for a nominal charge for copying 
%% etc. 
%% \CharacterTable
%%  {Upper-case    \A\B\C\D\E\F\G\H\I\J\K\L\M\N\O\P\Q\R\S\T\U\V\W\X\Y\Z
%%   Lower-case    \a\b\c\d\e\f\g\h\i\j\k\l\m\n\o\p\q\r\s\t\u\v\w\x\y\z
%%   Digits        \0\1\2\3\4\5\6\7\8\9
%%   Exclamation   \!     Double quote  \"     Hash (number) \#
%%   Dollar        \$     Percent       \%     Ampersand     \&
%%   Acute accent  \'     Left paren    \(     Right paren   \)
%%   Asterisk      \*     Plus          \+     Comma         \,
%%   Minus         \-     Point         \.     Solidus       \/
%%   Colon         \:     Semicolon     \;     Less than     \<
%%   Equals        \=     Greater than  \>     Question mark \?
%%   Commercial at \@     Left bracket  \[     Backslash     \\
%%   Right bracket \]     Circumflex    \^     Underscore    \_
%%   Grave accent  \`     Left brace    \{     Vertical bar  \|
%%   Right brace   \}     Tilde         \~}
\NeedsTeXFormat{LaTeX2e}[1994/06/01]
\ProvidesClass{refrep}
              [1994/10/30 v1.9e
     LaTeX document class]
\newcommand\@ptsize{}
\newif\if@restonecol
\newif\if@titlepage
\@titlepagetrue
\newif\if@openright
\newif\if@openbib
\@openbibfalse
\DeclareOption{a4paper}
   {\setlength\paperheight {297mm}%
    \setlength\paperwidth  {210mm}}
\DeclareOption{a5paper}
   {\setlength\paperheight {210mm}%
    \setlength\paperwidth  {148mm}}
\DeclareOption{b5paper}
   {\setlength\paperheight {250mm}%
    \setlength\paperwidth  {176mm}}
\DeclareOption{letterpaper}
   {\setlength\paperheight {11in}%
    \setlength\paperwidth  {8.5in}}
\DeclareOption{legalpaper}
   {\setlength\paperheight {14in}%
    \setlength\paperwidth  {8.5in}}
\DeclareOption{executivepaper}
   {\setlength\paperheight {10.5in}%
    \setlength\paperwidth  {7.25in}}
\DeclareOption{landscape}
   {\setlength\@tempdima   {\paperheight}%
    \setlength\paperheight {\paperwidth}%
    \setlength\paperwidth  {\@tempdima}}
\DeclareOption{square}
   {\setlength\paperheight {\paperwidth}}
\DeclareOption{10pt}{\renewcommand\@ptsize{0}}
\DeclareOption{11pt}{\renewcommand\@ptsize{1}}
\DeclareOption{12pt}{\renewcommand\@ptsize{2}}
\DeclareOption{oneside}{\@twosidefalse \@mparswitchfalse}
\DeclareOption{twoside}{\@twosidetrue  \@mparswitchtrue}
\DeclareOption{draft}{\setlength\overfullrule{5pt}}
\DeclareOption{final}{\setlength\overfullrule{0pt}}
\DeclareOption{titlepage}{\@titlepagetrue}
\DeclareOption{notitlepage}{\@titlepagefalse}
\DeclareOption{openright}{\@openrighttrue}
\DeclareOption{openany}{\@openrightfalse}
\DeclareOption{onecolumn}{\@twocolumnfalse}
\DeclareOption{twocolumn}{%
    \ClassError{Refrep}
    {There is no twocolumn layout in this class!}
    {Can you imagine how twocolumn layout will look\MessageBreak
     in this class? That's why!}
    \@twocolumnfalse}
\DeclareOption{leqno}{\input{leqno.clo}}
\DeclareOption{fleqn}{\input{fleqn.clo}}
\DeclareOption{openbib}{\@openbibtrue}
\newif\ifdescriptioncolon \descriptioncolontrue
\newif\ifdescriptionleft  \descriptionleftfalse
\newif\ifmaxipagerule     \maxipageruletrue
\ExecuteOptions{letterpaper,10pt,oneside,onecolumn,final,openany}
\ProcessOptions
\input{size1\@ptsize.clo}
\setlength\lineskip{1\p@}
\setlength\normallineskip{1\p@}
\renewcommand\baselinestretch{}
\setlength\parskip{0.5\baselineskip \@plus 2\p@}
\setlength\parindent {\z@}
\@lowpenalty   51
\@medpenalty  151
\@highpenalty 301
\setlength\headsep   {\baselineskip}
\newdimen\leftmarginwidth
\newdimen\fullwidth
\newdimen\emptyfoottopmargin
\newdimen\emptyheadtopmargin
\newcommand\settextfraction[1]%
{
  \setlength\@tempdima{\paperwidth}
  \addtolength\@tempdima{-2in}
  \if@twocolumn
      \setlength\textwidth{\@tempdima}
  \else
      \setlength\fullwidth{\@tempdima}
      \@settopoint\fullwidth
      \setlength\textwidth{#1\fullwidth} % This may change
  \fi
  \@settopoint\textwidth
  \setlength\leftmarginwidth{\fullwidth}
  \addtolength\leftmarginwidth{-\textwidth}
  \setlength\@tempdima        {\paperwidth}
  \addtolength\@tempdima      {-2in}
  \addtolength\@tempdima      {-\fullwidth}
  \setlength\oddsidemargin    {.5\@tempdima}
  \addtolength\oddsidemargin  {\leftmarginwidth}
  \@settopoint\oddsidemargin
  \setlength\evensidemargin  {\oddsidemargin}
  \setlength\marginparwidth  {\oddsidemargin}
  \addtolength\marginparwidth {-\marginparsep}
  \setlength\marginparpush{0\p@}
}
\settextfraction {0.7}

\setlength\@tempdima{\paperheight}
\addtolength\@tempdima{-2in}
\addtolength\@tempdima{-.5in}
\divide\@tempdima\baselineskip
\@tempcnta=\@tempdima
\setlength\textheight{\@tempcnta\baselineskip}
\advance\textheight by \topskip
\setlength\topmargin{\paperheight}
\addtolength\topmargin{-2in}
\addtolength\topmargin{-\headheight}
\addtolength\topmargin{-\headsep}
\addtolength\topmargin{-\textheight}
\addtolength\topmargin{-\footskip}     % this might be wrong!
\addtolength\topmargin{-.5\topmargin}
\@settopoint\topmargin
\setlength\emptyfoottopmargin{\topmargin}
\addtolength\emptyfoottopmargin{\baselineskip}
\setlength\emptyheadtopmargin{\topmargin}
\addtolength\emptyheadtopmargin{-\baselineskip}
\setcounter{topnumber}{2}
\renewcommand\topfraction{.7}
\setcounter{bottomnumber}{1}
\renewcommand\bottomfraction{.3}
\setcounter{totalnumber}{3}
\renewcommand\textfraction{.2}
\renewcommand\floatpagefraction{.5}
\setcounter{dbltopnumber}{2}
\renewcommand\dbltopfraction{.7}
\renewcommand\dblfloatpagefraction{.5}
\if@twoside
  \def\ps@plain{%
    \let\@mkboth\@gobbletwo
    \let\@oddhead\@empty
    \let\@evenhead\@empty
    \def\@oddfoot{\reset@font\rmfamily\hfil\thepage}
    \def\@evenfoot{\reset@font\rmfamily\thepage\hfil}}
\else
  \def\ps@plain{%
    \let\@mkboth\@gobbletwo
    \let\@oddhead\@empty
    \def\@oddfoot{\reset@font\rmfamily\hfil\thepage\hfil}
    \let\@evenhead\@empty
    \let\@evenfoot\@oddfoot}
\fi
\if@twoside
    \def\ps@headings{%
      \let\@oddfoot\@empty\let\@evenfoot\@empty
      \def\@evenhead{\hss\vbox to \z@{\vss\hsize=\fullwidth
      \hbox to \fullwidth{\thepage\hfil\slshape\leftmark}
      \vskip 3\p@ \hrule}}%
      \def\@oddhead{\hss\vbox to \z@{\vss\hsize=\fullwidth
      \hbox to \fullwidth{{\slshape\rightmark}\hfil\thepage}
      \vskip 3\p@ \hrule}}%
      \let\@mkboth\markboth
      \topmargin\emptyfoottopmargin
      \def\chaptermark##1{%
        \markboth {\ifnum \c@secnumdepth >\m@ne
            \@chapapp\ \thechapter \ \fi
            ##1}{}}%
      \def\sectionmark##1{%
        \markright {\ifnum \c@secnumdepth >\z@
            \thesection \ \fi
            ##1}}}
\else
    \def\ps@headings{%
      \let\@oddfoot\@empty
      \def\@oddhead{\hss\vbox to \z@{\vss\hsize=\fullwidth
      \hbox to \fullwidth{{\slshape\rightmark}\hfil\thepage}
      \vskip 3\p@ \hrule}}%
      \let\@mkboth\markboth
      \topmargin\emptyfoottopmargin
      \def\chaptermark##1{%
        \markright {\ifnum \c@secnumdepth >\m@ne
            \@chapapp\ \thechapter \ \fi
            ##1}}}
\fi

\if@twoside
    \def\ps@footings{%
      \let\@oddhead\@empty\let\@evenhead\@empty
      \def\@evenfoot{\hss\vbox to \z@{\vss\hsize=\fullwidth
      \hrule \vskip 3\p@
      \hbox to \fullwidth{\thepage\hfil\slshape\leftmark}}}%
      \def\@oddfoot{\hss\vbox to \z@{\vss\hsize=\fullwidth
      \hrule \vskip 3\p@
      \hbox to \fullwidth{{\slshape\rightmark}\hfil\thepage}}}%
      \let\@mkboth\markboth
      \topmargin\emptyheadtopmargin
      \def\chaptermark##1{%
        \markboth {\ifnum \c@secnumdepth >\m@ne
            \@chapapp\ \thechapter \ \fi
            ##1}{}}%
      \def\sectionmark##1{%
        \markright {\ifnum \c@secnumdepth >\z@
            \thesection \ \fi
            ##1}}}
\else
    \def\ps@footings{%
      \let\@oddhead\@empty
      \def\@oddfoot{\hss\vbox to \z@{\vss\hsize=\fullwidth
      \hrule \vskip 3\p@
      \hbox to \fullwidth{{\slshape\rightmark}\hfil\thepage}}}%
      \let\@mkboth\markboth
      \topmargin\emptyheadtopmargin
      \def\chaptermark##1{%
        \markright {\ifnum \c@secnumdepth >\m@ne
            \@chapapp\ \thechapter \ \fi
            ##1}}}
\fi
\def\ps@myheadings{%
    \let\@oddfoot\@empty\let\@evenfoot\@empty
    \def\@evenhead{\hss\vbox to \z@{\vss\hsize=\fullwidth
    \hbox to \fullwidth{\thepage\hfil\slshape\leftmark}
    \vskip 3\p@ \hrule}}%
    \def\@oddhead{\hss\vbox to \z@{\vss\hsize=\fullwidth
    \hbox to \fullwidth{{\slshape\rightmark}\hfil\thepage}
    \vskip 3\p@ \hrule}}%
    \let\@mkboth\@gobbletwo
    \let\chaptermark\@gobble
    \let\sectionmark\@gobble
    \topmargin\emptyfoottopmargin
    }
\def\ps@myfootings{%
      \let\@oddhead\@empty\let\@evenhead\@empty
      \def\@evenfoot{\hss\vbox to \z@{\vss\hsize=\fullwidth
      \hrule \vskip 3\p@
      \hbox to \fullwidth{\thepage\hfil\slshape\leftmark}}}%
      \def\@oddfoot{\hss\vbox to \z@{\vss\hsize=\fullwidth
      \hrule \vskip 3\p@
      \hbox to \fullwidth{{\slshape\rightmark}\hfil\thepage}}}%
    \let\@mkboth\@gobbletwo
    \let\chaptermark\@gobble
    \let\sectionmark\@gobble
    \topmargin\emptyheadtopmargin
    }
\if@titlepage
  \newcommand\maketitle{\begin{titlepage}%
  \let\footnotesize\small
  \let\footnoterule\relax
  \null\vfil
  \vskip 60\p@
  \begin{center}%
    {\LARGE \@title \par}%
    \vskip 3em%
    {\large
     \lineskip .75em%
      \begin{tabular}[t]{c}%
        \@author
      \end{tabular}\par}%
      \vskip 1.5em%
    {\large \@date \par}%       % Set date in \large size.
  \end{center}\par
  \@thanks
  \vfil\null
  \end{titlepage}%
  \setcounter{footnote}{0}%
  \let\thanks\relax\let\maketitle\relax
  \gdef\@thanks{}\gdef\@author{}\gdef\@title{}}
\else
  \newcommand\maketitle{\par
    \begingroup
      \renewcommand\thefootnote{\fnsymbol{footnote}}%
      \def\@makefnmark{\hbox to\z@{$\m@th^{\@thefnmark}$\hss}}%
      \long\def\@makefntext##1{\parindent 1em\noindent
            \hbox to1.8em{\hss$\m@th^{\@thefnmark}$}##1}%
      \if@twocolumn
        \ifnum \col@number=\@ne
          \@maketitle
        \else
          \twocolumn[\@maketitle]%
        \fi
      \else
        \newpage
        \global\@topnum\z@   % Prevents figures from going at top of page.
        \@maketitle
      \fi
      \thispagestyle{plain}\@thanks
    \endgroup
    \setcounter{footnote}{0}%
    \let\thanks\relax
    \let\maketitle\relax\let\@maketitle\relax
    \gdef\@thanks{}\gdef\@author{}\gdef\@title{}}
  \def\@maketitle{%
    \newpage
    \null
    %\vskip 2em%
    \longthickrule\vskip1.5em%
    {\secshape \parskip\z@ \parindent\z@
    \Large\bfseries \@title \par}%
    \vskip1.5em\longthickrule\vskip1.5em%
    {\normalsize
      \lineskip .5em%
      \begin{flushright}%
        {\slshape\@author\par}
        \vskip 1em%
        {\@date}%
      \end{flushright}\par}%
    \vskip 1.5em}
\fi
\newcommand\chaptermark[1]{}
\setcounter{secnumdepth}{2}
\newcounter {part}
\newcounter {chapter}
\newcounter {section}[chapter]
\newcounter {subsection}[section]
\newcounter {subsubsection}[subsection]
\newcounter {paragraph}[subsubsection]
\newcounter {subparagraph}[paragraph]
\renewcommand\thepart          {\Roman{part}}
\renewcommand\thechapter       {\arabic{chapter}}
\renewcommand\thesection       {\thechapter.\arabic{section}}
\renewcommand\thesubsection    {\thesection.\arabic{subsection}}
\renewcommand\thesubsubsection {\thesubsection .\arabic{subsubsection}}
\renewcommand\theparagraph     {\thesubsubsection.\arabic{paragraph}}
\renewcommand\thesubparagraph  {\theparagraph.\arabic{subparagraph}}
\newcommand\@chapapp{}
\newcommand\part{\cleardoublepage
                 \thispagestyle{plain}%
                 \if@twocolumn
                   \onecolumn
                   \@tempswatrue
                 \else
                   \@tempswafalse
                 \fi
                 \hbox{}\vfil
                 \secdef\@part\@spart}

\def\@part[#1]#2{%
    \ifnum \c@secnumdepth >-2\relax
      \refstepcounter{part}%
      \addcontentsline{toc}{part}{\thepart\hspace{1em}#1}%
    \else
      \addcontentsline{toc}{part}{#1}%
    \fi
     \longrule\medskip
    {\parindent \z@ \raggedright
     \interlinepenalty \@M
     \reset@font
     \Large
     \ifnum \c@secnumdepth >-2 \relax
     \thepart.\quad
     \fi
     #2\par \medskip
     \longrule\bigskip%
     \markboth{}{}\par}%
    \nobreak
    \vskip 3ex
    \@afterheading}
\def\@spart#1{%
    \longrule\medskip
    {\parindent \z@ \raggedright
     \interlinepenalty \@M
     \reset@font
     \Large #1\par}%
    \medskip\longrule
     \nobreak
     \vskip 3ex
     \@afterheading}
\newcommand\chapter{\if@openright\cleardoublepage\else\clearpage\fi
                           \if@pageperchapter\setcounter{page}{1}\fi
                    \thispagestyle{plain}%
                    \global\@topnum\z@
                    \@afterindentfalse
                    \secdef\@chapter\@schapter}
\def\@chapter[#1]#2{\ifnum \c@secnumdepth >\m@ne
                      \refstepcounter{chapter}%
                      \typeout{\@chapapp\space\thechapter.}%
                      \addcontentsline{toc}{chapter}%
                                   {\protect\numberline{\thechapter}#1}%
                    \else
                      \addcontentsline{toc}{chapter}{#1}
                    \fi
                    \chaptermark{#1}%
                    \addtocontents{lof}{\protect\addvspace{10\p@}}%
                    \addtocontents{lot}{\protect\addvspace{10\p@}}%
                    \if@twocolumn
                      \@topnewpage[\@makechapterhead{#2}]%
                    \else
                      \@makechapterhead{#2}%
                      \@afterheading
                    \fi}
\def\@makechapterhead#1{%
  \longthickrule\bigskip%
  {\parindent \z@ \secshape \reset@font \Large\bfseries
  \ifnum \c@secnumdepth >\m@ne
     \@chapapp{} \thechapter.\quad
  \fi
  #1\par}
  \bigskip\longthickrule\bigskip
  }
\def\@schapter#1{\if@twocolumn
                   \@topnewpage[\@makeschapterhead{#1}]%
                 \else
                   \@makeschapterhead{#1}%
                   \@afterheading
                 \fi}
\def\@makeschapterhead#1{%
    \longthickrule\bigskip%
    {\parindent \z@ \secshape \reset@font
    \Large \bfseries  #1\par}
    \bigskip\longthickrule\bigskip
    }
\newcommand\secshape{\leftskip=-\leftmarginwidth%
                     \rightskip=\@flushglue%
                     \hyphenpenalty=2000}
\newcommand\section{\@startsection {section}{1}{\z@}%
                                   {-2ex \@plus -1ex \@minus -.2ex}%
                                   {0.5ex \@plus .2ex}%
                                   {\secshape\reset@font\large\bfseries}}
\newcommand\subsection{\@startsection{subsection}{2}{\z@}%
                                     {-1.5ex\@plus -.5ex \@minus -.2ex}%
                                     {0.5ex \@plus .2ex}%
                                     {\secshape\reset@font\normalsize\bfseries}}
\newcommand\subsubsection{\@startsection{subsubsection}{3}{\z@}%
                                     {-1.5ex\@plus -.5ex \@minus -.2ex}%
                                     {0.5ex \@plus .2ex}%
                                     {\secshape\reset@font\normalsize\mdseries}}
\newcommand\paragraph{\@startsection{paragraph}{4}{\z@}%
                                    {2ex\@plus 1ex \@minus .2ex}%
                                    {-1em}%
                                    {\reset@font\normalsize\bfseries}}
\newcommand\subparagraph{\@startsection{subparagraph}{5}{\parindent}%
                                       {2ex \@plus 1ex \@minus .2ex}%
                                       {-1em}%
                                       {\reset@font\normalsize\bfseries}}
\if@twocolumn
  \setlength\leftmargini  {2em}
\else
  \setlength\leftmargini  {2.5em}
\fi
\setlength\leftmarginii  {2.2em}
\setlength\leftmarginiii {1.87em}
\setlength\leftmarginiv  {1.7em}
\if@twocolumn
  \setlength\leftmarginv  {.5em}
  \setlength\leftmarginvi {.5em}
\else
  \setlength\leftmarginv  {1em}
  \setlength\leftmarginvi {1em}
\fi
\setlength\leftmargin    {\leftmargini}
\setlength  \labelsep  {.5em}
\setlength  \labelwidth{\leftmargini}
\addtolength\labelwidth{-\labelsep}
\@beginparpenalty -\@lowpenalty
\@endparpenalty   -\@lowpenalty
\@itempenalty     -\@lowpenalty
\def\@listI{\leftmargin\leftmargini
            \parsep \parskip
            \topsep \z@
            \itemsep\z@}
\let\@listi\@listI
\@listi
\def\@listii {\leftmargin\leftmarginii
              \labelwidth\leftmarginii
              \advance\labelwidth-\labelsep
              \topsep    \z@
              \parsep    \parskip
              \itemsep   \z@}
\def\@listiii{\leftmargin\leftmarginiii
              \labelwidth\leftmarginiii
              \advance\labelwidth-\labelsep
              \topsep    \z@
              \parsep    \parskip
              \partopsep \z@
              \itemsep   \topsep}
\def\@listiv {\leftmargin\leftmarginiv
              \labelwidth\leftmarginiv
              \advance\labelwidth-\labelsep}
\def\@listv  {\leftmargin\leftmarginv
              \labelwidth\leftmarginv
              \advance\labelwidth-\labelsep}
\def\@listvi {\leftmargin\leftmarginvi
              \labelwidth\leftmarginvi
              \advance\labelwidth-\labelsep}
\renewcommand\theenumi{\arabic{enumi}}
\renewcommand\theenumii{\alph{enumii}}
\renewcommand\theenumiii{\roman{enumiii}}
\renewcommand\theenumiv{\Alph{enumiv}}
\newcommand\labelenumi{\theenumi.}
\newcommand\labelenumii{(\theenumii)}
\newcommand\labelenumiii{\theenumiii.}
\newcommand\labelenumiv{\theenumiv.}
\renewcommand\p@enumii{\theenumi}
\renewcommand\p@enumiii{\theenumi(\theenumii)}
\renewcommand\p@enumiv{\p@enumiii\theenumiii}
\newcommand\labelitemi{$\m@th\bullet$}
\newcommand\labelitemii{\normalfont\bfseries --}
\newcommand\labelitemiii{$\m@th\ast$}
\newcommand\labelitemiv{$\m@th\cdot$}
\newcommand\descriptionlabel[1]{%
            \ifdescriptionleft\else \hfil\fi
            \normalfont #1 \ifdescriptioncolon :\fi
            \ifdescriptionleft \hfil \fi}
\newenvironment{description}
               {\list{}{%
                    \labelsep\marginparsep
                    \labelwidth\leftmarginwidth
                    %\advance\labelwidth by \@totalleftmargin
                    \advance\labelwidth by \leftmargin
                    \advance\labelwidth by -\labelsep
                    \let\makelabel\descriptionlabel}}
               {\endlist}
\if@titlepage
  \newenvironment{abstract}{%
      \titlepage
      \null\vfil
      \begin{center}
        \bfseries \abstractname
      \end{center}}
     {\par\vfil\null\endtitlepage}
\else
  \newenvironment{abstract}{%
      \if@twocolumn
        \section*{\abstractname}%
      \else
        \small
        \begin{center}%
          {\bfseries \abstractname\vspace{-.5em}\vspace{\z@}}%
        \end{center}%
        \quote
      \fi}
      {\if@twocolumn\else\endquote\fi}
\fi
\newenvironment{verse}
               {\let\\=\@centercr
                \list{}{\itemsep      \z@
                        \itemindent   -1.5em%
                        \listparindent\itemindent
                        \rightmargin  \leftmargin
                        \advance\leftmargin 1.5em}%
                \item[]}
               {\endlist}
\newenvironment{quotation}
               {\list{}{\listparindent 1.5em%
                        \itemindent    \listparindent
                        \rightmargin   \leftmargin
                        \parsep        \z@ \@plus\p@}%
                \item[]}
               {\endlist}
\newenvironment{quote}
               {\list{}{\rightmargin\leftmargin}%
                \item[]}
               {\endlist}
\newenvironment{example}
               {\@beginparpenalty=\highpenalty
               \let\\=\@centercr
                \list{}{\itemsep      \z@
                        \itemindent   -1.5em%
                        \listparindent\itemindent
                        \rightmargin  \leftmargin
                        \advance\leftmargin 1.5em}%
                \ttfamily
                \item[]}
               {\endlist}
\newenvironment{titlepage}
               {\if@twocolumn
                  \@restonecoltrue\onecolumn
                \else
                  \@restonecolfalse\newpage
                \fi
                \thispagestyle{empty}%
                \setcounter{page}{1}}
               {\if@restonecol\twocolumn \else \newpage \fi
               \setcounter{page}{1}}
\newcommand\appendix{\par
  \setcounter{chapter}{0}%
  \setcounter{section}{0}%
  \renewcommand\@chapapp{\appendixname}%
  \renewcommand\thechapter{\Alph{chapter}}}
\setlength\arraycolsep{5\p@}
\setlength\tabcolsep{6\p@}
\setlength\arrayrulewidth{.4\p@}
\setlength\doublerulesep{2\p@}
\setlength\tabbingsep{\labelsep}
\skip\@mpfootins = \skip\footins
\setlength\fboxsep{3\p@}
\setlength\fboxrule{.4\p@}
\@addtoreset{equation}{chapter}
\renewcommand\theequation{\thechapter.\arabic{equation}}
\newcounter{figure}[chapter]
\renewcommand\thefigure{\thechapter.\@arabic\c@figure}
\def\fps@figure{tbp}
\def\ftype@figure{1}
\def\ext@figure{lof}
\def\fnum@figure{\figurename~\thefigure}
\newenvironment{figure}
               {\@float{figure}}
               {\end@float}
\newenvironment{figure*}
               {\@dblfloat{figure}}
               {\end@dblfloat}
\newcounter{table}[chapter]
\renewcommand\thetable{\thechapter.\@arabic\c@table}
\def\fps@table{tbp}
\def\ftype@table{2}
\def\ext@table{lot}
\def\fnum@table{\tablename~\thetable}
\newenvironment{table}
               {\@float{table}}
               {\end@float}
\newenvironment{table*}
               {\@dblfloat{table}}
               {\end@dblfloat}
\newlength\abovecaptionskip
\newlength\belowcaptionskip
\setlength\abovecaptionskip{10\p@}
\setlength\belowcaptionskip{0\p@}
\long\def\@makecaption#1#2{%
  \vskip\abovecaptionskip
  \sbox\@tempboxa{#1: #2}%
  \ifdim \wd\@tempboxa >\hsize
    #1: #2\par
  \else
    \hbox to\hsize{\hfil\box\@tempboxa\hfil}%
  \fi
  \vskip\belowcaptionskip}
\DeclareOldFontCommand{\rm}{\normalfont\rmfamily}{\mathrm}
\DeclareOldFontCommand{\sf}{\normalfont\sffamily}{\mathsf}
\DeclareOldFontCommand{\tt}{\normalfont\ttfamily}{\mathtt}
\DeclareOldFontCommand{\bf}{\normalfont\bfseries}{\mathbf}
\DeclareOldFontCommand{\it}{\normalfont\itshape}{\mathit}
\DeclareOldFontCommand{\sl}{\normalfont\slshape}{\@nomath\sl}
\DeclareOldFontCommand{\sc}{\normalfont\scshape}{\@nomath\sc}
\renewcommand{\cal}{\protect\pcal}
\newcommand{\pcal}{\@fontswitch{\relax}{\mathcal}}
\renewcommand{\mit}{\protect\pmit}
\newcommand{\pmit}{\@fontswitch{\relax}{\mathnormal}}
\newcommand\@pnumwidth{1.55em}
\newcommand\@tocrmarg {2.55em}
\newcommand\@dotsep{4.5}
\setcounter{tocdepth}{2}
\newcommand\tableofcontents{%
    \if@twocolumn
      \@restonecoltrue\onecolumn
    \else
      \@restonecolfalse
    \fi
    \chapter*{\contentsname
        \@mkboth{\contentsname}{\contentsname}}%
    \@starttoc{toc}%
    \if@restonecol\twocolumn\fi
    }
\newcommand\l@part[2]{%
  \ifnum \c@tocdepth >-2\relax
    \addpenalty{-\@highpenalty}%
    \addvspace{2.25em \@plus\p@}%
    \begingroup
      \setlength\@tempdima{3em}%
      \parindent \z@ \rightskip \@pnumwidth
      \parfillskip -\@pnumwidth
      {\leavevmode
       \large \bfseries #1\hfil \hbox to\@pnumwidth{\hss #2}}\par
       \nobreak
         \global\@nobreaktrue
         \everypar{\global\@nobreakfalse\everypar{}}
      \endgroup
  \fi}
\newcommand\l@chapter[2]{%
  \ifnum \c@tocdepth >\m@ne
    \addpenalty{-\@highpenalty}%
    \vskip 1.0em \@plus\p@
    \setlength\@tempdima{1.5em}%
    \begingroup
    \parindent \z@ \rightskip \@pnumwidth
    \parfillskip -\@pnumwidth
    \leavevmode \bfseries
    \advance\leftskip\@tempdima
    \hskip -\leftskip
    #1\nobreak\hfil \nobreak\hbox to\@pnumwidth{\hss #2}\par
    \penalty\@highpenalty
   \endgroup
  \fi}
\newcommand\l@section      {\@dottedtocline{1}{1.5em}{2.3em}}
\newcommand\l@subsection   {\@dottedtocline{2}{3.8em}{3.2em}}
\newcommand\l@subsubsection{\@dottedtocline{3}{7.0em}{4.1em}}
\newcommand\l@paragraph    {\@dottedtocline{4}{10em}{5em}}
\newcommand\l@subparagraph {\@dottedtocline{5}{12em}{6em}}
\newcommand\listoffigures{%
    \if@twocolumn
      \@restonecoltrue\onecolumn
    \else
      \@restonecolfalse
    \fi
    \chapter*{\listfigurename
      \@mkboth{\listfigurename}%
              {listfigurename}}%
    \@starttoc{lof}%
    \if@restonecol\twocolumn\fi
    }
\newcommand\l@figure{\@dottedtocline{1}{1.5em}{2.3em}}
\newcommand\listoftables{%
    \if@twocolumn
      \@restonecoltrue\onecolumn
    \else
      \@restonecolfalse
    \fi
    \chapter*{\listtablename
      \@mkboth{\listtablename}{\listtablename}}%
    \@starttoc{lot}%
    \if@restonecol\twocolumn\fi
    }
\let\l@table\l@figure
\newdimen\bibindent
\bibindent=1.5em
\newcommand\newblock{}
\newenvironment{thebibliography}[1]
     {\chapter*{\bibname
         \@mkboth{\bibname}{\bibname}}%
      \list{\@biblabel{\arabic{enumiv}}}%
   {\settowidth\labelwidth{\@biblabel{#1}}%
            \leftmargin\labelwidth
            \advance\leftmargin\labelsep
            \if@openbib
              \advance\leftmargin\bibindent
              \itemindent -\bibindent
              \listparindent \itemindent
              \parsep \z@
            \fi
            \usecounter{enumiv}%
            \let\p@enumiv\@empty
            \renewcommand\theenumiv{\arabic{enumiv}}}%
      \if@openbib
        \renewcommand\newblock{\par}
      \else
        \renewcommand\newblock{\hskip .11em \@plus.33em \@minus.07em}%
      \fi
      \sloppy\clubpenalty4000\widowpenalty4000%
      \sfcode`\.=\@m}
     {\def\@noitemerr{\@warning{Empty `thebibliography' environment}}%
      \endlist}
\newenvironment{theindex}
               {\if@twocolumn
                  \@restonecolfalse
                \else
                  \@restonecoltrue
                \fi
                \begin{fullpage}
                \let\twocolumn\REF@twocolumn
                \columnseprule \z@
                \columnsep 35\p@
                \twocolumn[\@makeschapterhead{\indexname}]%
                \@mkboth{\indexname}%
                        {\indexname}%
                \thispagestyle{plain}\parindent\z@
                \parskip\z@ \@plus .3\p@\relax
                \let\item\@idxitem}
               {\end{fullpage}\if@restonecol\onecolumn\else\clearpage\fi}
\newcommand\@idxitem  {\par\hangindent 40\p@}
\newcommand\subitem   {\par\hangindent 40\p@ \hspace*{20\p@}}
\newcommand\subsubitem{\par\hangindent 40\p@ \hspace*{30\p@}}
\newcommand\indexspace{\par \vskip 10\p@ \@plus5\p@ \@minus3\p@\relax}
\renewcommand\footnoterule{%
  \kern-3\p@
  \hrule width .4\columnwidth
  \kern 2.6\p@}
\@addtoreset{footnote}{chapter}
\long\def\@makefntext#1{%
    \@setpar{\@@par
            \@tempdima = \hsize
            \advance\@tempdima -1em
            \parshape \@ne 1em \@tempdima}%
    \par
    \parindent 1em \noindent
    \hbox to \z@{\hss\@makefnmark}#1}
\def\@addmarginpar{\@next\@marbox\@currlist{\@cons\@freelist\@marbox
    \@cons\@freelist\@currbox}\@latexbug\@tempcnta\@ne
    \if@twocolumn
     (  \if@firstcolumn \@tempcnta\m@ne \fi
    \else
         \@tempcnta\m@ne
    \fi
    \ifnum\@tempcnta <\z@  \global\setbox\@marbox\box\@currbox \fi
    \@tempdima\@mparbottom
    \advance\@tempdima -\@pageht
    \advance\@tempdima\ht\@marbox
    \ifdim\@tempdima >\z@
      \@@warning{Marginpar on page \thepage\space moved}%
    \else
      \@tempdima\z@
    \fi
    \global\@mparbottom\@pageht
    \global\advance\@mparbottom\@tempdima
    \global\advance\@mparbottom\dp\@marbox
    \global\advance\@mparbottom\marginparpush
    \advance\@tempdima -\ht\@marbox
    \global\setbox \@marbox
                   \vbox {\vskip \@tempdima \box \@marbox}%
    \global \ht\@marbox \z@
    \global \dp\@marbox \z@
    \kern -\@pagedp
    \nointerlineskip
    \hbox to\columnwidth
      {\ifnum \@tempcnta >\z@
          \hskip\columnwidth \hskip\marginparsep
       \else
          \hskip -\marginparsep \hskip -\marginparwidth
       \fi
       \box\@marbox \hss}%
    \nointerlineskip
    \hbox{\vrule \@height\z@ \@width\z@ \@depth\@pagedp}}
\def\marginlabel#1{\mbox{}\marginpar{\raggedleft #1}\ignorespaces}
\def\attention{\mbox{}\marginpar{\raggedleft \large\bfseries ! 
               $\rightarrow$}}
\def\seealso#1{\mbox{}\marginpar{\small $\rightarrow$ #1}\ignorespaces}
\def\longrule{\par\hbox to \linewidth{\hss
              \vrule width \fullwidth height 0.4\p@ depth \z@}\par}
\def\longthickrule{\par\hbox to \linewidth{\hss
              \vrule width \fullwidth height 1.0\p@ depth \z@}\par}
\newenvironment{maxipage}{\par
              \mbox{}\kern-\leftmarginwidth %\kern-\@totalleftmargin
              \begin{minipage}{\fullwidth}
                \medskip \ifmaxipagerule \hrule\medskip \fi
                \parskip = 0.5\baselineskip
                \def\marginpar{%
                \ClassError{Refrep}
                {Marginpar not allowed within Maxipage.}
                {Where should I put them?\MessageBreak
                 I'm using the full pagewidth.}}}
              {\par \vskip\parskip
                 \medskip \ifmaxipagerule \hrule\medskip \fi
               \end{minipage}\par}
\newenvironment{fullpage}{%
              \textwidth=\fullwidth
              \addtolength\oddsidemargin  {-\leftmarginwidth}
              \setlength\evensidemargin{\oddsidemargin}              
              \linewidth=\fullwidth
              \columnwidth=\fullwidth
              \clearpage
              \def\marginpar{%
                \ClassError{Refrep}
                {Marginpar not allowed within Fullpage.}
                {Where should I put them? I'm already\MessageBreak
                 using the whole page for text.}}}
              {\clearpage}
\def\condbreak#1{@vskip \z@ plus #1\pagebreak[3]\vskip \z@ plus -#1\relax}
\def\noparskip{\vskip-\parskip}
\let\REF@twocolumn\twocolumn
\def\twocolumn{%
\ClassError{Refrep}
{Sorry, there is no twocolumn layout in this class}
{Can you imagine how twocolumn layout will look?\MessageBreak
 That's why!}}
\newif\if@pageperchapter \@pageperchapterfalse
\newcommand{\pageperchapter}
    {\@pageperchaptertrue
     \renewcommand\@pnumwidth{2.55em}
     \@openrighttrue
     \renewcommand\thepage{%
       \ifnum \c@chapter = \z@
          \arabic{page}
       \else
          \arabic{chapter} -- \arabic{page}
       \fi
       }
     }

\newcommand\contentsname{Contents}
\newcommand\listfigurename{List of Figures}
\newcommand\listtablename{List of Tables}
\newcommand\bibname{Bibliography}
\newcommand\indexname{Index}
\newcommand\figurename{Figure}
\newcommand\tablename{Table}
\newcommand\partname{Part}
\newcommand\chaptername{Chapter}
\newcommand\appendixname{Appendix}
\newcommand\abstractname{Abstract}
\newcommand\today{\ifcase\month\or
  January\or February\or March\or April\or May\or June\or
  July\or August\or September\or October\or November\or December\fi
  \space\number\day, \number\year}
\setlength\columnsep{10\p@}
\setlength\columnseprule{0\p@}
\pagestyle{plain}
\pagenumbering{arabic}      % Arabic page numbers
\if@twoside
\else
  \raggedbottom
\fi
\if@twocolumn
  \twocolumn
  \sloppy
  \flushbottom
\else
  \onecolumn
\fi
\endinput
%% 
%% End of file `refrep.cls'.
