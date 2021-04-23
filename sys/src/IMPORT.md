<img src="https://harvey-os.org/images/harvey-os-logo.svg" alt="Harvey" width="200px"/>

# Bringing new code into Harvey

This document outlines how to import code from other Plan 9 projects into Harvey.

## Background
Harvey was based on a release of a branch of Plan 9 that
had been released under GPL 2 by UC Berkeley. This source was in turn
based on the "9k" version of Plan 9, which was funded by the US DOE
for supercomputers from 2005-2011. This work included the amd64 port,
the kenc toolchain for amd64, and the ports to various IBM Blue Gene
supercomputers. The final iteration of this work was the NIX kernel,
which is why Harvey has native 2MiB pages.

Harvey has many improvements over Plan 9. Harvey is standard C, and
compiles with both GCC and Clang.  Part of this change brings the move
to register parameters, which was recently reported to provide an 8%
performance improvement in Go.

Harvey supports two system call ABIs: the original Plan 9 ABI
(parameters on stack); and the standard ABI used by standard
toolchains (parameters in registers).  This new system call ABI has
similar advantages over the stack-based one as mentioned above.

The problem with our code base was that it was frozen in time. It was
not possible to bring in improvements from the last ten years because,
until March 2021, the rest of Plan 9 was still released under the
Lucent Public License, which was not compatible.

Now that the Plan 9 Foundation owns the Plan 9 code, and has
rereleased under the MIT license, we can import improved code.

That code must be transformed to compile in Harvey. To ease this path,
we lay out some ground rules.

## Importing code

When importing code, there are a few key goals

* Do not break Harvey
* Ensure the license is compatible
* Properly document the license in source tree
* Continously improve the scripts to future imports are easier

## Ensure the license is compatible

Before you take a single step, double check the license. MIT is OK;
GPL V2 is OK; GPL V3 is not OK; Apache 2 is not OK; LPL is not OK.  If
you are not sure, ask on Slack.

## Do not break Harvey

Never replace working code with new code until you are sure the new
code works.  Recently, as an experiment, we replaced partially broken
USB code with new code.  At that point, all of USB was broken. Bad
idea.

Breaks things, so not so good: https://github.com/Harvey-OS/harvey/pull/1106

Does not work, but does not replace anything: https://github.com/Harvey-OS/harvey/pull/1110

So when you bring in new code, if you plan to replace things, be ready
to prove it does not break anything.

## Properly document the license in source tree

The new USB code in https://github.com/Harvey-OS/harvey/pull/1110 is
MIT licensed. Note that as part of the import of code we also include
the license:
https://github.com/rminnich/harvey/blob/dd32b924238dc487ac4aa89e8e38bc32c87fe884/sys/src/cmd/legacyusb/LICENSE

If you move a subdirectory out from under the directory containing
this license, be sure to put that LICENSE file in the subdirectory.

E.g., should you move /sys/src/cmd/legacyusb/x/ to /sys/src/lib/x, you
should add the Plan 9 Foundation LICENSE file to /sys/src/lib/x.

## Continously improve the scripts so future imports are easier

We'll need to track improvements to the code we import. That means
we'll need to make import easy and automated. Use the scripts. Fix the
scripts.

Scripts should fix includes; types; and function calls. Be sure to
read and understand sys/src/spatches/. Fix that too.

## Make bisects easy: commit early, commit often.

Do NOT bundle up a giant set of fixes in one giant commit. Make
bisection easy!

## Once it's working, work to replace harvey code.

Once the new code is working, replace the old code. Preserve licenses.

Do this replacement with an individual git commit, not as part of a
set of fixes. It should be trivial to revert the replacement commit
for bisecting.

## Improve this document

As we learn how to do this, we need to improve this document. Please
do so.
