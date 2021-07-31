% BibTeX styles for various physics journals (27-Feb-1990).
% This file, physics.btx, is based on the file btxbst.doc.
	% Please notify Charles Karney (Karney@Princeton.EDU)
	% of any bugs, improvements, etc.
% Run this file through cpp to get specific versions.  I have written
% a version of cpp within Emacs.  To access it put
%	(autoload 'cpp "tex$bibtex:cpp"		; Use cpp for BibTeX styles
%		  "C preprocessor"
%		  t)
% into your .emacs file.  Read in this file with
%	emacs tex$bibtex:physics.btx
% and run cpp with e.g.,
%	M-x cpp <ret> aip <ret>
% Save the resulting file to tex$latex:aip.bst.
% To accomodate the variations we need (in addition to the definitions below)
%	ATIT_SUPPRESS:	do not include the titles of journal articles
%	NAME_INVERT:	put the initials last
%	NAME_UPPER:	convert names to all caps.
%	MONTH_SUPPRESS:	do not include months in journal articles
%	PAREN_DATE:	dates go in parens in journal articles
%	DATE_FIRST:	dates go before page numbers
%	VOLUME_SPACE:	volume set off with space (not colon)
%	NUM_SUPPRESS:	do not include numbers in journal articles
%	BOLD_VOL:	volume numbers in bold face in journal articles
%	BRACKET_NOTE:	notes in brackets
%	PAGE_START_J:	include only starting page for journal articles
%	PAGE_START_O:	include only starting page for other entries
%	PAGE_ABBREV:	abbreviate page to p. and pages to pp.
%	PHYSICS_JOUR:	include abbreviations for Physics journals
%	COMMA_DELIMIT:	use comma (instead of period) to divide a citation
%	NOTE_SUPPRESS:	suppress the note
%	MAX_NAMES:	max number of authors before using et al (0 = infinity)
%	MIN_NAMES:	number of authors to list with et al.
%	EDIT_VAR:	use "edited by ..." instead of "..., editors"
%	RMP_LABELS:	create RMP-style labels
%	KEY_CITE:	use key as label
%	NAMED_REFS:	use (Smith, 1988) style of citations
%	CSC_NAMES:	names set in caps and small caps
%	JOUR_DEEMPH:	don't emphasize journal name
%	ETAL_EMPH:	emphasize et al.
%	ALT_INCOLL	alternate ordering of fields in incollections
% These are all boolean (0 or 1) except for MAX_NAMES and MIN_NAMES.
% These need to satisfy 0 <= MIN_NAMES <= MAX_NAMES.
% The rule here is that if all these are defined to be zero, we revert to
% the standard styles.  For that reason we initialize them all to 0.
% These are the original macros
	% For American Institute of Physics Journals
% This is the start of btxbst.doc
% BibTeX `plain' family
	% version 0.99b for BibTeX versions 0.99a or later, LaTeX version 2.09.
	% Copyright (C) 1985, all rights reserved.
	% Copying of this file is authorized only if either
	% (1) you make absolutely no changes to your copy, including name, or
	% (2) if you do make changes, you name it something other than
	% btxbst.doc, plain.bst, unsrt.bst, alpha.bst, and abbrv.bst.
	% This restriction helps ensure that all standard styles are identical.
	% The file btxbst.doc has the documentation for this style.
% Please notify Oren Patashnik (PATASHNIK@SCORE.STANFORD.EDU) of any bugs in
% these standard styles or in this documentation file.
%
% This is file btxbxt.doc; it helps document bibliography styles,
% and is also a template file that you can use to make
% several different style files, if you have access to a C preprocessor.
% For example, the standard styles were made by doing something like
%	cpp -P -DPLAIN btxbst.doc plain.txt
%	cpp -P -DUNSRT btxbst.doc unsrt.txt
%	cpp -P -DALPHA btxbst.doc alpha.txt
%	cpp -P -DABBRV btxbst.doc abbrv.txt
% and then renaming after removing unwanted comments and blank lines.
% If you don't have access,
% you can edit this file by hand to imitate the preprocessor,
% with the following explanation of the C preprocessor constructs used here.
%
% The output of the preprocessor is the same as the input, except that certain
% lines will be excluded (and some blank lines will be added).  The sequence
%	#if VAR
%	    lines to be included when VAR is not zero
%	#else
%	    lines to be included when VAR is zero
%	#endif
% (with the #-signs appearing in column 1) means that one set or the other of
% the lines are to be included depending on the value of VAR.
% The #else part is optional.  Comments can be added after #else and #endif.
% Variables can be set by
%	#define VAR value
% and one can also use #ifdef VAR to see if VAR has any value, and #ifndef
% to see if it has none.
% Another #if form used in this file is #if !VAR, which includes the lines
% after the #if only if VAR is zero.
%
% Convention: Use all uppercase identifiers for these preprocessor variables
% so you can spot them easily
%
% The command line to the preprocessor should define one of PLAIN, UNSRT, ALPHA
% or ABBRV (though PLAIN will be used by default if none is given),
% and the following lines will set various boolean variables to control the
% various lines that are chosen from the rest of the file.
% Each boolean variable should be set true (1) or false (0) in each style.
% Here are the current variables, and their meanings:
%	0:	an alphabetic label is used (if false then a numeric
%			    label is used)
%	0:		the entries should be sorted by label (if nonnumeric)
%			    and other info, like authors (if false, then
%			    entries remain in order of occurrence)
%	0:	the authors, editors, etc., get the full names as
%			    given in the bibliography file (if false, the first
%			    names become initials)
%	1:	titles of non-"books" (e.g., articles) should be
%			    converted to lower-case, except the first letter or
%			    first letter after a colon
%			    (if false then they appear as in the database)
%	0:	months are spelled out in full (if false, then
%			    they're abbreviated)
%	0:	macro journal names are spelled out in full
%			    (if false then they are abbreviated, currently
%			    as they appear in ACM publications)
%#	    define PLAIN 1
%
%   Entry formatting: Similar to that recommended by Mary-Claire van Leunen
%	in "A Handbook for Scholars".  Book-like titles are italicized
%	(emphasized) and non-book titles are converted to sentence
%	capitilization (and not enclosed in quotes).
%	This file outputs a \newblock between major blocks of an entry
%	(the name \newblock is analogous to the names \newline and \newpage)
%	so that the user can obtain an "open" format, which has a line break
%	before each block and lines after the first are indented within blocks,
%	by giving the optional \documentstyle argument `openbib';
%	The default is the "closed" format---blocks runs together.
%
%   Citation alphabetic label format:
%		[Knu73] for single author (or editor or key)
%		[AHU83] (first letters of last names) for multiple authors
%
%   Citation label numberic format:
%		[number]
%
%   Reference list ordering for sorted, alphabetic lables:
%		alphabetical by citation label, then by author(s) or whatever
%		passes for author in the absence of one, then by year,
%		then title
%
%   Reference list ordering for sorted, numeric lables:
%		alphabetical by author(s) or whatever passes
%		for author in the absence of one, then by year, then title
%
%   Reference list ordering for unsorted:
%		by the order cited in the text
%
%   History
%   12/16/84	(HWT)	Original `plain' version, by Howard Trickey.
%   12/23/84	(LL)	Some comments made by Leslie Lamport.
%    2/16/85	(OP)	Changes based on LL's comments, Oren Patashnik.
%    2/17/85	(HWT)	Template file and other standard styles made.
%    3/28/85	(OP)	First release, version 0.98b for BibTeX 0.98f.
%    5/ 9/85	(OP)	Version 0.98c for BibTeX 0.98i:
%			fixed Theoretical Computer Science macro name;
%			fixed the format.vol.num.pages function.
%    1/24/88	(OP)	Version 0.99a for BibTeX 0.99a, main changes:
%			assignment operator (:=) arguments reversed;
%			the preamble$ function outputs the database PREAMBLE;
%			entry.max$ and global.max$ (built-in) variables replace
%			entry.string.max and global.string.max functions;
%			alphabetizing by year then title, not just title;
%			many unnecessary ties removed; \it ==> \em;
%			the `alpha' style uses a superscripted `+' instead of a
%			`*' for unnamed names in constructing the label;
%			the `abbrv' style now uses "Mar." and "Sept.";
%			the functions calc.label and presort now look at just
%			the fields they're supposed to;
%			BOOKLET, MASTERSTHESIS, TECHREPORT use nonbook titles;
%			INBOOK and INCOLLECTION take an optional type (e.g.
%			type = "Section"), overriding the default "chapter";
%			BOOK, INBOOK, INCOLLECTION, and PROCEEDINGS now allow
%			either volume or number, not just volume;
%			INCOLLECTION now allows an edition and series field;
%			PROCEEDINGS and INPROCEEDINGS now use the address field
%			to tell where a conference was held;
%			INPROCEEDINGS and PROCEEDINGS now allow either volume
%			or number, and also a series field;
%			MASTERSTHESIS and PHDTHESIS accept types other than
%			"Master's thesis" and "PhD thesis";
%			UNPUBLISHED now outputs, in one block, note then date;
%			MANUAL now prints out the organization in
%			the first block if the author field is empty;
%			MISC can't be empty---it requires some optional field.
%    3/23/88	(OP)	Version 0.99b for BibTeX 0.99c---changed the three
%			erroneous occurrences of `cite ' to `cite$ '; this
%			change didn't affect the four standard styles, so the
%			0.99a versions of those styles are still current.
%
% The ENTRY declaration
%   Like Scribe's (according to pages 231-2 of the April '84 edition),
%   but no fullauthor or editors fields because BibTeX does name handling.
%   The annote field is commented out here because this family doesn't
%   include an annotated bibliography style.  And in addition to the fields
%   listed here, BibTeX has a built-in crossref field, explained later.
ENTRY
% Fields:
  { address
%	    Usually the address of a publisher or other type of organization.
%	    Put information in this field only if it helps the reader find the
%	    thing---for example you should omit the address of a major
%	    publisher entirely.  For a PROCEEDINGS or an INPROCEEDINGS,
%	    however, it's the address of the conference; for those two entry
%	    types, include the publisher's or organization's address, if
%	    necessary, in the publisher or organization field.
%    annote
%	    Long annotation---for annotated bibliographies (begins sentence).
    author
%	    Name(s) of author(s), in BibTeX name format.
    booktitle
%	    Book title when the thing being referenced isn't the whole book.
%	    For book entries, the title field should be used instead.
    chapter
%	    Chapter (or section or whatever) number.
    edition
%	    Edition of a book---should be an ordinal (e.g., "Second").
    editor
%	    Name(s) of editor(s), in BibTeX name format.
%	    If there is also an author field, then the editor field should be
%	    for the book or collection that the work appears in.
    howpublished
%	     How something strange has been published (begins sentence).
    institution
%	    Sponsoring institution of a technical report.
    journal
%	    Journal name (macros are provided for many).
    key
%	    Alphabetizing, labeling, and cross-referencing key
%	    (needed when an entry has no author or editor).
    month
%	    Month (macros are provided).
    note
%	    To help the reader find a reference (begins sentence).
    number
%	    Number of a journal or technical report, or of a work in a series.
    organization
%	    Organization sponsoring a conference (or publishing a manual); if
%	    the editor (or author) is empty, and if the organization produces
%	    an awkward label or cross reference, you should put appropriately
%	    condensed organization information in the key field as well.
    pages
%	    Page number or numbers (use `--' to separate a range, use `+'
%	    to indicate pages following that don't form a simple range).
    publisher
%	    Publisher name.
    school
%	    School name (for theses).
    series
%	    The name of a series or set of books.
%	    An individual book will will also have it's own title.
    title
%	    The title of the thing you're referred to.
    type
%	    Type of a Techreport (e.g., "Research Note") to be used instead of
%	    the default "Technical Report"; or, similarly, the type of a
%	    thesis; or of a part of a book.
    volume
%	    The volume number of a journal or multivolume work.
    year
%	    The year should contain only numerals (technically, it should end
%	    with four numerals, after purification; doesn't a begin sentence).
  }
% There are no integer entry variables
  {}
% These string entry variables are used to form the citation label.
% In a storage pinch, sort.label can be easily computed on the fly.
  { label }
% Each entry function starts by calling output.bibitem, to write the
% \bibitem and its arguments to the .BBL file.  Then the various fields
% are formatted and printed by output or output.check.  Those functions
% handle the writing of separators (commas, periods, \newblock's),
% taking care not to do so when they are passed a null string.
% Finally, fin.entry is called to add the final period and finish the
% entry.
%
% A bibliographic reference is formatted into a number of `blocks':
% in the open format, a block begins on a new line and subsequent
% lines of the block are indented.  A block may contain more than
% one sentence (well, not a grammatical sentence, but something to
% be ended with a sentence ending period).  The entry functions should
% call new.block whenever a block other than the first is about to be
% started.  They should call new.sentence whenever a new sentence is
% to be started.  The output functions will ensure that if two
% new.sentence's occur without any non-null string being output between
% them then there won't be two periods output.  Similarly for two
% successive new.block's.
%
% The output routines don't write their argument immediately.
% Instead, by convention, that argument is saved on the stack to be
% output next time (when we'll know what separator needs to come
% after it).  Meanwhile, the output routine has to pop the pending
% output off the stack, append any needed separator, and write it.
%
% To tell which separator is needed, we maintain an output.state.
% It will be one of these values:
%	before.all		just after the \bibitem
%	mid.sentence		in the middle of a sentence: comma needed
%					if more sentence is output
%	after.sentence		just after a sentence: period needed
%	after.block		just after a block (and sentence):
%					period and \newblock needed.
% Note: These styles don't use after.sentence
%
% VAR: output.state : INTEGER		-- state variable for output
%
% The output.nonnull function saves its argument (assumed to be nonnull)
% on the stack, and writes the old saved value followed by any needed
% separator.  The ordering of the tests is decreasing frequency of
% occurrence.
%
% output.nonnull(s) ==
%  BEGIN
%	s := argument on stack
%	if output.state = mid.sentence then
%	    write$(pop() * ", ")
%		  -- "pop" isn't a function: just use stack top
%	else
%	    if output.state = after.block then
%		write$(add.period$(pop()))
%		newline$
%		write$("\newblock ")
%	    else
%		if output.state = before.all then
%		    write$(pop())
%		else	    -- output.state should be after.sentence
%		    write$(add.period$(pop()) * " ")
%		fi
%	    fi
%	    output.state := mid.sentence
%	fi
%	push s on stack
%  END
%
% The output function calls output.nonnull if its argument is non-empty;
% its argument may be a missing field (thus, not necessarily a string)
%
% output(s) ==
%  BEGIN
%	if not empty$(s) then output.nonnull(s)
%	fi
%  END
%
% The output.check function is the same as the output function except that, if
% necessary, output.check warns the user that the t field shouldn't be empty
% (this is because it probably won't be a good reference without the field;
% the entry functions try to make the formatting look reasonable even when
% such fields are empty).
%
% output.check(s,t) ==
%  BEGIN
%	if empty$(s) then
%	    warning$("empty " * t * " in " * cite$)
%	else output.nonnull(s)
%	fi
%  END
%
% The output.bibitem function writes the \bibitem for the current entry
% (the label should already have been set up), and sets up the separator
% state for the output functions.  And, it leaves a string on the stack
% as per the output convention.
%
% output.bibitem ==
%  BEGIN
%	newline$
%	write$("\bibitem[")	% for alphabetic labels,
%	write$(label)		% these three lines
%	write$("]{")		% are used
%	write$("\bibitem{")		% this line for numeric labels
%	write$(cite$)
%	write$("}")
%	push "" on stack
%	output.state := before.all
%  END
%
% The fin.entry function finishes off an entry by adding a period to the
% string remaining on the stack.  If the state is still before.all
% then nothing was produced for this entry, so the result will look bad,
% but the user deserves it. (We don't omit the whole entry because the
% entry was cited, and a bibitem is needed to define the citation label.)
%
% fin.entry ==
%  BEGIN
%	write$(add.period$(pop()))
%	newline$
%  END
%
% The new.block function prepares for a new block to be output, and
% new.sentence prepares for a new sentence.
%
% new.block ==
%  BEGIN
%	if output.state <> before.all then
%	    output.state := after.block
%	fi
%  END
%
% new.sentence ==
%  BEGIN
%	if output.state <> after.block then
%	    if output.state <> before.all then
%		output.state :=  after.sentence
%	    fi
%	fi
%  END
%
INTEGERS { output.state before.all mid.sentence after.sentence after.block }
FUNCTION {init.state.consts}
{ #0 'before.all :=
  #1 'mid.sentence :=
  #2 'after.sentence :=
  #3 'after.block :=
}
% the variables s and t are temporary string holders
STRINGS { s t }
FUNCTION {output.nonnull}
{ 's :=
  output.state mid.sentence =
    { ", " * write$ }
    { output.state after.block =
	{ "," * write$
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
FUNCTION {output.bibitem}
{ newline$
  "\bibitem{" write$
  cite$ write$
  "}" write$
  newline$
  ""
  before.all 'output.state :=
}
% This function finishes all entries.
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
{ skip$
}
% These three functions pop one or two (integer) arguments from the stack
% and push a single one, either 0 or 1.
% The 'skip$ in the `and' and `or' functions are used because
% the corresponding if$ would be idempotent
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
% Sometimes we begin a new block only if the block will be big enough.  The
% new.block.checka function issues a new.block if its argument is nonempty;
% new.block.checkb does the same if either of its TWO arguments is nonempty.
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
% The new.sentence.check functions are analogous.
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
% Here are some functions for formatting chunks of an entry.
% By convention they either produce a string that can be followed by
% a comma or period (using add.period$, so it is OK to end in a period),
% or they produce the null string.
%
% A useful utility is the field.or.null function, which checks if the
% argument is the result of pushing a `missing' field (one for which no
% assignment was made when the current entry was read in from the database)
% or the result of pushing a string having no non-white-space characters.
% It returns the null string if so, otherwise it returns the field string.
% Its main (but not only) purpose is to guarantee that what's left on the
% stack is a string rather than a missing field.
%
% field.or.null(s) ==
%  BEGIN
%	if empty$(s) then return ""
%	else return s
%  END
%
% Another helper function is emphasize, which returns the argument emphazised,
% if that is non-empty, otherwise it returns the null string.  Italic
% corrections aren't used, so this function should be used when punctation
% will follow the result.
%
% emphasize(s) ==
%  BEGIN
%	if empty$(s) then return ""
%	else return "{\em " * s * "}"
%
% The format.names function formats the argument (which should be in
% BibTeX name format) into "First Von Last, Junior", separated by commas
% and with an "and" before the last (but ending with "et~al." if the last
% of multiple authors is "others").  This function's argument should always
% contain at least one name.
%
% VAR: nameptr, namesleft, numnames: INTEGER
% pseudoVAR: nameresult: STRING		(it's what's accumulated on the stack)
%
% format.names(s) ==
%  BEGIN
%	nameptr := 1
%	numnames := num.names$(s)
%	namesleft := numnames
%	while namesleft > 0
%	  do
%				% for full names:
%	    t := format.name$(s, nameptr, "{ff~}{vv~}{ll}{, jj}")
%				% for abbreviated first names:
%	    t := format.name$(s, nameptr, "{f.~}{vv~}{ll}{, jj}")
%	    if nameptr > 1 then
%		if namesleft > 1 then nameresult := nameresult * ", " * t
%		else if numnames > 2
%		       then nameresult := nameresult * ","
%		     fi
%		     if t = "others"
%		       then nameresult := nameresult * " et~al."
%		       else nameresult := nameresult * " and " * t
%		     fi
%		fi
%	    else nameresult := t
%	    fi
%	    nameptr := nameptr + 1
%	    namesleft := namesleft - 1
%	  od
%	return nameresult
%  END
%
% The format.authors function returns the result of format.names(author)
% if the author is present, or else it returns the null string
%
% format.authors ==
%  BEGIN
%	if empty$(author) then return ""
%	else return format.names(author)
%	fi
%  END
%
% Format.editors is like format.authors, but it uses the editor field,
% and appends ", editor" or ", editors"
%
% format.editors ==
%  BEGIN
%	if empty$(editor) then return ""
%	else
%	    if num.names$(editor) > 1 then
%		return format.names(editor) * ", editors"
%	    else
%		return format.names(editor) * ", editor"
%	    fi
%	fi
%  END
%
% Other formatting functions are similar, so no "comment version" will be
% given for them.
%
% The `pop$' in this function gets rid of the duplicate `empty' value and
% the `skip$' returns the duplicate field value
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
FUNCTION {embolden}
{ duplicate$ empty$
    { pop$ "" }
    { "{\bf " swap$ * "}" * }
  if$
}
FUNCTION {paren}
{ duplicate$ empty$
    { pop$ "" }
    { "(" swap$ * ")" * }
  if$
}
INTEGERS { nameptr namesleft numnames }
INTEGERS { etal }
FUNCTION {format.names}
{ 's :=
  #1 'nameptr :=
  s num.names$ 'numnames :=
  numnames #5 >
  s numnames "{ll}" format.name$ "others" = numnames #1 > and
  or 'etal :=
  etal
    { #1 #1 + 'namesleft := }
    { numnames 'namesleft := }
  if$
    { namesleft #0 > }
    { s nameptr "{f.~}{vv~}{ll}{, jj}" format.name$ 't :=
      nameptr #1 >
	{ namesleft #1 >
	    { ", " * t * }
	    { nameptr #2 >
		{ "," * }
		'skip$
	      if$
	      t "others" =
	      etal or
		{ " et~al." * }
		{ " and " * t * }
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
}
FUNCTION {format.authors}
{ author empty$
    { "" }
    { author format.names }
  if$
}
FUNCTION {format.editors}
{ editor empty$
    { "" }
    { editor format.names
      editor num.names$ #1 >
	{ ", editors" * }
	{ ", editor" * }
      if$
    }
  if$
}
FUNCTION {format.edited}
{ editor empty$
    { "" }
    { "edited by " editor format.names * }
  if$
}
% The format.title function is used for non-book-like titles.
% For most styles we convert to lowercase (except for the very first letter,
% and except for the first one after a colon (followed by whitespace)),
% and hope the user has brace-surrounded words that need to stay capitilized;
% for some styles, however, we leave it as it is in the database.
FUNCTION {format.title}
{ title empty$
    { "" }
    { title "t" change.case$ }
  if$
}
% By default, BibTeX sets the global integer variable global.max$ to the BibTeX
% constant glob_str_size, the maximum length of a global string variable.
% Analogously, BibTeX sets the global integer variable entry.max$ to
% ent_str_size, the maximum length of an entry string variable.
% The style designer may change these if necessary (but this is unlikely)
% The n.dashify function makes each single `-' in a string a double `--'
% if it's not already
%
% pseudoVAR: pageresult: STRING		(it's what's accumulated on the stack)
%
% n.dashify(s) ==
%  BEGIN
%	t := s
%	pageresult := ""
%	while (not empty$(t))
%	  do
%	    if (first character of t = "-")
%	      then
%		if (next character isn't)
%		  then
%		    pageresult := pageresult * "--"
%		    t := t with the "-" removed
%		  else
%		    while (first character of t = "-")
%		      do
%			pageresult := pageresult * "-"
%			t := t with the "-" removed
%		      od
%		fi
%	      else
%		pageresult := pageresult * the first character
%		t := t with the first character removed
%	    fi
%	  od
%	return pageresult
%  END
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
FUNCTION {first.page}
{ 't :=
  ""
    {  t empty$ not t #1 #1 substring$ "-" = not and }
    { t #1 #1 substring$ *
      t #2 global.max$ substring$ 't :=
    }
  while$
}
% The format.date function is for the month and year, but we give a warning if
% there's an empty year but the month is there, and we return the empty string
% if they're both empty.
FUNCTION {format.date}
{ year empty$
    { "" }
    'year
  if$
}
% The format.btitle is for formatting the title field when it is a book-like
% entry---the style used here keeps it in uppers-and-lowers and emphasizes it.
FUNCTION {format.btitle}
{ title emphasize
}
% For several functions we'll need to connect two strings with a
% tie (~) if the second one isn't very long (fewer than 3 characters).
% The tie.or.space.connect function does that.  It concatenates the two
% strings on top of the stack, along with either a tie or space between
% them, and puts this concatenation back onto the stack:
%
% tie.or.space.connect(str1,str2) ==
%    BEGIN
%	if text.length$(str2) < 3
%	  then return the concatenation of str1, "~", and str2
%	  else return the concatenation of str1, " ", and str2
%    END
FUNCTION {tie.or.space.connect}
{ duplicate$ text.length$ #3 <
    { "~" }
    { " " }
  if$
  swap$ * *
}
% The either.or.check function complains if both fields or an either-or pair
% are nonempty.
%
% either.or.check(t,s) ==
%  BEGIN
%	if empty$(s) then
%	    warning$(can't use both " * t * " fields in " * cite$)
%	fi
%  END
FUNCTION {either.or.check}
{ empty$
    'pop$
    { "can't use both " swap$ * " fields in " * cite$ * warning$ }
  if$
}
% The format.bvolume function is for formatting the volume and perhaps
% series name of a multivolume work.  If both a volume and a series field
% are there, we assume the series field is the title of the whole multivolume
% work (the title field should be the title of the thing being referred to),
% and we add an "of <series>".  This function is called in mid-sentence.
FUNCTION {format.bvolume}
{ volume empty$
    { "" }
    { "volume" volume tie.or.space.connect
      series empty$
	'skip$
	{ " of " * series emphasize * }
      if$
      "volume and number" number either.or.check
    }
  if$
}
% The format.number.series function is for formatting the series name
% and perhaps number of a work in a series.  This function is similar to
% format.bvolume, although for this one the series must exist (and the
% volume must not exist).  If the number field is empty we output either
% the series field unchanged if it exists or else the null string.
% If both the number and series fields are there we assume the series field
% gives the name of the whole series (the title field should be the title
% of the work being one referred to), and we add an "in <series>".
% We capitilize Number when this function is used at the beginning of a block.
FUNCTION {format.number.series}
{ volume empty$
    { number empty$
	{ series field.or.null }
	{ output.state mid.sentence =
	    { "number" }
	    { "Number" }
	  if$
	  number tie.or.space.connect
	  series empty$
	    { "there's a number but no series in " cite$ * warning$ }
	    { " in " * series * }
	  if$
	}
      if$
    }
    { "" }
  if$
}
% The format.edition function appends " edition" to the edition, if present.
% We lowercase the edition (it should be something like "Third"), because
% this doesn't start a sentence.
FUNCTION {format.edition}
{ edition empty$
    { "" }
    { output.state mid.sentence =
	{ edition "l" change.case$ " edition" * }
	{ edition "t" change.case$ " edition" * }
      if$
    }
  if$
}
% The format.pages function is used for formatting a page range in a book
% (and in rare circumstances, an article).
%
% The multi.page.check function examines the page field for a "-" or "," or "+"
% so that format.pages can use "page" instead of "pages" if none exists.
% Note: global.max$ here means "take the rest of the string"
%
% VAR: multiresult: INTEGER	(actually, a boolean)
%
% multi.page.check(s) ==
%  BEGIN
%	t := s
%	multiresult := false
%	while ((not multiresult) and (not empty$(t)))
%	  do
%	    if (first character of t = "-" or "," or "+")
%	      then multiresult := true
%	      else t := t with the first character removed
%	    fi
%	  od
%	return multiresult
%  END
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
% This function doesn't begin a sentence so "pages" isn't capitalized.
% Other functions that use this should keep that in mind.
FUNCTION {format.pages}
{ pages empty$
    { "" }
    { pages multi.page.check
	{ "pages" pages n.dashify tie.or.space.connect }
	{ "page" pages tie.or.space.connect }
      if$
    }
  if$
}
FUNCTION {format.pages.a}
{ pages empty$
    { "" }
    { "page" pages first.page tie.or.space.connect }
  if$
}
% The format.vol.num.pages function is for the volume, number, and page range
% of a journal article.  We use the format:  vol(number):pages, with some
% variations for empty fields.  This doesn't begin a sentence.
FUNCTION {format.vol.num.pages}
{ volume field.or.null embolden
  " " swap$ * *
  pages empty$
    'skip$
    { duplicate$ empty$
	{ pop$ format.pages.a }
	{ ", " * pages first.page * }
      if$
    }
  if$
}
% The format.chapter.pages, if the chapter is present, puts whatever is in the
% type field (or else "chapter" if type is empty) in front of a chapter number.
% It then appends the pages, if present.  This doesn't begin a sentence.
FUNCTION {format.chapter.pages}
{ chapter empty$
    'format.pages
    { type empty$
	{ "chapter" }
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
% The format.in.ed.booktitle function is used for starting out a sentence
% that begins "In <booktitle>", putting an editor before the title if one
% exists.
FUNCTION {format.in.ed.booktitle}
{ booktitle empty$
    { "" }
    { editor empty$
	{ "in " booktitle emphasize * }
	{ "in " booktitle emphasize * ", " * format.edited * }
      if$
    }
  if$
}
% The function empty.misc.check complains if all six fields are empty, and
% if there's been no sorting or alphabetic-label complaint.
FUNCTION {empty.misc.check}
{ author empty$ title empty$ howpublished empty$
  month empty$ year empty$ note empty$
  and and and and and
    { "all relevant fields are empty in " cite$ * warning$ }
    'skip$
  if$
}
% The function format.thesis.type returns either the (case-changed) type field,
% if it is defined, or else the default string already on the stack
% (like "Master's thesis" or "PhD thesis").
FUNCTION {format.thesis.type}
{ type empty$
    'skip$
    { pop$
      type "t" change.case$
    }
  if$
}
% The function format.tr.number makes a string starting with "Technical Report"
% (or type, if that field is defined), followed by the number if there is one;
% it returns the starting part (with a case change) even if there is no number.
% This is used at the beginning of a sentence.
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
% Now come the cross-referencing functions (these are invoked because
% one entry in the database file(s) cross-references another, by giving
% the other entry's database key in a `crossref' field).  This feature
% allows one or more titled things that are part of a larger titled
% thing to cross-reference the larger thing.  These styles allow for
% five posibilities: (1) an ARTICLE may cross-reference an ARTICLE;
% (2) a BOOK, (3) INBOOK, or (4) INCOLLECTION may cross-reference a BOOK;
% or (5) an INPROCEEDINGS may cross-reference a PROCEEDINGS.
% Each of these is explained in more detail later.
%
% An ARTICLE entry type may cross reference another ARTICLE (this is
% intended for when an entire journal is devoted to a single topic---
% but since there is no JOURNAL entry type, the journal, too, should be
% classified as an ARTICLE but without the author and title fields).
% This will result in two warning messages for the journal's entry
% if it's included in the reference list, but such is life.
%
% format.article.crossref ==
%  BEGIN
%	if empty$(key) then
%	    if empty$(journal) then
%		warning$("need key or journal for " * cite$ *
%						" to crossref " * crossref)
%		return(" \cite{" * crossref * "}")
%	    else
%		return("In " * emphazise.correct (journal) *
%						" \cite{" * crossref * "}")
%		fi
%	else
%	    return("In " * key * " \cite{" * crossref * "}")
%	fi
%  END
%
% The other cross-referencing functions are similar, so no "comment version"
% will be given for them.
FUNCTION {format.article.crossref}
{ key empty$
    { journal empty$
	{ "need key or journal for " cite$ * " to crossref " * crossref *
	  warning$
	  ""
	}
	{ "In " journal * }
      if$
    }
    { "In " key * }
  if$
  " \cite{" * crossref * "}" *
}
% We use just the last names of editors for a cross reference: either
% "editor", or "editor1 and editor2", or "editor1 et~al." depending on
% whether there are one, or two, or more than two editors.
FUNCTION {format.crossref.editor}
{ editor #1 "{vv~}{ll}" format.name$
  editor num.names$ duplicate$
  #2 >
    { pop$ " et~al." * }
    { #2 <
	'skip$
	{ editor #2 "{ff }{vv }{ll}{ jj}" format.name$ "others" =
	    { " et~al." * }
	    { " and " * editor #2 "{vv~}{ll}" format.name$ * }
	  if$
	}
      if$
    }
  if$
}
% A BOOK (or INBOOK) entry type (assumed to be for a single volume in a
% multivolume work) may cross reference another BOOK (the entire multivolume).
% Usually there will be an editor, in which case we use that to construct the
% cross reference; otherwise we use a nonempty key field or else the series
% field (since the series gives the title of the multivolume work).
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
    }
    { format.crossref.editor * }
  if$
  " \cite{" * crossref * "}" *
}
% An INCOLLECTION entry type may cross reference a BOOK (assumed to be the
% collection), or an INPROCEEDINGS may cross reference a PROCEEDINGS.
% Often there will be an editor, in which case we use that to construct
% the cross reference; otherwise we use a nonempty key field or else
% the booktitle field (which gives the cross-referenced work's title).
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
    }
    { "In " format.crossref.editor * }
  if$
  " \cite{" * crossref * "}" *
}
% Now we define the type functions for all entry types that may appear
% in the .BIB file---e.g., functions like `article' and `book'.  These
% are the routines that actually generate the .BBL-file output for
% the entry.  These must all precede the READ command.  In addition, the
% style designer should have a function `default.type' for unknown types.
% Note: The fields (within each list) are listed in order of appearance,
% except as described for an `inbook' or a `proceedings'.
%
% The article function is for an article in a journal.  An article may
% CROSSREF another article.
%	Required fields: author, title, journal, year
%	Optional fields: volume, number, pages, month, note
%
% article ==
%  BEGIN
%	output.bibitem
%	output.check(format.authors,"author")
%	new.block
%	output.check(format.title,"title")
%	new.block
%	if missing$(crossref) then
%	    output.check(emphasize(journal),"journal")
%	    output(format.vol.num.pages)
%	    output.check(format.date,"year")
%       else
%	    output.nonnull(format.article.crossref)
%	    output(format.pages)
%	fi
%	new.block
%	output(note)
%	fin.entry
%  END
%
% The book function is for a whole book.  A book may CROSSREF another book.
%	Required fields: author or editor, title, publisher, year
%	Optional fields: volume or number, series, address, edition, month,
%			note
%
% book ==
%  BEGIN
%	if empty$(author) then output.check(format.editors,"author and editor")
%	else	output.check(format.authors,"author")
%		if missing$(crossref) then
%		    either.or.check("author and editor",editor)
%		fi
%	fi
%	new.block
%	output.check(format.btitle,"title")
%	if missing$(crossref) then
%	    output(format.bvolume)
%	    new.block
%	    output(format.number.series)
%	    new.sentence
%	    output.check(publisher,"publisher")
%	    output(address)
%       else
%	    new.block
%	    output.nonnull(format.book.crossref)
%	fi
%	output(format.edition)
%	output.check(format.date,"year")
%	new.block
%	output(note)
%	fin.entry
%  END
%
% The other entry functions are all quite similar, so no "comment version"
% will be given for them.
FUNCTION {article}
{ output.bibitem
  format.authors "author" output.check
  new.block
  crossref missing$
    { journal field.or.null
      format.vol.num.pages 
      format.date empty$
	'skip$
	{ duplicate$ empty$
	    { pop$ format.date paren }
            { " " * format.date paren * }
	  if$
	}
      if$
      output
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
      crossref missing$
	{ "author and editor" editor either.or.check }
	'skip$
      if$
    }
  if$
  new.block
  format.btitle "title" output.check
  crossref missing$
    { format.bvolume output
      new.block
      format.number.series output
      new.sentence
      publisher "publisher" output.check
      address output
    }
    { new.block
      format.book.crossref output.nonnull
    }
  if$
  format.edition output
  format.date "year" output.check
  new.block
  note output
  fin.entry
}
% A booklet is a bound thing without a publisher or sponsoring institution.
%	Required: title
%	Optional: author, howpublished, address, month, year, note
FUNCTION {booklet}
{ output.bibitem
  format.authors output
  new.block
  format.title "title" output.check
  howpublished address new.block.checkb
  howpublished output
  address output
  format.date output
  new.block
  note output
  fin.entry
}
% For the conference entry type, see inproceedings.
% An inbook is a piece of a book: either a chapter and/or a page range.
% It may CROSSREF a book.  If there's no volume field, the type field
% will come before number and series.
%	Required: author or editor, title, chapter and/or pages, publisher,year
%	Optional: volume or number, series, type, address, edition, month, note
FUNCTION {inbook}
{ output.bibitem
  author empty$
    { format.editors "author and editor" output.check }
    { format.authors output.nonnull
      crossref missing$
	{ "author and editor" editor either.or.check }
	'skip$
      if$
    }
  if$
  new.block
  format.btitle "title" output.check
  crossref missing$
    { format.bvolume output
      format.chapter.pages "chapter and pages" output.check
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
  format.date "year" output.check
  new.block
  note output
  fin.entry
}
% An incollection is like inbook, but where there is a separate title
% for the referenced thing (and perhaps an editor for the whole).
% An incollection may CROSSREF a book.
%	Required: author, title, booktitle, publisher, year
%	Optional: editor, volume or number, series, type, chapter, pages,
%			address, edition, month, note
FUNCTION {incollection}
{ output.bibitem
  format.authors "author" output.check
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
      format.date "year" output.check
    }
    { format.incoll.inproc.crossref output.nonnull
      format.chapter.pages output
    }
  if$
  new.block
  note output
  fin.entry
}
% An inproceedings is an article in a conference proceedings, and it may
% CROSSREF a proceedings.  If there's no address field, the month (& year)
% will appear just before note.
%	Required: author, title, booktitle, year
%	Optional: editor, volume or number, series, pages, address, month,
%			organization, publisher, note
FUNCTION {inproceedings}
{ output.bibitem
  format.authors "author" output.check
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
	  format.date "year" output.check
	}
	{ address output.nonnull
	  format.date "year" output.check
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
% The conference function is included for Scribe compatibility.
FUNCTION {conference} { inproceedings }
% A manual is technical documentation.
%	Required: title
%	Optional: author, organization, address, edition, month, year, note
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
  format.date output
  new.block
  note output
  fin.entry
}
% A mastersthesis is a Master's thesis.
%	Required: author, title, school, year
%	Optional: type, address, month, note
FUNCTION {mastersthesis}
{ output.bibitem
  format.authors "author" output.check
  new.block
  format.title "title" output.check
  new.block
  "Master's thesis" format.thesis.type output.nonnull
  school "school" output.check
  address output
  format.date "year" output.check
  new.block
  note output
  fin.entry
}
% A misc is something that doesn't fit elsewhere.
%	Required: at least one of the `optional' fields
%	Optional: author, title, howpublished, month, year, note
FUNCTION {misc}
{ output.bibitem
  format.authors output
  title howpublished new.block.checkb
  format.title output
  howpublished new.block.checka
  howpublished output
  format.date output
  new.block
  note output
  fin.entry
  empty.misc.check
}
% A phdthesis is like a mastersthesis.
%	Required: author, title, school, year
%	Optional: type, address, month, note
FUNCTION {phdthesis}
{ output.bibitem
  format.authors "author" output.check
  new.block
  format.btitle "title" output.check
  new.block
  "PhD thesis" format.thesis.type output.nonnull
  school "school" output.check
  address output
  format.date "year" output.check
  new.block
  note output
  fin.entry
}
% A proceedings is a conference proceedings.
% If there is an organization but no editor field, the organization will
% appear as the first optional field (we try to make the first block nonempty);
% if there's no address field, the month (& year) will appear just before note.
%	Required: title, year
%	Optional: editor, volume or number, series, address, month,
%			organization, publisher, note
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
% A techreport is a technical report.
%	Required: author, title, institution, year
%	Optional: type, number, address, month, note
FUNCTION {techreport}
{ output.bibitem
  format.authors "author" output.check
  new.block
  format.title "title" output.check
  new.block
  format.tr.number output.nonnull
  institution "institution" output.check
  address output
  format.date "year" output.check
  new.block
  note output
  fin.entry
}
% An unpublished is something that hasn't been published.
%	Required: author, title, note
%	Optional: month, year
FUNCTION {unpublished}
{ output.bibitem
  format.authors "author" output.check
  new.block
  format.title "title" output.check
  new.block
  note "note" output.check
  format.date output
  fin.entry
}
% We use entry type `misc' for an unknown type; BibTeX gives a warning.
FUNCTION {default.type} { misc }
% Here are macros for common things that may vary from style to style.
% Users are encouraged to use these macros.
%
% Months are either written out in full or abbreviated
MACRO {jan} {"Jan."}
MACRO {feb} {"Feb."}
MACRO {mar} {"Mar."}
MACRO {apr} {"Apr."}
MACRO {may} {"May"}
MACRO {jun} {"June"}
MACRO {jul} {"July"}
MACRO {aug} {"Aug."}
MACRO {sep} {"Sept."}
MACRO {oct} {"Oct."}
MACRO {nov} {"Nov."}
MACRO {dec} {"Dec."}
% Journals are either written out in full or abbreviated;
% the abbreviations are like those found in ACM publications.
%
% To get a completely different set of abbreviations, it may be best to make
% a separate .bib file with nothing but those abbreviations; users could then
% include that file name as the first argument to the \bibliography command
MACRO {acmcs} {"ACM Comput. Surv."}
MACRO {acta} {"Acta Inf."}
MACRO {cacm} {"Commun. ACM"}
MACRO {ibmjrd} {"IBM J. Res. Dev."}
MACRO {ibmsj} {"IBM Syst.~J."}
MACRO {ieeese} {"IEEE Trans. Softw. Eng."}
MACRO {ieeetc} {"IEEE Trans. Comput."}
MACRO {ieeetcad}
 {"IEEE Trans. Comput.-Aided Design Integrated Circuits"}
MACRO {ipl} {"Inf. Process. Lett."}
MACRO {jacm} {"J.~ACM"}
MACRO {jcss} {"J.~Comput. Syst. Sci."}
MACRO {scp} {"Sci. Comput. Programming"}
MACRO {sicomp} {"SIAM J. Comput."}
MACRO {tocs} {"ACM Trans. Comput. Syst."}
MACRO {tods} {"ACM Trans. Database Syst."}
MACRO {tog} {"ACM Trans. Gr."}
MACRO {toms} {"ACM Trans. Math. Softw."}
MACRO {toois} {"ACM Trans. Office Inf. Syst."}
MACRO {toplas} {"ACM Trans. Prog. Lang. Syst."}
MACRO {tcs} {"Theoretical Comput. Sci."}
%% /usr/local/lib/tex/bibtex/phyjfull.btx, Tue Jun 30 08:37:48 1992
%% Edit by Nelson H. F. Beebe <beebe@alfred.math.utah.edu>
%% Change file names in leading comment
% Journal Title Abbreviations from the Physical Review Style and Notation
%	Guide, July, 1983, in BAPS Vol. 28.
%  Selected by Cris Barnes and Charles Karney, March 1988
% IMPORTANT!! Don't make changes to this file without making the
% 	      corresponding changes to
%			phyjabb.bib
%			phyjabb.btx
MACRO {advp} {"Advances in Physics"}
MACRO {ajp} {"American Journal of Physics"}
MACRO {ao} {"Applied Optics"}
MACRO {apl} {"Applied Physics Letters"}
MACRO {apj} {"Astrophysical Journal"}
MACRO {baps} {"Bulletin of the American Physical Society"}
MACRO {cpc} {"Computer Physics Communications"}
MACRO {cppcf} {"Comments on Plasma Physics and Controlled Fusion"}
MACRO {fed} {"Fusion Engineering and Design"}
MACRO {ft} {"Fusion Technology"}
MACRO {ieeens} {"IEEE Transactions on Nuclear Science"}
MACRO {ieeeps} {"IEEE Transactions on Plasma Science"}
MACRO {ijimw} {"International Journal of Infrared and Millimeter Waves"}
MACRO {ip} {"Infrared Physics"}
MACRO {jap} {"Journal of Applied Physics"}
MACRO {jcp} {"Journal of Computational Physics"}
MACRO {jetp} {"Soviet Physics-JETP"}
MACRO {jfe} {"Journal of Fusion Energy"}
MACRO {jfm} {"Journal of Fluid Mechanics"}
MACRO {jgr} {"Journal of Geophysical Research"}
MACRO {jmp} {"Journal of Mathematical Physics"}
MACRO {jne} {"Journal of Nuclear Energy"}
MACRO {jnec} {"Journal of Nuclear Energy, Part C: Plasma Physics, Accelerators, Thermonuclear Research"}
MACRO {jnm} {"Journal of Nuclear Materials"}
MACRO {josa} {"Journal of the Optical Society of America"}
MACRO {jpp} {"Journal of Plasma Physics"}
MACRO {jpsj} {"Journal of the Physical Society of Japan"}
MACRO {jvst} {"Journal of Vacuum Science and Technology"}
MACRO {nedf} {"Nuclear Engineering and Design/Fusion"}
MACRO {nf} {"Nuclear Fusion"}
MACRO {nim} {"Nuclear Instruments and Methods"}
MACRO {nimpr} {"Nuclear Instruments and Methods in Physics Research"}
MACRO {nt/f} {"Nuclear Technology/Fusion"}
MACRO {pf} {"Physics of Fluids"}
MACRO {pfa} {"Physics of Fluids A: Fluid Dynamics"}
MACRO {pfb} {"Physics of Fluids B: Plasma Physics"}
MACRO {pl} {"Physics Letters"}
MACRO {pla} {"Physics Letters A"}
MACRO {pnas} {"Proceedings of the National Academy of Sciences of the USA"}
MACRO {pp} {"Plasma Physics"}
MACRO {ppcf} {"Plasma Physics and Controlled Fusion"}
MACRO {prl} {"Physical Review Letters"}
MACRO {pr} {"Physical Review"}
MACRO {pra} {"Physical Review A: General Physics"}
MACRO {ps} {"Physica Scripta"}
MACRO {rmp} {"Reviews of Modern Physics"}
MACRO {rsi} {"Review of Scientific Instruments"}
MACRO {sjpp} {"Soviet Journal of Plasma Phys."}
MACRO {spd} {"Soviet Physics-Doklady"}
MACRO {sptp} {"Soviet Physics-Technical Physics"}
MACRO {spu} {"Soviet Physics-Uspeki"}
% Now we read in the .BIB entries.
READ
% The sortify function converts to lower case after purify$ing; it's
% used in sorting and in computing alphabetic labels after sorting
%
% The chop.word(w,len,s) function returns either s or, if the first len
% letters of s equals w (this comparison is done in the third line of the
% function's definition), it returns that part of s after w.
% This long comment applies only to alphabetic labels
%
% The format.lab.names function makes a short label by using the initials of
% the von and Last parts of the names (but if there are more than four names,
% (i.e., people) it truncates after three and adds a superscripted "+";
% it also adds such a "+" if the last of multiple authors is "others").
% If there is only one name, and its von and Last parts combined have just
% a single name-token ("Knuth" has a single token, "Brinch Hansen" has two),
% we take the first three letters of the last name.  The boolean
% et.al.char.used tells whether we've used a superscripted "+", so that we
% know whether to include a LaTeX macro for it.
%
% format.lab.names(s) ==
%  BEGIN
%	numnames := num.names$(s)
%	if numnames > 1 then
%	    if numnames > 4 then
%		namesleft := 3
%	    else
%		namesleft := numnames
%	    nameptr := 1
%	    nameresult := ""
%	    while namesleft > 0
%	      do
%		if (name_ptr = numnames) and
%		     format.name$(s, nameptr, "{ff }{vv }{ll}{ jj}") = "others"
%		   then nameresult := nameresult * "{\etalchar{+}}"
%			et.al.char.used := true
%		   else nameresult := nameresult *
%				format.name$(s, nameptr, "{v{}}{l{}}")
%		nameptr := nameptr + 1
%		namesleft := namesleft - 1
%	      od
%	    if numnames > 4 then
%		nameresult := nameresult * "{\etalchar{+}}"
%		et.al.char.used := true
%	else
%	    t := format.name$(s, 1, "{v{}}{l{}}")
%	    if text.length$(t) < 2 then	% there's just one name-token
%		nameresult := text.prefix$(format.name$(s,1,"{ll}"),3)
%	    else
%		nameresult := t
%	    fi
%	fi
%	return nameresult
%  END
%
% Exactly what fields we look at in constructing the primary part of the label
% depends on the entry type; this selectivity (as opposed to, say, always
% looking at author, then editor, then key) helps ensure that "ignored" fields,
% as described in the LaTeX book, really are ignored.  Note that MISC is part
% of the deepest `else' clause in the nested part of calc.label; thus, any
% unrecognized entry type in the database is handled correctly.
%
% There is one auxiliary function for each of the four different sequences of
% fields we use.  The first of these functions looks at the author field, and
% then, if necessary, the key field.  The other three functions, which might
% look at two fields and the key field, are similar, except that the key field
% takes precedence over the organization field (for labels---not for sorting).
%
% The calc.label function calculates the preliminary label of an entry, which
% is formed by taking three letters of information from the author or editor or
% key or organization field (depending on the entry type and on what's empty,
% but ignoring a leading "The " in the organization), and appending the last
% two characters (digits) of the year. It is an error if the appropriate fields
% among author, editor, organization, and key are missing, and we use
% the first three letters of the cite$ in desperation when this happens.
% The resulting label has the year part, but not the name part, purify$ed
% (purify$ing the year allows some sorting shenanigans by the user).
%
% This function also calculates the version of the label to be used in sorting.
%
% The final label may need a trailing 'a', 'b', etc., to distinguish it from
% otherwise identical labels, but we can't calculated those "extra.label"s
% until after sorting.
%
% calc.label ==
%  BEGIN
%	if type$ = "book" or "inbook" then
%	    author.editor.key.label
%	else if type$ = "proceedings" then
%	    editor.key.organization.label
%	else if type$ = "manual" then
%	    author.key.organization.label
%	else
%	    author.key.label
%	fi fi fi
%	label := label * substring$(purify$(field.or.null(year)), -1, 2)
%		% assuming we will also sort, we calculate a sort.label
%	sort.label := sortify(label), but use the last four, not two, digits
%  END
% When sorting, we compute the sortkey by executing "presort" on each entry.
% The presort key contains a number of "sortify"ed strings, concatenated
% with multiple blanks between them.  This makes things like "brinch  per"
% come before "brinch hansen  per".
%
% The fields used here are: the sort.label for alphabetic labels (as set by
% calc.label), followed by the author names (or editor names or organization
% (with a leading "The " removed) or key field, depending on entry type and on
% what's empty), followed by year, followed by the first bit of the title
% (chopping off a leading "The ", "A ", or "An ").
% Names are formatted: Von Last First Junior.
% The names within a part will be separated by a single blank
% (such as "brinch hansen"), two will separate the name parts themselves
% (except the von and last), three will separate the names,
% four will separate the names from year (and from label, if alphabetic),
% and four will separate year from title.
%
% The sort.format.names function takes an argument that should be in
% BibTeX name format, and returns a string containing "   "-separated
% names in the format described above.  The function is almost the same
% as format.names.
% This long comment applies only to alphabetic labels, when sorted
%
% Now comes the final computation for alphabetic labels, putting in the 'a's
% and 'b's and so forth if required.  This involves two passes: a forward
% pass to put in the 'b's, 'c's and so on, and a backwards pass
% to put in the 'a's (we don't want to put in 'a's unless we know there
% are 'b's).
% We have to keep track of the longest (in width$ terms) label, for use
% by the "thebibliography" environment.
%
% VAR: longest.label, last.sort.label, next.extra: string
%      longest.label.width, last.extra.num: integer
%
% initialize.longest.label ==
%  BEGIN
%	longest.label := ""
%	last.sort.label := int.to.chr$(0)
%	next.extra := ""
%	longest.label.width := 0
%	last.extra.num := 0
%  END
%
% forward.pass ==
%  BEGIN
%	if last.sort.label = sort.label then
%	    last.extra.num := last.extra.num + 1
%	    extra.label := int.to.chr$(last.extra.num)
%	else
%	    last.extra.num := chr.to.int$("a")
%	    extra.label := ""
%	    last.sort.label := sort.label
%	fi
%  END
%
% reverse.pass ==
%  BEGIN
%	if next.extra = "b" then
%	    extra.label := "a"
%	fi
%	label := label * extra.label
%	if width$(label) > longest.label.width then
%	    longest.label := label
%	    longest.label.width := width$(label)
%	fi
%	next.extra := extra.label
%  END
% Now comes the computation for numeric labels.
% We use either the sorted order or original order.
% We still have to keep track of the longest (in width$ terms) label, for use
% by the "thebibliography" environment.
STRINGS { longest.label }
INTEGERS { number.label longest.label.width }
FUNCTION {initialize.longest.label}
{ "" 'longest.label :=
  #1 'number.label :=
  #0 'longest.label.width :=
}
FUNCTION {longest.label.pass}
{ number.label int.to.str$ 'label :=
  number.label #1 + 'number.label :=
  label width$ longest.label.width >
    { label 'longest.label :=
      label width$ 'longest.label.width :=
    }
    'skip$
  if$
}
EXECUTE {initialize.longest.label}
ITERATE {longest.label.pass}
% Now we're ready to start writing the .BBL file.
% We begin, if necessary, with a LaTeX macro for unnamed names in an alphabetic
% label; next comes stuff from the `preamble' command in the database files.
% Then we give an incantation containing the command
%     \begin{thebibliography}{...}
% where the `...' is the longest label.
%
% We also call init.state.consts, for use by the output routines.
FUNCTION {begin.bib}
{ preamble$ empty$
    'skip$
    { preamble$ write$ newline$ }
  if$
  "\begin{thebibliography}{"  longest.label  * "}" * write$ newline$
}
EXECUTE {begin.bib}
EXECUTE {init.state.consts}
% Now we produce the output for all the entries
ITERATE {call.type$}
% Finally, we finish up by writing the `\end{thebibliography}' command.
FUNCTION {end.bib}
{ newline$
  "\end{thebibliography}" write$ newline$
}
EXECUTE {end.bib}
