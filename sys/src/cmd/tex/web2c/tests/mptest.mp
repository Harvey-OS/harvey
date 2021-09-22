% bug-test.mp -- Test for the presence or absence of known bugs.
%
% Ulrik Vieth, 1996/12/03.
%

% * String length bug: (reported to tex-implementors, 12/95)
% 
%   - if buggy, a string consisting of a single tab char |"^^I"| 
%     is reported to have length 3 instead of length 1

def show_length(expr s) =
  message("Length of string " & ditto & s & ditto);
  message("==> " & decimal(length(s)));
enddef;

message("* Test for string length bug:");

string s; s=char(9);
show_length(s);

string t; t="	"; % <-- tab char
show_length(t);


% * 8-bit chars in |btex ... etex|: (metafont@ens.fr, 11/95)
%
%   - if buggy, MP will complain about "Invalid character"
%
% (unfortunately can't assume dcr1000 is available; punt for now --karl.)

%message("* Test for 8-bit chars in btex...etex:");
%verbatimtex \font\dcr=dcr1000 etex;
%picture p;
%p = btex \dcr dv|DV\ etex; % <--  \"a\"o\"u\"A\"O\"U


% * big numbers in |btex ... etex|: (metafont@ens.fr, 10/96)
%
%   - if buggy, MP will complain about "Number too large (4096)"

message("* Test for big numbers in btex...etex:");
picture q;
q = btex $2^{12} = 4096 \qquad 2^{15} = 32768$ etex;

end.
