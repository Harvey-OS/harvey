%%% ====================================================================
%%%  @BibTeX-style-file{
%%%     author          = "Werenfried Spit",
%%%     version         = "2.5.1",
%%%     date            = "2 November 1993",
%%%     time            = "12:29:12 CET",
%%%     filename        = "nederlands.bst",
%%%     address         = "Departament de Fisica Teorica
%%%                        Universitat de Valencia
%%%                        c/ Moliner, 50
%%%                        E-46100 Burjassot, Valencia
%%%                        Espanya",
%%%     telephone       = "+34-6-386 4551",
%%%     email           = "spit@vm.ci.uv.es
%%%                        spit@ific.uv.es",
%%%     codetable       = "ISO/ASCII",
%%%     keywords        = "BibTeX, Dutch",
%%%     supported       = "yes",
%%%     checksum        = "19068 1618 4873 37905",
%%%     abstract        = "In Dutch (and German, ...) names are sorted on their
%%%                        last part; prepositions (the `von-part') are not
%%%                        considered to be part of the last name for sorting
%%%                        purposes. Thus, J. van Dale will be listed under D.
%%%
%%%                        This style is part of the Harvard package that
%%%                        supports citations like `see Van Dale (1983)' in the
%%%                        text; harvard.sty is needed. Sorting is done
%%%                        according to dutch rules. The layout of the
%%%                        bibliography is adapted to conventions is historical
%%%                        linguistics. As of version 2.3 books may have both
%%%                        an author and an editor.
%%%
%%%                        More information in dutch in the file. ",
%%%     docstring       = "The checksum field above contains a CRC-16
%%%                        checksum as the first value, followed by the
%%%                        equivalent of the standard UNIX wc (word
%%%                        count) utility output of lines, words, and
%%%                        characters.  This is produced by Robert
%%%                        Solovay's checksum utility.",
%%%  }
%%% ====================================================================
 FUNCTION {hello}
{ "BibTeX bibliography style `Nederlands' " top$
  "version <2.5>   dated <1-6-93>   (WS)" top$
}
% (C) Werenfried Spit    04-10-90
%
%     Departament de F{\'\i}sica Te\`orica
%     Universitat de Val\`encia
%     c/ dr.Moliner, 50
%     46100 Burjassot
%     Val\`encia
%     Espanya
%
%     spit@vm.ci.uv.es
%
%
%     Met gebruikmaking van veel werk van Piet v Oostrum en Oren Patashnik
%     Met dank aan Paul Wackers en Peter Williams.
%     Voor BibTeX 0.99a en LaTeX 2.09
%
%
% In deze bibliografiestijl wordt ondersteuning gegeven om in de
% tekst citaties met de auteursnaam en het jaartal te laten verschijnen
% bv zie (Van Dale, 1982). Naar Nederlandse gewoonte gebeurt het
% sorteren op achternaam, hoewel het tussenvoegsel eerder geschreven
% wordt. (Van Dale staat dus onder de D.) De ietwat ingewikkelde regel
% uit de nederlandse grammatica voor het gebruik van hoofdletters in
% tussenvoegsels van namen wordt toegepast. (Zie hiervoor het Groene
% Boekje of de ANS, of natuurlijk Van Dale, 1982).
%
% In tegenstelling tot de standaard bibstijlen laat deze stijl toe dat
% een boek zowel een auteur als een redacteur heeft. In zulke gevallen
% wordt op redacteur gesorteerd. Er komt dan bv onder de H in de bibliografie
%  W. Shakespeare, 1991. {\em Macbeth}, bezorgd door W. Hogan. ....
% en in de tekst zal `Hogan (1991)' voorkomen.
%
% Om deze bibliografiestijl te gebruiken moet de LaTeXstijl harvard.sty
% worden gebruikt. Deze definieert \harvarditem en \citeasnoun en
% herdefinieert \cite. Zie verder de bijbehorende documentatie
%
% Van deze stijl is ook versie 1.11 nog beschikbaar: deze werkt met
% citeauthoryear.sty ipv harvard.sty.
%
%
%
% wijzigingen:
% 02-11-93 fout in lidwoordafbreking verbeterd (dank aan Frank Poppe)
% 01-06-93 kruisreferenties veranderd
% 15-12-92 vermelding jaartal bij tijdschriftartikelen verdubbeld
% 14-12-92 vermelding jaartal bij tijdschriftartikelen verplaatst
%          enkele niet gebruikte routines verwijderd
% 22-10-92 mogelijkheid om bij boeken zowel een `author' als een `editor'
%          te vermelden toegevoegd
%          vermelding uitgeversadres (`address') veranderd
%
% 18-05-92 foutje in de sorteerroutine verbeterd  (dank aan Paul Wackers)
% 22-04-92 foutje in de behandeling van seriewerken verbeterd
%          behandeling van het `editor'-veld vernederlandst
%          (dank aan Ton vd Woude)
% 26-03-92 verschillende foutmeldingen verwijderd
% 20-03-92 mogelijk voorkomen van te lange outputregels voorkomen
% 17-03-92 verbetering van de verwerking van entries zonder year-veld
% 16-03-92 behandeling van labels veranderd om te lange strings te voorkomen
%          jaartal voor alle soorten entries direct na de auteur geplaatst
% 12-03-92 label aan jaartal toegevoegd
%          aanpassingen tbv Harvardstijlen
% 14-11-91 regels langer dan 80 tekens verwijderd (ivm met IBMse nukken)
% 27-05-91 karaktertabel toegevoegd
% 14-05-91 behandeling van tussenvoegsels in namen verbeterd
%
%%   Upper-case    \A\B\C\D\E\F\G\H\I\J\K\L\M\N\O\P\Q\R\S\T\U\V\W\X\Y\Z
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
%%   Right brace   \}     Tilde         \~
%%
%
%   Citation format: [author-last-name, year]
%             [author-last-name \& author-last-name, year]
%             [author-last-name {\em et~al.}, year]
%
%   Reference list ordering: alphabetical by author or whatever passes
%   for author in the absence of one.


ENTRY
  { address
    author
    booktitle
    chapter
    edition
    editor
    howpublished
    institution
    journal
    key
    month
    note
    number
    organization
    pages
    publisher
    school
    series
    title
    type
    volume
    year
    font
  }
  { title.used }
  { label abbr.label full.label extra.label sort.label list.label year.string }

INTEGERS { output.state before.all mid.sentence after.sentence after.block }

FUNCTION {init.state.consts}
{ #0 'before.all :=
  #1 'mid.sentence :=
  #2 'after.sentence :=
  #3 'after.block :=
}

STRINGS { s t }

FUNCTION {output.nonnull}
{ 's :=
  output.state mid.sentence =
    { ", " * write$ }
    { output.state after.block =
    { add.period$ write$
      newline$
      "\newblock " write$
    }
    { output.state before.all =
        'write$
        { add.period$ " " * write$ }
      if$
    }
      if$
      mid.sentence 'output.state :=
    }
  if$
  s
}

FUNCTION {output}
{ duplicate$ empty$
    'pop$
    'output.nonnull
  if$
}

FUNCTION {output.check}
{ 't :=
  duplicate$ empty$
    { pop$ "empty " t * " in " * cite$ * warning$ }
    'output.nonnull
  if$
}

FUNCTION {One.cap}  %(WS) 14-05-91 capitalizes first letter of string
                    %              leaves the rest the same
{ duplicate$
  #1 #1   substring$ "\uppercase{" swap$ * "}" * swap$
  #2 #200 substring$ *
}

FUNCTION {output.bibitem}         % (WS 12-03-92) \bibitem replaced by
{ newline$                        % \harvarditem;
  "\harvarditem[" write$          % (WS 22 10 92) label doubly written
  label write$                    % to prevent too long strings
  "]%" write$ newline$            % (WS 20 03 92) newlines inserted
  "{"  write$
  label write$
  "}%" write$ newline$
  "{"  write$
  year.string write$
  "}{" write$
  cite$ write$
  "}" write$
  newline$
  ""
  before.all 'output.state :=
}

FUNCTION {fin.entry}
{ add.period$
  write$
  newline$
}

FUNCTION {new.block}
{ output.state before.all =
    'skip$
    { after.block 'output.state := }
  if$
}

FUNCTION {new.sentence}
{ output.state after.block =
    'skip$
    { output.state before.all =
    'skip$
    { after.sentence 'output.state := }
      if$
    }
  if$
}

FUNCTION {not}
{   { #0 }
    { #1 }
  if$
}

FUNCTION {and}
{   'skip$
    { pop$ #0 }
  if$
}

FUNCTION {or}
{   { pop$ #1 }
    'skip$
  if$
}

FUNCTION {new.block.checka}
{ empty$
    'skip$
    'new.block
  if$
}

FUNCTION {new.block.checkb}
{ empty$
  swap$ empty$
  and
    'skip$
    'new.block
  if$
}

FUNCTION {new.sentence.checka}
{ empty$
    'skip$
    'new.sentence
  if$
}

FUNCTION {new.sentence.checkb}
{ empty$
  swap$ empty$
  and
    'skip$
    'new.sentence
  if$
}

FUNCTION {field.or.null}
{ duplicate$ empty$
    { pop$ "" }
    'skip$
  if$
}

FUNCTION {emphasize}
{ duplicate$ empty$
    { pop$ "" }
    { "{\em " swap$ * "}" * }
  if$
}

INTEGERS { nameptr namesleft numnames }

FUNCTION {format.names}
{ 's :=
  #1 'nameptr :=
  s num.names$ 'numnames :=
  numnames 'namesleft :=
    { namesleft #0 > }

    { s nameptr "{ff~}{vv~}{ll}{, jj}" format.name$ 't :=

      nameptr #1 >                  % in de referentie wordt de volledige naam
    { namesleft #1 >                % afgedrukt: "J.H. van Dale"  (WS)
        { ", " * t * }
        { numnames #2 >
         { "," * }
         'skip$
          if$
          t "others" =
         { " {\em ~et~al.}" * }
         { " en " * t * }           % hier wordt een tussenvoegsel
          if$                       % tussen auteursnamen gemaakt   (WS)
        }
      if$
    }
    't
      if$
      nameptr #1 + 'nameptr :=
      namesleft #1 - 'namesleft :=
    }
  while$
  font empty$
  {   "{" swap$ * "}" * }
  {   "{" font * " " * swap$ * "}" * }
  if$
}

FUNCTION {format.authors}
{ author empty$
    { "" }
    { author format.names }
  if$
}


% Een editor heet in het Nedelands redacteur, maar het is gebruikelijk
% dit aan te geven met ``redactie''. (230492).

FUNCTION {format.editors}
{ editor empty$
    { "" }
    {  editor format.names
       ", redactie" *
    }
  if$
}

% Sommige boeken hebben behalve een auteur ook een redacteur; m.n.
% wanneer de tekst van een ouder werk opnieuw wordt uitgegeven. Voor
% dit soort gevallen is format.editortoo:
%
FUNCTION {format.editorstoo}
{ editor empty$
    { "" }
    {  " bezorgd door " editor format.names *
    }
  if$
}



FUNCTION {format.title}
{ title empty$
    { "" }

    { title }

  if$
}

FUNCTION {n.dashify}
{ 't :=
  ""
    { t empty$ not }
    { t #1 #1 substring$ "-" =
    { t #1 #2 substring$ "--" = not
        { "--" *
          t #2 global.max$ substring$ 't :=
        }
        {   { t #1 #1 substring$ "-" = }
         { "-" *
           t #2 global.max$ substring$ 't :=
         }
          while$
        }
      if$
    }
    { t #1 #1 substring$ *
      t #2 global.max$ substring$ 't :=
    }
      if$
    }
  while$
}

FUNCTION {format.date}
{ year empty$
    { month empty$
    { "" }
    { "there's a month but no year in " cite$ * warning$
      month
    }
      if$
    }
    { month empty$
    { year.string }       % (WS 16 03 92) year.string ipv year extra.label *
    { month " " * year.string * }
      if$
    }
  if$
}

FUNCTION {format.btitle}
{ title emphasize
}

FUNCTION {tie.or.space.connect}
{ duplicate$ text.length$ #3 <
    { "~" }
    { " " }
  if$
  swap$ * *
}

FUNCTION {either.or.check}
{ empty$
    'pop$
    { "can't use both " swap$ * " fields in " * cite$ * warning$ }
  if$
}

FUNCTION {format.bvolume}                            % in het Nederlands (WS)
{ volume empty$
    { "" }
    { series empty$
      { " deel"  }                                  % zonder komma (WS;220492)
      { " "  series emphasize * ", deel" * }
      if$
      volume tie.or.space.connect
    }
  if$
}

FUNCTION {format.number.series}                      % in het Nederlands (WS)
{ volume empty$
    { number empty$
        { series field.or.null }
        { series empty$
            { "there's a number but no series in " cite$ * warning$ }
            { series number tie.or.space.connect }
          if$
        }
      if$
    }
    { "" }
  if$
}

FUNCTION {format.edition}                             % in het Nederlands (WS)
{ edition empty$
    { "" }
    { edition "1" =
        { "Eerste" }
        { edition "2" =
            { "Tweede" }
            { edition "3" =
                { "Derde" }
                { edition "$^{\rm e}$ " * }
                if$
            }
            if$
        }
        if$
        output.state mid.sentence =
            { "l" change.case$ " druk" * }
            { "t" change.case$ " druk" * }
            if$
    }
    if$
}

INTEGERS { multiresult }

FUNCTION {multi.page.check}
{ 't :=
  #0 'multiresult :=
    { multiresult not
      t empty$ not
      and
    }
    { t #1 #1 substring$
      duplicate$ "-" =
      swap$ duplicate$ "," =
      swap$ "+" =
      or or
    { #1 'multiresult := }
    { t #2 global.max$ substring$ 't := }
      if$
    }
  while$
  multiresult
}

FUNCTION {format.pages}
{ pages empty$
    { "" }
    { pages multi.page.check
    { "p." pages n.dashify tie.or.space.connect }
    { "p." pages tie.or.space.connect }
      if$
    }
  if$
}


FUNCTION {format.vol.date.pages}         % (14-12-1992) was: format.vol.pages
{   volume empty$
       { "empty volume in " cite$ * warning$ " " }
       {  volume  }
    if$
    number empty$
       'skip$
      { "-" * number * }
    if$
    year empty$
       { "empty year in " cite$ * warning$ " " }
%%       { " (" * format.date * ")" * }           % (15-12-1992) jaartal zonder
       { " (" * year * ")" * }                    %   volgletter
    if$
    pages empty$
       'skip$
      { duplicate$ empty$
        { pop$ format.pages }
        { ", " * pages n.dashify * }
      if$
         }
   if$
}

FUNCTION {format.vol.pages}
{   volume empty$
       { "empty volume in " cite$ * warning$ " " }
       {  volume  }
    if$
    number empty$
       'skip$
      { "-" * number * }
    if$
    pages empty$
       'skip$
      { duplicate$ empty$
        { pop$ format.pages }
        { ", " * pages n.dashify * }
      if$
         }
   if$
}




FUNCTION {format.chapter.pages}                       % in het Nederlands (WS)
{ chapter empty$
    'format.pages
    { type empty$
        { output.state mid.sentence =
          { "hoofdstuk" }
          { "Hoofdstuk" }
          if$
         }
        { type "l" change.case$ }
      if$
      chapter tie.or.space.connect
      pages empty$
        'skip$
        { ", " * format.pages * }
      if$
    }
  if$
}

FUNCTION {format.in.ed.booktitle}                     % in het Nederlands (WS)
{ booktitle empty$                                    %
    { "" }
    { editor empty$
      { output.state mid.sentence =
        { "in " booktitle emphasize * }
        { "In " booktitle emphasize * }
        if$ }
      { output.state mid.sentence =
        { "in " booktitle emphasize * ", " * format.editors * }
        { "In " booktitle emphasize * ", " * format.editors * }
        if$ }
      if$
    }
  if$
}

FUNCTION {empty.misc.check}
{ author empty$ title empty$ howpublished empty$
  month empty$ year empty$ note empty$
  and and and and and

  key empty$ not and

    { "all relevant fields are empty in " cite$ * warning$ }
    'skip$
  if$
}

FUNCTION {format.thesis.type}
{ type empty$
    'skip$
    { pop$
      type "t" change.case$
    }
  if$
}

FUNCTION {format.preprint.number}
{ type empty$
    { "Preprint" }
    'type
  if$
  number empty$
    { "t" change.case$ }
    { number tie.or.space.connect }
  if$
}

FUNCTION {format.tr.number}
{ type empty$
    { "Technical Report" }
    'type
  if$
  number empty$
    { "t" change.case$ }
    { number tie.or.space.connect }
  if$
}


FUNCTION {format.article.crossref}
{ key empty$
    { journal empty$
    { "need key or journal for " cite$ * " to crossref " * crossref *
      warning$
      ""
    }
    { "In {\em " journal * "\/}" * }
      if$
    }
    { "In " key * }
  if$
  " \cite{" * crossref * "}" *
}

FUNCTION {format.crossref.editor}
{ editor #1 "{vv~}{ll}" format.name$ One.cap
  editor num.names$ duplicate$
  #2 >
    { pop$ "{\em ~et~al.}" * }
    { #2 <
    'skip$
    { editor #2 "{ff }{vv }{ll}{ jj}" format.name$ "others" =
        { " {\em ~et~al.}" * }
        { " en " * editor #2 "{vv~}{ll}" format.name$ One.cap * }
      if$
    }
      if$
    }
  if$
}


% v2.5 (1-6-93) crossreferencing changed: now uses \citeasnoun in case
%      there is an editor (which is supposed to be the author/editor
%      of the crossreferenced entry)
FUNCTION {format.book.crossref}
{ volume empty$
    { "empty volume in " cite$ * "'s crossref of " * crossref * warning$
      "In "
    }
    { "Volume" volume tie.or.space.connect
      " of " *
    }
  if$
  editor empty$
  editor field.or.null author field.or.null =
  or
    { key empty$
    { series empty$
        { "need editor, key, or series for " cite$ * " to crossref " *
          crossref * warning$
          "" *
        }
        { "{\em " * series * "\/}" * }
      if$
    }
    { key * }
      if$
     " \cite{" * crossref * "}" *
    }
    { " \citeasnoun{" * crossref * "}" * }
  if$
}

FUNCTION {institution.or.organization.output}
{institution empty$
 {organization empty$
  { "Both organization and institution are missing in " cite$ * warning$ }
  { organization output }
  if$
 }
 {institution output}
if$
}

% v2.5 (1-6-93) crossreferencing changed: uses \citeasnoun when possible
FUNCTION {format.incoll.inproc.crossref}
{ editor empty$
  editor field.or.null author field.or.null =
  or
    { key empty$
    { booktitle empty$
        { "need editor, key, or booktitle for " cite$ * " to crossref " *
          crossref * warning$
          ""
        }
        { "In {\em " booktitle * "\/}" * }
      if$
    }
    { "In " key * }
      if$
    " \cite{" * crossref * "}" *
    }
    { "In \citeasnoun{" crossref * "}" * }
  if$

}

FUNCTION {article}
{ output.bibitem
  format.authors "author" output.check
  format.date    "year"   output.check
  new.block
  format.title "title" output.check
  new.block
  crossref missing$
    { journal emphasize "journal" output.check
      format.vol.date.pages output        % (14-12-1992) was: format.vol.pages
    }
    { format.article.crossref output.nonnull
      format.pages output
    }
  if$
  new.block
  note output
  fin.entry
}

FUNCTION {book}
{ output.bibitem
  author empty$
    { format.editors "author and editor" output.check }
    { format.authors output.nonnull
%      crossref missing$
%    { "author and editor" editor either.or.check }
%      'skip$
%     if$
    }
  if$
  format.date "year" output.check
  new.block
  format.btitle "title" output.check
  author empty$
  { skip$ }
  { editor empty$
    'skip$
     { format.editorstoo "editor" output.check }
     if$
   }
  if$
  crossref missing$
    { format.bvolume output
      new.block
      format.number.series output
      new.sentence
      address output
      publisher "publisher" output.check
    }
    { new.block
      format.book.crossref output.nonnull
    }
  if$
  format.edition output
  new.block
  note output
  fin.entry
}

FUNCTION {booklet}
{ output.bibitem
  format.authors output
  format.date output                     % no check on year (WS 26-03-92)
  new.block
  format.title "title" output.check
  howpublished address new.block.checkb
  howpublished output
  address output
  new.block
  note output
  fin.entry
}

FUNCTION {inbook}                            % produceert zowel hoofdstuk-
{ output.bibitem                             % als boektitel  (WS)
  author empty$                              % zie ook format.in.ed.booktitle
    { format.editors "author and editor" output.check }
    { format.authors output.nonnull
      crossref missing$
        { "author and editor" editor either.or.check }
        'skip$
      if$
    }
  if$
  format.date "year" output.check
  new.block
  format.btitle "title" output.check
  crossref missing$
    { format.bvolume output
      format.chapter.pages "chapter and pages" output.check
      format.in.ed.booktitle output
      new.block
      format.number.series output
      new.sentence
      publisher "publisher" output.check
      address output
    }
    { format.chapter.pages "chapter and pages" output.check
      new.block
      format.book.crossref output.nonnull
    }
  if$
  format.edition output
  new.block
  note output
  fin.entry
}

FUNCTION {incollection}
{ output.bibitem
  format.authors "author" output.check
  format.date "year" output.check
  new.block
  format.title "title" output.check
  new.block
  crossref missing$
    { format.in.ed.booktitle "booktitle" output.check
      format.bvolume output
      format.number.series output
      format.chapter.pages output
      new.sentence
      publisher "publisher" output.check
      address output
      format.edition output
    }
    { format.incoll.inproc.crossref output.nonnull
      format.chapter.pages output
    }
  if$
  new.block
  note output
  fin.entry
}

FUNCTION {inproceedings}
{ output.bibitem
  format.authors "author" output.check
  format.date "year" output.check
  new.block
  format.title "title" output.check
  new.block
  crossref missing$
    { format.in.ed.booktitle "booktitle" output.check
      format.bvolume output
      format.number.series output
      format.pages output
      address empty$
    { organization publisher new.sentence.checkb
      organization output
      publisher output
    }
    { address output.nonnull
      new.sentence
      organization output
      publisher output
    }
      if$
    }
    { format.incoll.inproc.crossref output.nonnull
      format.pages output
    }
  if$
  new.block
  note output
  fin.entry
}

FUNCTION {conference} { inproceedings }

FUNCTION {manual}
{ output.bibitem
  author empty$
    { organization empty$
    'skip$
    { organization output.nonnull
      address output
    }
      if$
    }
    { format.authors output.nonnull }
  if$
  format.date output                     % no check on year (WS 26-03-92)
  new.block
  format.btitle "title" output.check
  author empty$
    { organization empty$
    { address new.block.checka
      address output
    }
    'skip$
      if$
    }
    { organization address new.block.checkb
      organization output
      address output
    }
  if$
  format.edition output
  new.block
  note output
  fin.entry
}

FUNCTION {mastersthesis}                % doctoraalscriptie (WS)
{ output.bibitem
  format.authors "author" output.check
  format.date "year" output.check
  new.block
  format.btitle "title" output.check
  new.block
  "Doctoraalscriptie" format.thesis.type output.nonnull
  school "school" output.check
  address output
  number empty$
         { skip$ }
         { " (Nr. " number * ")" * output }
         if$
  new.block
  note output
  fin.entry
}


FUNCTION {phdthesis}                                     % proefschrift (WS)
{ output.bibitem
  format.authors "author" output.check
  format.date "year" output.check
  new.block
  format.btitle "title" output.check
  new.block
  "Dissertatie" format.thesis.type output.nonnull
  school "school" output.check
  address output
  new.block
  note output
  fin.entry
}

FUNCTION {preprint}
{ output.bibitem
  format.authors "author" output.check
  format.date "year" output.check
  new.block
  format.title "title" output.check
  new.block
  format.preprint.number output.nonnull
  institution.or.organization.output
  publisher output
  address output
  new.block
  note output
  fin.entry
}

FUNCTION {proceedings}
{ output.bibitem
  editor empty$
    { organization output }
    { format.editors output.nonnull }
  if$
  new.block
  format.btitle "title" output.check
  format.bvolume output
  format.number.series output
  address empty$
    { editor empty$
    { publisher new.sentence.checka }
    { organization publisher new.sentence.checkb
      organization output
    }
      if$
      publisher output
      format.date "year" output.check
    }
    { address output.nonnull
      format.date "year" output.check
      new.sentence
      editor empty$
    'skip$
    { organization output }
      if$
      publisher output
    }
  if$
  new.block
  note output
  fin.entry
}

FUNCTION {techreport}
{ output.bibitem
  format.authors "author" output.check
  format.date "year" output.check
  new.block
  format.title "title" output.check
  new.block
  format.tr.number output.nonnull
  institution "institution" output.check
  address output
  new.block
  note output
  fin.entry
}

FUNCTION {unpublished}
{ output.bibitem
  format.authors "author" output.check
  format.date output                     % no check on year (WS 26-03-92)
  new.block
  format.title "title" output.check
  new.block
  note "note" output.check
  fin.entry
}

FUNCTION {misc}
{ output.bibitem
  format.authors output
  format.date output                     % no check on year (WS 26-03-92)
  title howpublished new.block.checkb
  format.title output
  howpublished new.block.checka
  howpublished output
  new.block
  note output
  fin.entry
  empty.misc.check
}

FUNCTION {default.type} { misc }

%
MACRO {tbp} {"verschijnt"}

% De namen van de maanden in het Nederlands
%
MACRO {jan} {"januari"}
MACRO {feb} {"februari"}
MACRO {mar} {"maart"}
MACRO {apr} {"april"}
MACRO {may} {"mei"}
MACRO {jun} {"juni"}
MACRO {jul} {"juli"}
MACRO {aug} {"augustus"}
MACRO {sep} {"september"}
MACRO {oct} {"oktober"}
MACRO {nov} {"november"}
MACRO {dec} {"december"}

% De Nederlanse universiteiten
%
MACRO {KUN} {"Katholieke Universiteit Nijmegen"}
MACRO {KUB} {"Katholieke Universiteit Brabant"}
MACRO {LUW} {"Landbouwuniversiteit Wageningen"}
MACRO {RL}  {"Rijksuniversiteit Limburg"}
MACRO {RUG} {"Rijksuniversiteit te Groningen"}
MACRO {RUL} {"Rijksuniversiteit te Leiden"}
MACRO {RUU} {"Rijksuniversiteit te Utrecht"}
MACRO {TUD} {"Technische Universiteit Delft"}
MACRO {TUE} {"Technische Universiteit Eindhoven"}
MACRO {TUT} {"Universiteit Twente"}
MACRO {EUR} {"Erasmusuniversiteit Rotterdam"}
MACRO {UvA} {"Universiteit van Amsterdam"}
MACRO {VU}  {"Vrije Universiteit"}



% Een aantal natuurkundetijdschriften
%
MACRO {AcP}    {"Acta Physica"}
MACRO {AcPA}   {"Acta Physica Austriaca"}
MACRO {AcPP}   {"Acta Physica Polonica"}
MACRO {AdP}    {"Advances in Physics"}
MACRO {AdNP}   {"Advances in Nuclear Physics"}
MACRO {AmJP}   {"American Journal of Physics"}
MACRO {AnPDu}  {"Annalen der Physik (Leipzig)"}
MACRO {AnPFr}  {"Annales de Physique (Paris)"}
MACRO {AnPNY}  {"Annals of Physics (NY)"}
MACRO {ARevNS} {"Annual Review of Nuclear Science"}
MACRO {ADNDT}  {"Atomic Data and Nuclear Data Tables"}
MACRO {AusJP}  {"Australian Journal of Physics"}
MACRO {CanJP}  {"Canadian Journal Physics"}
MACRO {CPC}    {"Computer Physics Communications"}
MACRO {EJP}    {"European Journal of Physics"}
MACRO {EPL}    {"Europhysics Letters"}
MACRO {FBS}    {"Few Body Systems"}
MACRO {FBSSup} {"Few Body Systems Supplement"}
MACRO {FortP}  {"Fortschritte der Physik"}
MACRO {HePA}   {"Helvetica Physica Acta"}
MACRO {IEEE}   {"IEEE Transactions"}
MACRO {IJMP}   {"International Journal of Modern Physics"}
MACRO {JETP}   {"JETP"}
MACRO {JETPL}  {"JETP Letters"}
MACRO {JaFi}   {"{\cyr {\Ja}derna\ja\ Fizika}"}
MACRO {JMP}    {"Journal of Mathematical Physics"}
MACRO {JPFr}   {"Journal de Physique (Paris)"}
MACRO {JP}     {"Journal of Physics"}
MACRO {JPSJ}   {"Journal of the Physical Society of Japan"}
MACRO {JPSJSup}{"Journal of the Physical Society of Japan Supplement"}
MACRO {MPL}    {"Modern Physics Letters"}
MACRO {NCim}   {"Il Nuovo Cimento"}
MACRO {NDS}    {"Nuclear Data Sheets"}
MACRO {NP}     {"Nuclear Physics"}
MACRO {NPSup}  {"Nuclear Physics Supplements"}
MACRO {NTvN}   {"Nederlands Tijdschrift voor Natuurkunde"}
MACRO {Phy}    {"Physica"}
MACRO {PL}     {"Physics Letters"}
MACRO {PRSLon} {"Proceedings of the Royal Society of London"}
MACRO {PR}     {"Physical Review"}
MACRO {PRL}    {"Physical Review Letters"}
MACRO {PRep}   {"Physics Reports"}
MACRO {PScr}   {"Physica Scripta"}
MACRO {PToday} {"Physics Today"}
MACRO {PTP}    {"Progress of Theoretical Physics"}
MACRO {RMP}    {"Reviews of Modern Physics"}
MACRO {RPP}    {"Reports of Progress in Physics"}
MACRO {ScAm}   {"Scientific American"}
MACRO {SSin}   {"Scienta Sinica"}
MACRO {SovJNP} {"Soviet Journal of Nuclear Physics"}
MACRO {SovJPN} {"Soviet Journal of Particles and Nuclei"}
MACRO {SovPDo} {"Soviet Physics Doklady"}
MACRO {SovPUs} {"Soviet Physics Uspekhi"}
MACRO {SPAW}   {"Sitzungsberichte der Preussische Akademie der Wissenschaften"}
MACRO {STS}    {"Bulletin of Science, Technology and Social Affairs"}
MACRO {ZfN}    {"Zeitschrift f{\H{u}}r Naturforschung"}
MACRO {ZfP}    {"Zeitschrift f{\H{u}}r Physik"}



READ

EXECUTE {hello}

FUNCTION {sortify}
{ purify$
  "l" change.case$
}

INTEGERS { numberlen numberzero}

FUNCTION {numsortify}
{ purify$
  "l" change.case$
  's :=
  #1 'numberlen :=
  { s numberlen #1 substring$ "1" =
    s numberlen #1 substring$ "2" =
    s numberlen #1 substring$ "3" =
    s numberlen #1 substring$ "4" =
    s numberlen #1 substring$ "5" =
    s numberlen #1 substring$ "6" =
    s numberlen #1 substring$ "7" =
    s numberlen #1 substring$ "8" =
    s numberlen #1 substring$ "9" =
    s numberlen #1 substring$ "0" =
    or or or or or or or or or    }
  { numberlen #1 + 'numberlen :=  }
  while$
  #6 numberlen -  'numberzero :=
  "000000" #1 numberzero substring$
  s *
}
INTEGERS { len }

FUNCTION {chop.word}
{ 's :=
  'len :=
  s #1 len substring$ =
    { s len #1 + global.max$ substring$ }
    's
  if$
}

INTEGERS { et.al.char.used }

FUNCTION {initialize.et.al.char.used}
{ #0 'et.al.char.used :=
}

EXECUTE {initialize.et.al.char.used}


% this function is provided twice (v2.3 22-10-92): once for the full label
% once for the abbreviated label
% the abbreviated version is the version from v2.1
%
FUNCTION {format.lab.names.abbr}
{ 's :=
  s num.names$ 'numnames :=

  numnames #1 =                               % maak het citatielabel dat
    { s #1 "{vv~}{ll}" format.name$ One.cap}  % wordt afgedrukt
    { numnames #2 =                           %  b.v.  "van Dale"     (WS)
        {s #1 "{vv~}{ll} " format.name$ One.cap
         s #2 "{ll}" format.name$ "others" =
                { "{\em~et~al.}" * }
                { " en " * s #2 "{vv~}{ll}" format.name$ One.cap * }
         if$
        }
        { s #1 "{vv~}{ll}" format.name$ One.cap  "{\em~et~al.}" *}
      if$
    }
  if$

}

FUNCTION {format.lab.names.full}
{ 's :=
  #1 'nameptr :=
  s num.names$ 'numnames :=
  numnames 'namesleft :=
    { namesleft #0 > }                                    % maak het citatielabel dat
    { s nameptr "{vv~}{ll}" format.name$ One.cap  't :=   % wordt afgedrukt
      nameptr > #1                                        %  b.v.  "van Dale"     (WS)
      { namesleft #1 >
        { ", " * t * }
        { s "others" =
          { "{\em~et~al.}"  * }
          { " en " * t * }
          if$
        }
        if$
       }
      't
      if$
      nameptr #1 + 'nameptr :=
      namesleft #1 - 'namesleft :=
    }
    while$
    t
}


FUNCTION {author.key.label}
{ author empty$
    { key empty$
    { cite$ #1 #3 substring$ }
    { key }
      if$
    }
    { author format.lab.names.abbr }
  if$
}


%% (26-120-92): we proberen eerst de editor, dan de auteur
FUNCTION {author.editor.key.label}
{ editor empty$
    { author empty$
    { key empty$

        { cite$ #1 #3 substring$ }

        { key }
      if$
    }
    { author format.lab.names.abbr }
      if$
    }
    { editor format.lab.names.abbr }
  if$
}

FUNCTION {author.key.organization.label}
{ author empty$
    { key empty$
    { organization empty$

        { cite$ #1 #3 substring$ }

        { "The " #4 organization chop.word #3 text.prefix$ }
      if$
    }
    { key }
      if$
    }
    { author format.lab.names.abbr }
  if$
}

FUNCTION {editor.key.organization.label}
{ editor empty$
    { key empty$
    { organization empty$

        { cite$ #1 #3 substring$ }

        { "The " #4 organization chop.word #3 text.prefix$ }
      if$
    }
    { key }
      if$
    }
    { editor format.lab.names.abbr }
  if$
}

FUNCTION {calc.label}
{ type$ "book" =
  type$ "inbook" =
  or
    'author.editor.key.label
    { type$ "proceedings" =
    'editor.key.organization.label
    { type$ "manual" =
        'author.key.organization.label
        'author.key.label
      if$
    }
      if$
    }
  if$
    duplicate$
    year empty$
    { }
    { year * extra.label * }
    if$
  'sort.label :=
  'label :=                   % maak het af te drukken citeerlabel label
                              % en het sorteerlabel sort.label  (WS 16 03 92)
}

FUNCTION {sort.format.names}
{ 's :=
  #1 'nameptr :=
  ""
  s num.names$ 'numnames :=
  numnames 'namesleft :=
    { namesleft #0 > }
    { nameptr #1 >
    { "         " * }
    'skip$
      if$                 % het sorteerlabel begrijpt Nederlandse namen
                          % b.v  "Dale van JH"   (WS)
      s nameptr "{ll{ }}{vv{ }}{  f{ }}{  jj{ }}" format.name$ 't :=
      nameptr numnames = t "others" = and
    { "{\em~et~al}" * }
    { t sortify * }
      if$
      nameptr #1 + 'nameptr :=
      namesleft #1 - 'namesleft :=
    }
  while$
}

FUNCTION {sort.format.title}
{ 't :=
  "De " #3                                % titels worden gesorteerd
   "Een " #4                              % op het lidwoord na  (WS)
    "A " #2                               % verbeterd (2-11-93)
     "An " #3
      "The " #4 t chop.word
              chop.word
          chop.word
      chop.word
  chop.word
  sortify
  #1 global.max$ substring$
}

FUNCTION {author.sort}
{ author empty$
    { key empty$
    { "to sort, need author or key in " cite$ * warning$
      ""
    }
    { key sortify }
      if$
    }
    { author sort.format.names }
  if$
}


%% (26-10-92: we proberen eerst op editor te sorteren,
%% als dat niet kan op author
FUNCTION {author.editor.sort}
{ editor empty$
    { author empty$
    { key empty$
        { "to sort, need author, editor, or key in " cite$ * warning$
          ""
        }
        { key sortify }
      if$
    }
    { author sort.format.names }
      if$
    }
    { editor sort.format.names }
  if$
}

FUNCTION {author.organization.sort}
{ author empty$
    { organization empty$
    { key empty$
        { "to sort, need author, organization, or key in " cite$ * warning$
          ""
        }
        { key sortify }
      if$
    }
    { "The " #4 organization chop.word sortify }
      if$
    }
    { author sort.format.names }
  if$
}

FUNCTION {editor.organization.sort}
{ editor empty$
    { organization empty$
    { key empty$
        { "to sort, need editor, organization, or key in " cite$ * warning$
          ""
        }
        { key sortify }
      if$
    }
    { "The " #4 organization chop.word sortify }
      if$
    }
    { editor sort.format.names }
  if$
}

FUNCTION {presort}
{ calc.label                           % maak het label dat wordt afgedrukt
  " "                                  %
  type$ "book" =                       % maak nu het sorteerlabel
  type$ "inbook" =                     % (WS)
  or
    'author.editor.sort
    { type$ "proceedings" =
        'editor.organization.sort
        { type$ "manual" =
            'author.organization.sort
            'author.sort
          if$
        }
      if$
    }
  if$
  *
  "                                                                     "  *
                    % this fills up the author-string with tabs for proper
                    % alphabetization: `Dale and Dale' should come after `Dale'
  year field.or.null numsortify            % year.string bestaat hier nog niet
  *
  type$ "article" =
        { " " *
          journal field.or.null sortify *
          volume  field.or.null numsortify *
          pages   field.or.null numsortify * }
        { skip$    }
        if$
  "    "
  *
  title field.or.null
  sort.format.title
  *
  #1 entry.max$ substring$
  'sort.key$ :=
}

ITERATE {presort}

SORT

STRINGS { longest.label last.sort.label next.extra }

INTEGERS { longest.label.width last.extra.num }

FUNCTION {initialize.longest.label}
{ "" 'longest.label :=
  #0 int.to.chr$ 'last.sort.label :=
  "" 'next.extra :=
  #0 'longest.label.width :=
  #0 'last.extra.num :=
}

FUNCTION {forward.pass}           % (WS 16 03 92) sort.label instead of label
{ last.sort.label sort.label =
    { last.extra.num #1 + 'last.extra.num :=
      last.extra.num int.to.chr$ 'extra.label :=
    }
    { "a" chr.to.int$ 'last.extra.num :=
      "" 'extra.label :=
      sort.label 'last.sort.label :=
    }
  if$
}


FUNCTION {reverse.pass}         % (WS 16 03 92) year.string used
{ next.extra "b" =
    { "a" 'extra.label := }
    'skip$
  if$
  year empty$
    { "n.d." extra.label * 'year.string := }
    { year   extra.label * 'year.string := }
  if$
  extra.label 'next.extra :=
}


EXECUTE {initialize.longest.label}

ITERATE {forward.pass}

REVERSE {reverse.pass}

FUNCTION {begin.bib}

{ et.al.char.used
    { "\newcommand{\etalchar}[1]{$^{#1}$}" write$ newline$ }
    'skip$
  if$
  preamble$ empty$

    'skip$
    { preamble$ write$ newline$ }
  if$

  "\begin{thebibliography}{}" write$ newline$

}

EXECUTE {begin.bib}

EXECUTE {init.state.consts}

ITERATE {call.type$}

FUNCTION {end.bib}
{ newline$
  "\end{thebibliography}" write$ newline$
}

EXECUTE {end.bib}

