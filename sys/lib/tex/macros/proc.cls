%% 
%% This is file `proc.cls', generated 
%% on <1994/12/18> with the docstrip utility (2.2i).
%% 
%% The original source files were:
%% 
%% proc.dtx  (with options: `class')
%% 
%% Copyright 1994 the LaTeX3 project and the individual authors. 
%% All rights reserved. For further copyright information see the file 
%% legal.txt, and any other copyright indicated in this file. 
%% 
%% This file is part of the LaTeX2e system. 
%% ---------------------------------------- 
%% 
%% This system is distributed in the hope that it will be useful, 
%% but WITHOUT ANY WARRANTY; without even the implied warranty of 
%% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
%% 
%% 
%% IMPORTANT NOTICE: 
%% 
%% For error reports in case of UNCHANGED versions see bugs.txt. 
%% 
%% Please do not request updates from us directly. Distribution is 
%% done through Mail-Servers and TeX organizations. 
%% 
%% You are not allowed to change this file. 
%% 
%% You are allowed to distribute this file under the condition that 
%% it is distributed together with all files mentioned in manifest.txt. 
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
\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{proc}
              [1994/05/27 v1.3i
               Standard LaTeX document class]
\DeclareOption{a5paper}
    {\ClassError{proc}{Option `a5paper' not supported}{}}
\DeclareOption{b5paper}
    {\ClassError{proc}{Option `b5paper' not supported}{}}
\DeclareOption{onecolumn}%
    {\ClassError{proc}{Option `onecolumn' not supported}{}}
\DeclareOption{titlepage}%
    {\ClassError{proc}{Option `titlepage' not supported}{}}
\PassOptionsToClass{twocolumn}{article}
\DeclareOption*{\PassOptionsToClass{\CurrentOption}{article}}
\ProcessOptions
\LoadClass{article}
\setlength\footskip{75\p@}
\if@compatibility
  \setlength\textwidth{6.75in}
\else
  \setlength\textwidth{\paperwidth}
  \addtolength\textwidth{-126\p@}
  \@settopoint\textwidth
\fi
\if@compatibility
\else
  \@settopoint\textwidth
\fi
\if@compatibility
  \setlength\textheight{9in}
\else
  \ifcase\@ptsize
    \setlength\textheight{53\baselineskip}
  \or
    \setlength\textheight{47\baselineskip}
  \or
    \setlength\textheight{44\baselineskip}
  \fi
\fi
\advance\textheight by \topskip
\setlength\oddsidemargin{-10\p@}
\setlength\evensidemargin{-10\p@}
\setlength\marginparwidth {\z@}
\setlength\topmargin{-47\p@}
\setlength\columnsep{.375in}
\let\@oddhead\@empty
\let\@evenhead\@empty
\def\@oddfoot{\normalfont\rmfamily\rightmark \hfil
              \pagename{} \thepage}
\def\@evenfoot{\@oddfoot}
\def\maketitle{\par
 \begingroup
   \renewcommand\thefootnote{\fnsymbol{footnote}}%
   \def\@makefnmark{\hbox to\z@{$\m@th^{\@thefnmark}$\hss}}%
   \twocolumn[\@maketitle]%
   \@thanks
 \endgroup
 \setcounter{footnote}{0}%
 \let\maketitle\relax
 \let\@maketitle\relax
 \gdef\@thanks{}\gdef\@author{}\gdef\@title{}\let\thanks\relax}
\def\@maketitle{%
  \vbox to 2.25in{%
    \hsize\textwidth
    \linewidth\hsize
    \vfil
    \centering
    {\LARGE \@title \par}
    \vskip 2em
    {\large \begin{tabular}[t]{c}\@author \end{tabular}\par}
    \vfil}}
\def\copyrightspace{%
  \footnotetext[0]{\mbox{}\vrule \@height 97\p@ \@width \z@}}
\renewenvironment{abstract}%
  {\section*{\abstractname}}
  {\par}
\newcommand\pagename{Page}
\endinput
%% 
%% End of file `proc.cls'.
