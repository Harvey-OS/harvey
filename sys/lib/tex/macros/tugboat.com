% version 1.05
% 29 Mar 90
%
%               *****     TUGBOAT.COM   *****
%
%       This file contains macros which are common to both the PLAIN
%       and LaTeX style files for TUGboat.

%       Among other things, it contains supplementary definitions for
%       abbreviations and logos that appear in TUGboat.  See the bottom
%       of the file (after \endinput) for a list of items defined.

% *************************************************************************

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%     *****  helpful shorthand  *****
%

%  The following allow for easier changes of category.  These require that
%  the character be addressed as a control-sequence: e.g. \makeescape\/ will
%  make the / an escape character.

\def\makeescape#1{\catcode`#1=0 }
\def\makebgroup#1{\catcode`#1=1 }
\def\makeegroup#1{\catcode`#1=2 }
\def\makemath#1{\catcode`#1=3 }
\def\makealign#1{\catcode`#1=4 }
\def\makeeol#1{\catcode`#1=5 }
\def\makeparm#1{\catcode`#1=6 }
\def\makesup#1{\catcode`#1=7 }
\def\makesub#1{\catcode`#1=8 }
\def\makeignore#1{\catcode`#1=9 }
\def\makespace#1{\catcode`#1=10 }
\def\makeletter#1{\catcode`#1=11 }
\def\makeother#1{\catcode`#1=12 }
\def\makeactive#1{\catcode`#1=13 }
\def\makecomment#1{\catcode`#1=14 }

\def\makeatletter{\catcode`\@=11 }      % included for historical reasons
\chardef\other=12
\def\makeatother{\catcode`\@=\other}

                                        % alternative to localization
\def\savecat#1{%
  \expandafter\xdef\csname\string#1savedcat\endcsname{\the\catcode`#1}}
\def\restorecat#1{\catcode`#1=\csname\string#1savedcat\endcsname}


\savecat\@
\makeletter\@           % used, as in PLAIN, in protected control sequences

                        % for restoring meanings of global control sequences
\def\SaveCS#1{%
  \def\scratch{\expandafter\let\csname saved@@#1\endcsname}%
  \expandafter\scratch\csname#1\endcsname}
\def\RestoreCS#1{%
  \def\scratch{\expandafter\let\csname#1\endcsname}%
  \expandafter\scratch\csname saved@@#1\endcsname}


% To distinguish between macro files loaded

\def\plaintubstyle{plain}
\def\latextubstyle{latex}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%     *****  abbreviations and logos  *****
%


\def\AMS{American Mathematical Society}

\def\AmSTeX{{\the\textfont2 A}\kern-.1667em\lower.5ex\hbox
        {\the\textfont2 M}\kern-.125em{\the\textfont2 S}-\TeX}

\def\aw{A\kern.1em-W}
\def\AW{Addison\kern.1em-\penalty\z@\hskip\z@skip Wesley}

\def\BibTeX{{\rm B\kern-.05em{\smc i\kern-.025emb}\kern-.08em\TeX}}

\def\CandT{{\sl Computers \& Typesetting}}

\def\DVItoVDU{DVIto\kern-.12em VDU}

%       Japanese TeX
\def\JTeX{\leavevmode\hbox{\lower.5ex\hbox{J}\kern-.18em\TeX}}

\def\JoT{{\sl The Joy of \TeX}}

%       note -- \LaTeX definition is from LATEX.TEX 2.09 of 7 Jan 86,
%               adapted for additional flexibility in TUGboat
%\def\LaTeX{\TestCount=\the\fam \leavevmode L\raise.42ex
%       \hbox{$\fam\TestCount\scriptstyle\kern-.3em A$}\kern-.15em\TeX}
%       note -- broken in two parts, to permit separate use of La,
%               as in (La)TeX
\def\La{\TestCount=\the\fam \leavevmode L\raise.42ex
        \hbox{$\fam\TestCount\scriptstyle\kern-.3em A$}}
\def\LaTeX{\La\kern-.15em\TeX}


%       for Robert McGaffey
\def\Mc{\setbox\TestBox=\hbox{M}M\vbox to\ht\TestBox{\hbox{c}\vfil}}

\font\manual=logo10 % font used for the METAFONT logo, etc.
\def\MF{{\manual META}\-{\manual FONT}}
\def\mf{{\smc Metafont}}
\def\MFB{{\sl The \slMF book}}

%       multilingual (INRS) TeX
\def\mtex{T\kern-.1667em\lower.5ex\hbox{\^E}\kern-.125emX}

\def\pcMF{\leavevmode\raise.5ex\hbox{p\kern-.3ptc}MF}
\def\PCTeX{PC\thinspace\TeX}
\def\pcTeX{\leavevmode\raise.5ex\hbox{p\kern-.3ptc}\TeX}

\def\Pas{Pascal}

\def\PiC{P\kern-.12em\lower.5ex\hbox{I}\kern-.075emC}
\def\PiCTeX{\PiC\kern-.11em\TeX}

\def\plain{{\tt plain}}

\def\POBox{P.\thinspace O.~Box }
\def\POBoxTUG{\POBox\unskip~9506, Providence, RI~02940}

\def\PS{{Post\-Script}}

\def\SC{Steering Committee}

\def\SliTeX{{\rm S\kern-.06em{\smc l\kern-.035emi}\kern-.06em\TeX}}

\def\slMF{\MF}
%       Use \font\manualsl=logosl10 instead, if it's available,
%       for \def\slMF{{\manualsl META}\-{\manualsl FONT}}

%       Atari ST (Klaus Guntermann)
\def\stTeX{{\smc st\rm\kern-0.13em\TeX}}

\def\TANGLE{{\tt TANGLE}}

\def\TB{{\sl The \TeX book}}
\def\TP{{\sl \TeX\/}: {\sl The Program\/}}

\def\TeX{T\hbox{\kern-.1667em\lower.424ex\hbox{E}\kern-.125emX}}

\def\TeXhax{\TeX hax}

%       Don Hosek
\def\TeXMaG{\TeX M\kern-.1667em\lower.5ex\hbox{A}\kern-.2267emG}

%\def\TeXtures{\TestCount=\the\fam
%       \TeX\-\hbox{$\fam\TestCount\scriptstyle TURES$}}
\def\TeXtures{{\it Textures}}

\def\TeXXeT{\TeX--X\kern-.125em\lower.5ex\hbox{E}\kern-.1667emT}

\def\tubfont{\sl}               % redefined in other situations
\def\TUB{{\tubfont TUGboat\/}}

\def\TUG{\TeX\ \UG}

\def\UG{Users Group}

\def\UNIX{{\smc unix}}

\def\VAX{\leavevmode\hbox{V\kern-.12em A\kern-.1em X}}

\def\VorTeX{V\kern-2.7pt\lower.5ex\hbox{O\kern-1.4pt R}\kern-2.6pt\TeX}

\def\XeT{\leavevmode\hbox{X\kern-.125em\lower.424ex\hbox{E}\kern-.1667emT}}

\def\WEB{{\tt WEB}}
\def\WEAVE{{\tt WEAVE}}



%********************************************************************

\newlinechar=`\^^J
\normallineskiplimit=1pt

\clubpenalty=10000
\widowpenalty=10000

\def\NoParIndent{\parindent=\z@}
\newdimen\normalparindent        \normalparindent=20pt          % plain = 20pt
\def\NormalParIndent{\global\parindent=\normalparindent}
\NormalParIndent

\def\BlackBoxes{\overfullrule=5pt }
\def\NoBlackBoxes{\overfullrule=\z@ }
\def\newline{\hskip\z@ plus \pagewd \break}
\def\nohyphens{\hyphenpenalty\@M\exhyphenpenalty\@M}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%     *****  utility registers and definitions  *****
%

%       test registers for transient use; paired - internal/external
\newbox\T@stBox                 \newbox\TestBox
\newcount\T@stCount             \newcount\TestCount
\newdimen\T@stDimen             \newdimen\TestDimen
\newif\ifT@stIf                 \newif\ifTestIf


%       \cs existence test, stolen from TeXbook exercise 7.7
\def\ifundefined#1{\expandafter\ifx\csname#1\endcsname\relax }

%       Smashes repeated from AMS-TeX; PLAIN implements only full \smash .
\newif\iftop@           \newif\ifbot@
\def\topsmash{\top@true\bot@false\smash@}
\def\botsmash{\top@false\bot@true\smash@}
\def\smash{\top@true\bot@true\smash@}
\def\smash@{\relax\ifmmode\def\next{\mathpalette\mathsm@sh}%
        \else\let\next\makesm@sh\fi \next }
\def\finsm@sh{\iftop@\ht\z@\z@\fi\ifbot@\dp\z@\z@\fi\box\z@}


%       Vertical `laps'; cf. \llap and \rlap
\long\def\ulap#1{\vbox to \z@{\vss#1}}
\long\def\dlap#1{\vbox to \z@{#1\vss}}

%       And centered horizontal and vertical `laps'
\def\xlap#1{\hbox to \z@{\hss#1\hss}}
\long\def\ylap#1{\vbox to \z@{\vss#1\vss}}
\long\def\zlap#1{\ylap{\xlap{#1}}}


%       Avoid unwanted vertical glue when making up pages.
\def\basezero{\baselineskip\z@skip \lineskip\z@skip}


%  Empty rules for special occasions
\def\nullhrule{\hrule height\z@ depth\z@ width\z@ }
\def\nullvrule{\vrule height\z@ depth\z@ width\z@ }

%       Support ad-hoc strut construction.
\def\makestrut[#1;#2]{\vrule height#1 depth#2 width\z@ }


%       Today's date, to be printed on drafts.  Based on TeXbook, p.406.

\def\today{\number\day\space \ifcase\month\or
        Jan \or Feb \or Mar \or Apr \or May \or Jun \or
        Jul \or Aug \or Sep \or Oct \or Nov \or Dec \fi
        \number\year}

%       Current time; this may be system dependent!
\newcount\hours
\newcount\minutes
\def\SetTime{\hours=\time
        \global\divide\hours by 60
        \minutes=\hours
        \multiply\minutes by 60
        \advance\minutes by-\time
        \global\multiply\minutes by-1 }
\SetTime
\def\now{\number\hours:\ifnum\minutes<10 0\fi\number\minutes}

\def\Now{\today\ \now}

\newif\ifPrelimDraft            \PrelimDraftfalse

\def\midrtitle{\ifPrelimDraft {{\tensl preliminary draft, \Now}}\fi}

%  Section heads.  The following set of macros is used to set the large
%  TUGboat section heads (e.g. "General Delivery", "Fonts", etc.)

\newdimen\PreTitleDrop   \PreTitleDrop=\z@

\newskip\AboveTitleSkip  \AboveTitleSkip=12pt
\newskip\BelowTitleSkip  \BelowTitleSkip=8pt

\newdimen\strulethickness       \strulethickness=.6pt
\def\sthrule{\hrule height\strulethickness depth \z@ }
\def\stvrule{\vrule width\strulethickness }

\newdimen\stbaselineskip        \stbaselineskip=18pt

\def\@sectitle #1{%
  \par \SecTitletrue
  \penalty-1000
  \secsep
  \vbox{
    \sthrule
    \hbox{%
      \stvrule
      \vbox{
        \advance\hsize by -2\strulethickness
        \raggedcenter
        \def\\{\unskip\break}%
        \sectitlefont
        \makestrut[2\stfontheight;\z@]
        #1%
        \makestrut[\z@;\stfontheight]\endgraf
        }%
      \stvrule }
    \sthrule }
  \nobreak
  \vskip\baselineskip }

%  distance between articles which are run together
\def\secsep{\vskip 5\baselineskip}

\newif\ifSecTitle
\SecTitlefalse





%  Registration marks

\def\HorzR@gisterRule{\vrule height 0.2pt depth \z@ width 0.5in }
\def\DownShortR@gisterRule{\vrule height 0.2pt depth 1pc width 0.2pt }
\def\UpShortR@gisterRule{\vrule height 1pc depth \z@ width 0.2pt }


%               ``T'' marks centered on top and bottom edges of paper

\def\ttopregister{\dlap{%
        \hbox to \trimwd{\HorzR@gisterRule \hfil \HorzR@gisterRule
                        \HorzR@gisterRule \hfil \HorzR@gisterRule}%
        \hbox to \trimwd{\hfil \DownShortR@gisterRule \hfil}}}
\def\tbotregister{\ulap{%
        \hbox to \trimwd{\hfil \UpShortR@gisterRule \hfil}%
        \hbox to \trimwd{\HorzR@gisterRule \hfil \HorzR@gisterRule
                        \HorzR@gisterRule \hfil \HorzR@gisterRule}}}

\def\topregister{\ttopregister}
\def\botregister{\tbotregister}



%       PLAIN's definition of \raggedright doesn't permit any stretch, and
%       results in too many overfull boxes.  We also turn off hyphenation.
\newdimen\raggedskip    \raggedskip=\z@
\newdimen\raggedstretch \raggedstretch=5em    % ems of font set now (10pt)
\newskip\raggedparfill  \raggedparfill=\z@ plus 1fil

\def\raggedspaces{\spaceskip=.3333em \relax \xspaceskip=.5em \relax }
%       Some applications may have to add stretch, in order to avoid
%       all overfull boxes.

\def\raggedright{%
  \nohyphens
  \rightskip=\raggedskip plus\raggedstretch \raggedspaces
  \parfillskip=\raggedparfill }
\def\raggedleft{%
  \nohyphens
  \leftskip=\raggedskip plus\raggedstretch \raggedspaces 
  \parfillskip=\z@skip }
\def\raggedcenter{%
  \nohyphens
  \leftskip=\raggedskip plus\raggedstretch
  \rightskip=\leftskip \raggedspaces 
  \parindent=\z@ \parfillskip=\z@skip }

\def\normalspaces{\spaceskip\z@skip \xspaceskip\z@skip }


%       Miscellaneous useful stuff

\def\,{\relax\ifmmode\mskip\thinmuskip\else\thinspace\fi}

%\def~{\penalty\@M \ } % tie -- this is PLAIN value; it is reset in AMS-TeX
\def~{\unskip\nobreak\ \ignorespaces} % AMS-TeX value

\def\newbox{\alloc@4\box\chardef\insc@unt}   % remove \outer
\def\boxcs#1{\box\csname#1\endcsname}
\def\setboxcs#1{\setbox\csname#1\endcsname}
\def\newboxcs#1{\expandafter\newbox\csname#1\endcsname}

\def\gobble#1{}

\def\vellipsis{%
  \leavevmode\kern0.5em
  \raise1pt\vbox{\baselineskip6pt\vskip7pt\hbox{.}\hbox{.}\hbox{.}}
  }

\def\bull{\vrule height 1ex width .8ex depth -.2ex } % square bullet
\def\cents{{\rm\raise.2ex\rlap{\kern.05em$\scriptstyle/$}c}}
\def\Dag{\raise .6ex\hbox{$\scriptstyle\dagger$}}

\def\careof{\leavevmode\hbox{\raise.75ex\hbox{c}\kern-.15em
                /\kern-.125em\smash{\lower.3ex\hbox{o}}} \ignorespaces}
\def\sfrac#1/#2{\leavevmode\kern.1em
        \raise.5ex\hbox{\the\scriptfont\z@ #1}\kern-.1em
        /\kern-.15em\lower.25ex\hbox{\the\scriptfont\z@ #2}}

\def\d@sh#1{\nobreak\thinspace#1\penalty\z@\thinspace}
\def\dash{\d@sh{--}}
\def\Dash{\d@sh{---}}
\def\lDash{\unskip\ ---\nobreak\thinspace\ignorespaces}
\def\rDash{\unskip\nobreak\thinspace---\ \ignorespaces}

%       Hack to permit automatic hyphenation after an actual hyphen.

\def\hyph{-\penalty\z@\hskip\z@skip }

\def\slash{/\penalty\z@\hskip\z@skip }        % "breakable" slash

%  So far, \nth{n} works only for 0 <= n <= 20.
\def\nth#1{\TestCount=#1
    \ifcase\TestCount \def\next{th}%    0th
    \or   \def\next{st}%                1st
    \or   \def\next{nd}%                2nd
    \or   \def\next{rd}%                3rd
    \else \def\next{th}%                nth
    \fi \TestCount=\the\fam\relax #1$^{\fam\TestCount\next}$}


%       Dates and other items which identify the volume and issue

%       To use: \vol 5, 2.
%               \issdate October 1984.
%               \issueseqno=10
%       For production, these are set in a separate file, TUGBOT.DATES,
%       which is issue-specific.

\newcount\issueseqno            \issueseqno=-1

\def\v@lx{\gdef\volx{Volume~\volno~(\volyr), No.~\issno}}
\def\volyr{}
\def\volno{}
\def\vol #1,#2.{\gdef\volno{#1\unskip}%
        \gdef\issno{\ignorespaces#2\unskip}%
        \setbox\TestBox=\hbox{\volyr}%
        \ifdim \wd\TestBox > .2em \v@lx \fi }

\def\issdate #1#2 #3.{\gdef\issdt{#1#2 #3}\gdef\volyr{#3}%
        \gdef\bigissdt{#1{\smc\uppercase{#2}} #3}%
        \setbox\TestBox=\hbox{\volno}%
        \ifdim \wd\TestBox > .2em \v@lx \fi }


\vol 0, 0.                      % volume, issue.
\issdate Thermidor, 2001.       % month, year of publication


\ifx\tugstyloaded@\plaintubstyle
  \def\tubissue#1(#2){\TUB~#1, no.~#2}
\else
  \def\tubissue#1#2{\TUB~#1, no.~#2}
\fi

\def\xEdNote{{\tenuit Editor's note:\enspace }}


%       TUGboat conventions include the issue number in the file name.
%       Permit this to be incorporated into file names automatically.
%       If issue number = 11, \Input filnam  will read tb11filnam.tex.


\def\infil@{\jobname}
\def\Input #1 {\ifnum\issueseqno<0 \def\infil@{#1}%
                \else \def\infil@{tb\number\issueseqno#1}\fi
                \input \infil@\relax
                \ifRMKopen\immediate\closeout\TBremarkfile\RMKopenfalse\fi}

\newif\ifRMKopen        \RMKopenfalse
\newwrite\TBremarkfile
\def\TBremarkON#1{%
  \ifRMKopen\else\RMKopentrue\immediate\openout\TBremarkfile=\infil@.rmk \fi
  \toks@={#1}%
  \immediate\write\TBremarkfile{^^J\the\toks@}%
  \immediate\write16{^^JTBremark:: \the\toks@^^J}}
\def\TBremarkOFF#1{}
\let\TBremark=\TBremarkOFF


%       Write out (both to a file and to the log) the starting page number
%       of an article, to be used for cross references and in contents.
%       \pagexref  is used for articles fully processed in the TUGboat run.
%       \PageXref  is used for "extra" pages, where an item is submitted
%               as camera copy, and only running heads (at most) are run.

\ifx\tugstyloaded@\plaintubstyle
\def\pagexrefON#1{%
        \write-1{\def\expandafter\noexpand\csname#1\endcsname{\number\pageno}}%
        \write\ppoutfile{%
                \def\expandafter\noexpand\csname#1\endcsname{\number\pageno}}%
        }
\def\PageXrefON#1{%
        \immediate\write-1{\def\expandafter
                        \noexpand\csname#1\endcsname{\number\pageno}}%
        \immediate\write\ppoutfile{\def\expandafter
                        \noexpand\csname#1\endcsname{\number\pageno}}}
\else
\def\pagexrefON#1{%
        \write-1{\def\expandafter\noexpand\csname#1\endcsname{\number\c@page}}%
        \write\ppoutfile{%
                \def\expandafter\noexpand\csname#1\endcsname{\number\c@page}}%
        }
\def\PageXrefON#1{%
        \immediate\write-1{\def\expandafter
                        \noexpand\csname#1\endcsname{\number\c@page}}%
        \immediate\write\ppoutfile{\def\expandafter
                        \noexpand\csname#1\endcsname{\number\c@page}}}
\fi

\def\pagexrefOFF#1{}
\let\pagexref=\pagexrefOFF
\def\PageXrefOFF#1{}
\let\PageXref=\PageXrefOFF

\def\xreftoON#1{%
  \ifundefined{#1}%
    ???\TBremark{Need cross reference for #1.}%
  \else\csname#1\endcsname\fi}
\def\xreftoOFF#1{???}
\let\xrefto=\xreftoOFF

\def\TBdriver#1{}


%  Authors, addresses, signatures

\def\theauthor#1{\csname theauthor#1\endcsname}
\def\theaddress#1{\csname theaddress#1\endcsname}
\def\thenetaddress#1{\csname thenetaddress#1\endcsname}

\newcount\count@@
\def\@defaultauthorlist{%         % standard way of listing authors
   \count@=\authornumber
   \advance\count@ by -2
   \count@@=0
   \loop
   \ifnum\count@>0
      \advance\count@@ by 1
      \ignorespaces\csname theauthor\number\count@@\endcsname\unskip,
      \advance\count@ by -1
   \repeat
   \count@=\authornumber
   \advance\count@ by -\count@@
   \ifnum\authornumber>0
     \ifnum\count@>1
       \count@=\authornumber
       \advance\count@ by -1   
       \ignorespaces\csname theauthor\number\count@\endcsname\unskip\ and
       \fi
     \ignorespaces\csname theauthor\number\authornumber\endcsname\unskip
   \fi
  }

\def\signature#1{\def\@signature{#1}}
\def\@signature{\@defaultsignature}

\def\@defaultsignature{%
  \count@=0
  \loop
  \ifnum\count@<\authornumber
    \medskip
    \advance\count@ by \@ne
    \signaturemark
    \theauthor{\number\count@}\\
    \theaddress{\number\count@}\\
    \thenetaddress{\number\count@}\\
  \repeat
  }

\newdimen\signaturewidth   \signaturewidth=12pc
\def\makesignature{%
  \par
  \rightline{%
    \vbox{\hsize\signaturewidth \ninepoint \raggedright
      \parindent \z@ \everypar={\hangindent 1pc }
      \parskip \z@skip
      \netaddrat
      \netaddrpercent
      \def\|{\unskip\hfil\break}%
      \def\\{\endgraf}%
      \def\net{\tt}%
      \def\phone{\rm Phone: } \rm
      \medskip
      \@signature}}
  }

{\makeactive\@
 \gdef\signatureat{\makeactive\@\def@{\char"40\discretionary{}{}{}}}
 \makeactive\%
 \gdef\signaturepercent{\makeactive\%\def%{\char"25\discretionary{}{}{}}}
}

\def\signaturemark{\leavevmode\llap{$\diamond$\enspace}}




%       some hyphenation exceptions:
\hyphenation{man-u-script man-u-scripts}


\restorecat\@

\endinput


Contents and Notes
------------------

\makeescape, ..., \makecomment allow users to change category
codes a little more easily.

\savecat#1 and \restorecat#1 will save and restore the category
of a given character.  These are useful in cases where one doesn't
wish to localize the settings and therefore be required to globally
define or set things.

\SaveCS#1 and \RestoreCS#1 save and restore `meanings' of control
sequences.  Again this is useful in cases where one doesn't want to
localize or where global definitions clobber a control sequence which
is needed later with its `old' definition.

Abbreviations.  Just a listing with indications of expansion where
that may not be obvious.  For full definitions, see real code above.

\AMS            American Mathematical Society
\AmSTeX
\aw             A-W (abbreviation for Addison-Wesley)
\AW             Addison Wesley
\BibTeX
\CandT          Computers \& Typesetting
\DVItoVDU       DVItoVDU
\JTeX
\JoT            The Joy of \TeX
\LaTeX
\Mc             M ``w/ raised c''
\MF             METAFONT
\mf             Metafont (using small caps)
\MFB            The Metafont book
\mtex           multilingual TeX
\pcMF           pcMF
\PCTeX
\pcTeX
\Pas            Pascal
\PiCTeX
\plain          plain (in typewriter font)
\POBox          P. O. Box
\POBoxTUG       TUG PO Box
\PS             PostScript
\SC             Steering Committee
\SliTeX
\slMF           Metafont (slanted)
\stTeX          TeX for the Atari ST
\TANGLE
\TB             The \TeX book
\TeX
\TeXhax
\TeXMaG
\TeXtures
\TeXXeT
\TUB            TUGboat
\TUG            TeX Users Group
\UNIX
\VAX
\VorTeX
\XeT
\WEB
\WEAVE

\NoBlackBoxes           turns off marginal rules marking overfull boxes
\BlackBoxes             turns them back on
\newline                horizontal glue plus a break

\ifundefined#1          checks argument with \csname against \relax

\topsmash               smashes above baseline  (from AMSTeX)
\botsmash               smashes below baseline  (from AMSTeX)
\smash                  smashes both            (from plain)

\ulap                   lap upwards
\dlap                   lap downwards
\xlap                   reference point at center horizontally; 0 width
\ylap                   reference point at center vertically; 0 height, depth
\zlap                   combination \xlap and \ylap

\basezero               to avoid insertion of baselineskip and lineskip glue

\nullhrule              empty \hrule
\nullvrule              empty \vrule

\makestrut[#1;#2]       ad hoc struts;  #1=height, #2=depth

\today                  today's date
\SetTime                converts \time to hours, minutes
\now                    displays time in hours and minutes
\Now                    shows current date and time

\ifPrelimDraft          flag to indicate status as preliminary draft

\rtitlex                TUGboat volume and number info for running head
\midrtitle              information for center of running head

\HorzR@gisterRule       pieces of registration marks ("trimmarks")
\DownShortR@gisterRule
\UpShortR@gisterRule

\ttopregister           top registration line with `T' in center
\tbotregister           bottom registration line with inverted `T' in center
\topregister            register actually used
\botregister


\raggedskip             parameters used for ragged settings
\raggedstretch
\raggedparfill
\raggedspaces

\raggedright
\raggedleft
\raggedcenter
\normalspaces
\raggedbottom

\bull                   square bullet
\cents                  ``cents'' sign
\Dag                    superscripted dagger
\careof                 c/o
\sfrac                  slashed fraction

\dash                   en-dash surrounded by thinspaces; only breakable AFTER
\Dash                   em-dash, as above

\hyph                   permit automatic hyphenation after an actual hyphen

\slash                  "breakable" slash
\nth                    for obtaining "1^{st}", "2^{nd}", 3^{rd}, etc.

\tubissue               gets \TUB followed by volume and issue numbers

\xEdNote                Editor's Note:

\Input                  \input with some other bookkeepping for
                        case where multiple articles are put together

\TBremark               reminder to TUGboat editorial staff
\TBremarkON
\TBremarkOFF

\pagexref               used to write out page numbers to screen and
\pagexrefON             external files
\pagexrefOFF
\PageXref
\PageXrefON
\PageXrefOFF

\xrefto                 used for symbolic cross-reference to other pages
\xreftoON               in TUGboat
\xreftoOFF

\TBdriver               marks code which only takes effect when articles
                        are run together in a driver file

\signatureat            items for signatures
\signaturepercent
\signaturemark
\signaturewidth

% Change history

v1.05   29 Mar 90
added \lDash and \rDash for `parenthetical' dashing
added \TP for TeX: The Program
added \relax after file input of \Input
added \relax before \ifmmode of \,

v1.04   28 Feb 90
modified pagexref macros to work in both plain and latex styles
(this should affect authors)

v1.03   26 Feb 90
removed <tab>s and adjusted definition of \slMF

v1.02   25 Feb 90
added definitions of \plaintubissue, \latextubissue
added definition of \tubissue

v1.01   19 Feb 90
added \signaturewidth to allow for modification
added \nth to obtain 1^{st}, etc.
