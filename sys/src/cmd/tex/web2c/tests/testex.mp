MetaPost would not accept this file.  It is just for
testing mptotex.

non-tex material is ignored.
`words' like betextify and etex_plus are OK too.
Now[]verbatimtex	 TeX \hsize="a0 in
more TeX  % with comments etex (That counts
even though it follows a %-sign
A string "even with etex or btex in it" is ignored
TeX material such as btex$a+b$ etex is often very short. btex
An initial newline is now ignored. etex (as of Version 0.60)

Removing the initial %-sign on any of the following
lines should cause an error:

% an "unbalanced string
% unmatched etex
% btex verbatimtex in tex mode etex
% btex tex text btex more text etex.
% a non-ending verbatimtex section
No visible "etex" here.
