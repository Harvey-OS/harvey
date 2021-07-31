% LaTeX style file for the Maple Technical Newsletter
% Copyright 1994, by Waterloo Maple Software. 
\NeedsTeXFormat{LaTeX2e}
\typeout{}
\typeout{mtn.cls:  A Document Class for the Maple Technical Newsletter}
\typeout{}
\typeout{This style is based on the macros used by Maple's Export to LaTeX.}
\typeout{Maple input and output are specified by:}
\typeout{}
\typeout{Input:}
\typeout{}
\typeout{        \protect\begin{mapleinput}}
\typeout{        x + y;}
\typeout{        \protect\end{mapleinput}}
\typeout{}
\typeout{Output:}
\typeout{}
\typeout{        \protect\begin{maplelatex}}
\typeout{        \protect\[}
\typeout{        x + y}
\typeout{        \protect\]}
\typeout{        \protect\end{maplelatex}}
\typeout{}
\typeout{Plots:}
\typeout{}
\typeout{To include postscript graphics from a file use the macro}
\typeout{}
\typeout{        \protect\mapleplot{filename.ps}}
\typeout{}
\typeout{}
%
\ProvidesClass{mtn}[1994/10/25 v1.0]
%
\edef\PtSize{11pt}   %% default
\DeclareOption{10pt}{
	\edef\PtSize{10pt}
	}
\DeclareOption{11pt}{
	\edef\PtSize{11pt}
	}
\DeclareOption{12pt}{
	\edef\PtSize{12pt}
	}
\DeclareOption*{\PassOptionsToPackage{\CurrentOption}{mapleenv}}
\ExecuteOptions{dvips}
\ProcessOptions
%
\LoadClass[\PtSize,twoside]{article}
\RequirePackage{mapleenv,fancyheadings}
%%
%% DEFINE PAGE LAYOUT 
%% including fancy headers and footers
%% 
\setlength{\marginparwidth}{30pt}
\setlength{\marginparsep}{5pt}
\setlength{\oddsidemargin}{0.5 in}
\setlength{\evensidemargin}{-0.5 in}
\setlength{\textheight}{21.5 cm}
\setlength{\textwidth}{6.25 in}
\setlength{\headheight}{\baselineskip}
\setlength{\footskip}{1.36 cm}
\setlength{\topmargin}{-0.125 in}
%%
%% USE FANCY PAGESTYLE TO ALLOW PROPER HEADERS AND FOOTERS
%% (see man fancyheadings for details)
%%
\pagestyle{fancyplain}
\setlength{\footrulewidth}{0.6pt}
\setlength{\headrulewidth}{0.6pt}
\setlength{\plainheadrulewidth}{0.6pt}
\setlength{\plainfootrulewidth}{0.6pt}
\lfoot[\rm\thepage]{}
\cfoot{}
\rfoot[]{\rm\thepage}
%%
%%	HEADING command
\newcommand{\HEADING}[3]{
\par
\vspace{ 0.5 in} 
\par
{\samepage 
\thispagestyle{plain}
\nopagebreak[4] 
\noindent \hspace{-0.09 em}{\Huge \rm {#1}}\par 
\nopagebreak[4]
\vspace{0.5 ex} 
\nopagebreak[4]
\noindent {\Large \em {#2}}\par 
\nopagebreak[4]
\vspace{3 ex}
\nopagebreak[4]
\hrule
\vspace{3 ex}
\rhead[]{\fancyplain{}{#3}}
\lhead[\fancyplain{}{#3}]{}
}
}
%%
%% heading COMMAND
\newcommand{\heading}[1]{
\par
\vspace{2 ex}
\par\noindent{\Large \bf #1}
\vspace*{1 ex}
\par
}
%%
%% subheading command
\newcommand{\subheading}[1]{
\par
\vspace*{1 ex}
\par\noindent{\sc #1}
\vspace*{1 ex}
\par
}
\raggedbottom
\endinput   % mtn.cls
