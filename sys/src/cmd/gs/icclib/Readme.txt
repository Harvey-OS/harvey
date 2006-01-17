ICC profile I/O library (icclib), README file

Date 2002/4/22, Version 2.02

This distribution contains source code which implements the reading and
writing of color profile files that conform to the International Color
Consortium (ICC) Profile Format Specification, Version 3.4.

For more information about the ICC, and for copies of the specification,
please refer to http://www.color.org/.

(Note that this software is written from the ICC V3.4 standard, but the
software and its author are not affiliated with, or otherwise connected
with the ICC.)

The ICC profile I/O library archive is kept at
http://web.access.net.au/argyll/color.html

(Version 2.02 is has a few minor changes and fixups from V2.00.
See icc.c for a more detailed change history.) 

Motivation

Color is still very much a black art to many programmers dealing with
computer graphics. The ICC Profile Format is an industry attempt to provide
an interchange format to help solve the problems of specifying color, and
in transferring color graphics from, and between systems and devices.
Although the ICC format has been around a number of years, and has long
been adopted by companies in the business of providing systems for
publishing and printing, and is now widely used as part of commercial
operating system support for device independent color, its uptake in the
general world of computer graphics has been slow.

The writing of this library was prompted by my private and professional
enthusiasm for computer graphics, and color. Inspired by other examples of
freely usable software (notably the Independent JPEG Group's free JPEG
software, and Sam Leffler's TIFF library), I have decided to make this
library available under similar terms.  I hope that this library will
provide a starting point for including ICC profile support more widely that
is currently the case, particularly in open source code projects.

Overview

This package contains a C software implementation of the ICC Profile
Format, Version 3.4. The ICC Profile Format attempts to provide a
cross-platform device profile format, that can be used to translate color
data created on one device into another device's native color space. For a
fuller explanation of what the ICC Profile Format is all about, please
refer to http://www.color.org, and the profile specification.

In summary this library provides:

   * Full source code, free for commercial and non-commercial use.
   * Support for all version 3.4 header elements, Tags and Tag Types.
   * Conversion to/from machine native representation of all data
     types.
   * Support for user defined Tags.
   * Support for adding/deleting Tags.
   * Support for Tag type sharing within a file (often used for
     sharing LUTs amongst intents).
   * Support for reading/writing embedded profiles, including
     from/to a memory buffer, rather than a file.
   * Provides a single function for transforming color values through
     a profile, including support for intents, forward and reverse
     transforms, gamut lookup or preview lookup.
   * Provides support and code examples for creating all profile
     types, monochrome, matrix and Lut.
   * Attempts to be platform neutral.
   * Loads Tag Types on demand to conserve memory space.

Changes from V1.30

 Change absolute conversion to be white point only, and use
 Bradford transform by default. (ie. we are now ignoring the
 comment in section 6.4.22 of the 1998 spec. about the
 media black point being used for absolute colorimetry,
 ignoring the recommendation on page 118 in section E.5,
 and are taking up the recommendation on page 124 in section
 E.16 that a more sophisticated chromatic adaption model be used.)

 This is for better compatibility with other CMM's, and to
 improve the results when using simple links between
 profiles with non-D50 white points. Standard profiles
 like sRGB will also be more accurate when interpreted
 with absolute colorimetric intent.
 This will cause some slight profile inaccuracy when used
 with profile created with previous versions of icclib.

 Added file I/O class to allow substitution of alternative ICC profile
 file access. Provide standard file class instance, and memory image
 instance of file I/O class as default and example. 
 Added an optional new_icc_a() object creator, that takes a memory
 allocator class instance. This allows an alternate memory heap to
 be used with the icc class. 
 Renamed object free() methods to del() for more consistency with new().

 Added ColorSync 2.5 specific VideoCardGamma tag support (from Neil Okamoto)

Package contents:

 icclib.zip ZIP archive of the following files
 README    This file.
 Licence.txt Important! - Permissions for use of this package.
 icc.c     Library source file.

 icc.h     Library include file. Note machine dependent defines. Includes
           icc9809.h.
 icc9809.h Lightly modified standard ICC header file.
 iccdump.c Program that dumps ASCII description of a profile.

 icclu.c   Program that allows interactive or batch translation of color
           values though a profile.
 icctest.c Basic library tag Read/Write example and regression test code.

 lutest.c  Color lookup regression test code, and example for creating
           color profiles.
 iccrw.c   Source code skeleton for reading and then re-writing a profile.
 jamfile   JAM style "makefile" see http://www.perforce.com/jam/jam.html
 makefile  UNIX style makefile. Modify this to suite your system.


Style

For handling convenience, I have included all the library source code in
two files. The down side is that they are both hard to read and navigate
through. The code could do with some cleaning up and rearrangement, to make
clear the distinction between public and private elements. (C++ would help
here, but is less portable.) The code attempts to be ANSI C compliant,
written in an object oriented style. Unfortunately, it has not been tested
on a wide variety of platforms, nor with a very wide set of color profiles,
so there my well be a number of bugs to discover. A tutorial on how to use
the library would also be a good thing !

The best way to learn how to use the library, is to take a look at
icctest.c, lutest.c and iccrw.c. The first is used to test writing and
reading to every type of element, with every possible variation of usage.
You will need a copy of the ICC spec. handy to understand what it all
means. The second source file specifically creates and then tests various
types of profiles, including monochrome, matrix and Lut style profiles. The
last is a source code skeleton, that reads a profile completely into
memory, and then writes it out again to a different file.

With the release of version 2.02 of icclib, the library is now as useful as
it is likely to be, allowing convenient color conversion between PCS
(profile connection spaces, either XYZ or Lab) and device specific color
spaces. The library does not attempt to be a complete color management
system however, lacking profile creation and linking functionality.
(The Argyll CMS provides full CMS functionality.)

I welcome feedback, positive or negative, so please mail me at
GraemeGill@access.net.au

Graeme Gill
