%% 
%% This is file `amsdtx.cls', generated 
%% on <1995/1/27> with the docstrip utility (2.2i).
%% 
%% The original source files were:
%% 
%% amsdtx.dtx  (with options: `amsdtx')
%%% ====================================================================
%%%  @LaTeX-file{
%%%     filename        = "amsdtx.dtx",
%%%     version         = "1.2",
%%%     date            = "1995/01/27",
%%%     time            = "12:08:33 EST",
%%%     author          = "American Mathematical Society",
%%%     copyright       = "Copyright (C) 1995 American Mathematical Society,
%%%                        all rights reserved.  Copying of this file is
%%%                        authorized only if either:
%%%                        (1) you make absolutely no changes to your copy,
%%%                        including name; OR
%%%                        (2) if you do make changes, you first rename it
%%%                        to some other name.",
%%%     address         = "American Mathematical Society,
%%%                        Technical Support,
%%%                        Electronic Products and Services,
%%%                        P. O. Box 6248,
%%%                        Providence, RI 02940,
%%%                        USA",
%%%     telephone       = "401-455-4080 or (in the USA and Canada)
%%%                        800-321-4AMS (321-4267)",
%%%     FAX             = "401-331-3842",
%%%     checksum        = "42700 822 2615 28275",
%%%     email           = "tech-support@math.ams.org (Internet)",
%%%     codetable       = "ISO/ASCII",
%%%     keywords        = "latex, amslatex, ams-latex, user documentation",
%%%     supported       = "yes",
%%%     abstract        = "This is part of the AMS-\LaTeX{} distribution.
%%%                        It provides a couple of document classes used
%%%                        to produce user documentation or technical
%%%                        documentation.",
%%%     docstring       = "The checksum field above contains a CRC-16
%%%                        checksum as the first value, followed by the
%%%                        equivalent of the standard UNIX wc (word
%%%                        count) utility output of lines, words, and
%%%                        characters.  This is produced by Robert
%%%                        Solovay's checksum utility.",
%%%  }
%%% ====================================================================
\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{amsdtx}[1995/01/27 v1.2]
\DeclareOption*{\PassOptionsToClass{\CurrentOption}{book}}
\ProcessOptions
\LoadClass{book}
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
\chardef\bslchar=`\\ % p. 424, TeXbook
\newcommand{\addbslash}{\expandafter\@addbslash\string}
\def\@addbslash#1{\bslchar\@nobslash#1}
\newcommand{\nobslash}{\expandafter\@nobslash\string}
\def\@nobslash#1{\ifnum`#1=\bslchar\else#1\fi}
\newcommand{\autoindex}{\index}
\newcommand{\ntt}{\normalfont\ttfamily}
\def\@boxorbreak{\leavevmode
  \ifmmode\hbox\else\ifdim\lastskip=\z@\penalty9999 \fi\fi}
\DeclareRobustCommand{\cs}[1]{\@boxorbreak{\ntt\addbslash#1\@empty
  \autoindex{\nobslash#1@\string\cs{\string#1\@empty}}}}
\DeclareRobustCommand{\cn}[1]{\@boxorbreak{\ntt\addbslash#1\@empty
  \autoindex{\nobslash#1@\string\cn{\string#1\@empty}}}}
\DeclareRobustCommand{\cls}[1]{{\ntt#1}%
  \autoindex{#1@\string\cls{#1} class}}
\DeclareRobustCommand{\pkg}[1]{{\ntt#1}%
  \autoindex{#1@\string\pkg{#1} package}}
\DeclareRobustCommand{\opt}[1]{{\ntt#1}%
  \autoindex{#1@\string\opt{#1} option}}
\DeclareRobustCommand{\env}[1]{{\ntt#1}%
  \autoindex{#1@\string\env{#1} environment}}
\DeclareRobustCommand{\fn}[1]{{\ntt#1}%
   \autoindex{#1@\string\fn{#1}}}
\DeclareRobustCommand{\bst}[1]{{\ntt#1}\autoindex{#1@{\string\ntt{}#1
  bibliography style}}}
\newcommand{\indexcs}[1]{\autoindex{#1@\string\cs{#1}}}
\def\allowtthyphens{\input{OT1cmtt.fd}%
  \expandafter\let\csname OT1+cmtt\endcsname\@empty}
\hfuzz2pc
\vbadness9999 \hbadness5000
\def\AmS{{\protect\usefont{OMS}{cmsy}{m}{n}%
  A\kern-.1667em\lower.5ex\hbox{M}\kern-.125emS}}
\def\latex/{{\protect\LaTeX}}
\def\amslatex/{{\protect\AmS-\protect\LaTeX}}
\def\tex/{{\protect\TeX}}
\def\amstex/{{\protect\AmS-\protect\TeX}}
\def\bibtex/{{Bib\protect\TeX}}
\def\makeindx/{MakeIndex}
\def\xypic/{XY\mbox{-}pic}
\newcommand{\Textures}{\textit{Textures}}
\def\<#1>{\textit{$\langle$#1\/$\rangle$}}
\def\@starttoc#1{\begingroup
  \let\autoindex\@gobble
  \makeatletter
  \@input{\jobname.#1}\if@filesw
  \expandafter\newwrite\csname tf@#1\endcsname
             \immediate\openout
                 \csname tf@#1\endcsname \jobname.#1\relax
  \fi \global\@nobreakfalse \endgroup}
\newcommand{\qc}[1]{}% check for prior definition
\edef\qc#1{\noexpand\protect\expandafter\noexpand\csname qc \endcsname
  \noexpand\protect#1}
\@namedef{qc }#1#2{\begingroup\ntt
  \ifx\ #2\char`\ \else\escapechar\m@ne\string#2\fi\endgroup}
\DeclareRobustCommand{\qcat}{\qc\@}%
\DeclareRobustCommand{\qcamp}{\qc\&}%
\DeclareRobustCommand{\qcbang}{\qc\!}%
\DeclareRobustCommand{\arg}[1]{{\ntt\##1}}
\def\ncn#1{{\let\index\@gobble\cn{#1}}}

\DeclareRobustCommand{\cnbang}{%
  \ncn{\!}\index{"!@{\ntt\bslchar\qcbang}}}
\DeclareRobustCommand{\cnat}{%
  \ncn{\!}\index{"@@{\ntt\bslchar\qcat}}}
\def\5{\penalty500 }
\newcommand{\errexa}{\par\noindent\textit{Example}:\ }
\newcommand{\errexpl}{\par\noindent\textit{Explanation}:\ }
\renewcommand\chapter{\par \@afterindentfalse
                    \secdef\@chapter\@schapter}
\def\@chapter[#1]#2{\ifnum \c@secnumdepth >\m@ne
                       \if@mainmatter
                         \refstepcounter{chapter}%
                         \typeout{\@chapapp\space\thechapter.}%
                         \addcontentsline{toc}{chapter}%
                                   {\protect\numberline{\thechapter}#1}%
                       \else
                         \addcontentsline{toc}{chapter}{#1}\fi
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
  \vspace{1.5\baselineskip}%
  {\parindent \z@ \raggedright \reset@font
    \ifnum \c@secnumdepth >\m@ne
      \large\bfseries \@chapapp\space\thechapter
      \par\nobreak
      \vskip.5\baselineskip\relax
    \fi
    #1\par\nobreak
    \vskip\baselineskip
  }}
\def\@schapter#1{\if@twocolumn
                   \@topnewpage[\@makeschapterhead{#1}]%
                 \else
                   \@makeschapterhead{#1}%
                   \@afterheading
                 \fi}
\def\@makeschapterhead#1{%
  \vspace*{1.5\baselineskip}%
  {\parindent \z@ \raggedright
    \reset@font
    \large \bfseries  #1\par\nobreak
    \vskip\baselineskip
  }}
\renewcommand\maketitle{\par
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
  \vskip 2em%
  \begin{center}%
    {\LARGE \@title \par}%
    \vskip 1.5em%
    {\large
      \lineskip .5em%
      \begin{tabular}[t]{c}%
        \@author
      \end{tabular}\par}%
    \vskip 1em%
    {\large \@date}%
  \end{center}%
  \par
  \uppercase\expandafter{\expandafter\toks@\expandafter{\@title}}%
  \edef\@tempa{\noexpand\markboth{\the\toks@}{\the\toks@}}%
  \@tempa
  \vskip 1.5em}
\renewcommand\section{\@startsection {section}{1}{\z@}%
                                   {-.6\baselineskip \@plus -3\p@}%
                                   {.4\baselineskip}
                                   {\reset@font\normalsize\bfseries}}
\renewcommand\subsection{\@startsection{subsection}{2}{\z@}%
                                     {-.3\baselineskip\@plus -2\p@}%
                                     {.2\baselineskip}%
                                     {\reset@font\normalsize\bfseries}}
\renewcommand\subsubsection{\@startsection{subsubsection}{3}{\z@}%
                                     {-.2\baselineskip\@plus -2\p@}%
                                     {.2\baselineskip}%
                                     {\reset@font\normalsize\bfseries}}
\renewenvironment{theindex}{%
  \if@twocolumn \@restonecolfalse \else \@restonecoltrue \fi
  \columnseprule \z@ \columnsep 35\p@
  \let\autoindex\@gobble
  \twocolumn[\section*{\indexname}]%
  \@mkboth{\uppercase{\indexname}}{\uppercase{\indexname}}%
  \thispagestyle{plain}\parindent\z@
  \parskip\z@ \@plus .3\p@\relax
  \let\item\@idxitem
}{%
  \if@restonecol\onecolumn\else\clearpage\fi
}
\renewcommand{\thesection}{\arabic{section}}
\long\def\@makecaption#1#2{%
  \addvspace\abovecaptionskip
  \begingroup
    \countdef\@parcycles=8 % local count register
    \@parcycles\z@
    \@setpar{\advance\@parcycles\@ne \ifnum\@parcycles>999
      \@@par\@parcycles\z@\fi
      \ifhmode \unskip\hskip\parfillskip\penalty-\@M\fi}%
    \@hangfrom{\textbf{#1.} }\vadjust{\penalty\m@ne}#2%
  \endgroup
  \ifhmode\unpenalty\fi\par
  \ifnum\lastpenalty=\m@ne % only one line in the caption
    \unpenalty \setbox\@tempboxa\lastbox
    \nointerlineskip
    \hbox to\hsize{\hfill\unhbox\@tempboxa\unskip\hfill}%
  \fi
  \nobreak\vskip\belowcaptionskip
}
\renewenvironment{table}{%
  \belowcaptionskip\abovecaptionskip \abovecaptionskip\z@skip
  \@float{table}%
}{%
  \end@float
}
\renewenvironment{table*}{%
  \belowcaptionskip\abovecaptionskip \abovecaptionskip\z@skip
  \@dblfloat{table}%
}{%
  \end@dblfloat
}
\setcounter{topnumber}{4}\setcounter{bottomnumber}{4}
\setcounter{totalnumber}{4}\setcounter{dbltopnumber}{4}
\renewcommand{\topfraction}{.97}\renewcommand{\bottomfraction}{.97}
\renewcommand{\textfraction}{.03}\renewcommand{\floatpagefraction}{.9}
\renewcommand{\dbltopfraction}{.97}
\renewcommand{\dblfloatpagefraction}{.9}
\setlength{\floatsep}{8pt plus6pt}
\setlength{\textfloatsep}{10pt plus8pt}
\setlength{\intextsep}{8pt plus6pt}
\setlength{\dblfloatsep}{8pt plus6pt}
\setlength{\dbltextfloatsep}{10pt plus8pt}
\setlength{\@fptop}{0pt}\setlength{\@fpsep}{8pt}%
\setlength{\@fpbot}{0pt plus 1fil}
\setlength{\@dblfptop}{0pt}\setlength{\@dblfpsep}{8pt}%
\setlength{\@dblfpbot}{0pt plus 1fil}

\pagestyle{headings}
\newcommand{\arrayargpatch}{%
  \let\@oldarray\@array
  \edef\@array[##1]##2{\catcode\number`\|=\number\catcode`\|
    \catcode\number`\@=\number\catcode`\@ \relax
    \let\noexpand\@array\noexpand\@oldarray
    \noexpand\@array[##1]{##2}}%
  \catcode`\|=12 \catcode`\@=12 \relax
}
\newenvironment{ctab}{%
  \par\topsep\medskipamount
  \trivlist\centering
  \item[]%
  \arrayargpatch
  \begin{tabular}%
}{%
  \end{tabular}%
  \endtrivlist
}
\RequirePackage{doc}
\def\AltMacroFont{\MacroFont}
\def\hDocInput#1{\MakePercentIgnore
  \begingroup
  \begingroup \lccode`\~=`\@
  \lowercase{\endgroup\long\def ~}##1##{%
    \catcode`\==12 \skipfileheader{##1}}%
  \catcode`\@=\active \catcode`\==14 % comment
  \def\filename{#1}%
  \@@input#1 \MakePercentComment}
\def\skipfileheader#1#2 {\endgroup
  \hGetFileInfo#2 version = "??" date = "??"\@nil
  \begingroup\catcode`\==9 \catcode`\ =9 \futurelet\0\endgroup
}
\long\def\hGetFileInfo#1 version = "#2"#3 date = "#4"#5\@nil{%
  \def\fileversion{#2}\def\filedate{#4}}
\endinput
%% 
%% End of file `amsdtx.cls'.
