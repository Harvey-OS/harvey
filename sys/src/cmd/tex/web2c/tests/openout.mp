tracingonline := -1;
write "hello" to "xfoo.mut";
write EOF to "xfoo.mut";

% This crashed mp in 6.96 for Ulrik, probably from openoutnameok?
% Also problems with docstrip.
% (Except his filenames didn't have the x's.)
write "test" to "xopeonout";
write EOF to "xopenout";
write "test" to "./xopenout";

bye.
