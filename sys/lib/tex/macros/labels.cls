%% 
%% This is file `labels.cls', generated 
%% on <1995/2/23> with the docstrip utility (1).
%% 
%% The original source files were:
%% 
%% labels.dtx  (with options: `class')
%% Copyright (C) 1994 Sebastian Rahtz. All rights reserved. 
%% Permission is granted to to customize the declarations in this file to 
%% serve the needs of your installation. However, no permission is 
%% granted to distribute a modified version of this file under its 
%% original name. 
%% 
\def\fileversion{8}
\def\filedate{1994/04/15}
\def\docdate {1994/01/15}
%% File: labels.dtx Copyright (C) 1994 Sebastian Rahtz
\ProvidesClass{labels}[\filedate\space\fileversion]
\LoadClass{article}
\newdimen\label@width
\newdimen\label@height
\newcount\LabelCols
\newcount\LabelRows
\newdimen\left@border
\newdimen\top@border
\newdimen\half@label
\newdimen\area@width
\newsavebox{\this@label}
\newcount\label@number
\newcount\numberoflabels
\newcount\skip@labels
\newcount\l@so@far
\newif\ifframedlabels
\newif\iffirst@label
\first@labeltrue
\framedlabelsfalse
\LabelCols=3
\LabelRows=8
\left@border=8mm
\top@border=4mm
\textwidth=210mm
\textheight=297mm
\topmargin=-1in
\headheight=0em
\headsep=0em
\topskip=0em
\footskip=0em
\oddsidemargin=-1in
\evensidemargin=-1in
\pagestyle{empty}
\parindent=0em
\parskip=0pt
\def\LabelSetup{
\label@width\textwidth\divide\label@width by\LabelCols
\label@height\textheight\divide\label@height by\LabelRows
\typeout{Creating labels sized \the\label@width\space by \the\label@height}%
\label@number=1
\area@width=\label@width%
\advance\area@width by -\left@border%
\half@label=\label@height\divide\half@label by 2
\advance\half@label by -\top@border
}
\LabelSetup
\numberoflabels=1%
\def\sticky@label{\l@so@far=0%
\loop\ifnum\l@so@far<\numberoflabels\advance\l@so@far by 1\make@label%
\repeat}
\def\make@label{%
\ifframedlabels%
\let\boxing@type\framebox%
\else%
\let\boxing@type\makebox%
\fi%
\boxing@type[\label@width][c]{%
\rule{0pt}{\label@height}% 
\raisebox{\half@label}[0pt][0pt]{%
\rule{\left@border}{0pt}\usebox{\this@label}}}%
\ifnum\label@number=\LabelCols%
\message{|}%
\endgraf\nointerlineskip%
\global\label@number=1\else\global\advance\label@number by 1\fi%
}%
\catcode `\^^M = \active
\def ^^M{\futurelet\nexttoken\isitapar}%
\def\isitapar{\ifx^^M\nexttoken\let\action=\new@label%
\else\let\action\start@newline\fi\action}%
\def\new@label{\message{+}\end@@label\start@@label\@gobble}%
\def\start@newline{\expandafter\newline}%
\def\startingtoken{\ifx^^M\firsttoken\let\action=\@gobble\else%
\let\action=\relax\fi\action}%
\catcode `\^^M = 5\relax%
\def\start@@label{%
\savebox{\this@label}\bgroup\raggedright%
\begin{minipage}{\area@width}%
\catcode `\^^M =\active}%
\def\end@@label{%
\end{minipage}\egroup\sticky@label}%
\def\genericlabel#1{%
\iffirst@label\ifframedlabels%
\advance\label@height by-2\fboxsep%
\advance\label@height by-2\fboxrule%
\half@label\label@height\divide\half@label by 2
\advance\half@label by -\top@border%
\first@labelfalse%
\fi\fi%
\savebox{\this@label}{#1}\sticky@label\@gobblecr}
\def\addresslabel#1{\genericlabel{%
\begin{tabular}{l}#1\end{tabular}}}
\newenvironment{labels}%
{%
\iffirst@label\ifframedlabels%
\advance\area@width by-2\fboxsep%
\advance\area@width by-2\fboxrule%
\advance\label@height by-2\fboxsep%
\advance\label@height by-2\fboxrule%
\half@label\label@height\divide\half@label by 2
\advance\half@label by -\top@border%
\first@labelfalse%
\fi\fi%
\start@@label\futurelet\firsttoken\startingtoken}%
{\end@@label}
\def\labelfile#1{\begin{labels}\input#1\end{labels}}
\def\promptlabels{\typein[\labelfilename]{What is the name of the
label file?}
\labelfile{\labelfilename}}
\newcount\sk@pped
\def\skiplabels#1{\sk@pped=#1%
\savebox{\this@label}{\rule{0pt}{.5in}}%
\skip@labels=0
\loop\ifnum\skip@labels<\sk@pped\advance\skip@labels by 1
\message{x}\make@label
\repeat}
\endinput
%% 
%% End of file `labels.cls'.
