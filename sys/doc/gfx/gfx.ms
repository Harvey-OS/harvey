.TL
Raster Graphics in Plan 9
.AU
Tom Duff
td@plan9.att.com
.NH
Introduction
.PP
Section 9 of the Plan 9 Programmer's Manual describes
a file format for storing raster images
.I picfile (9.6)) (
and a suite of commands and libraries for manipulating them.
.PP
Binaries of the picture manipulation programs are located in the directory
.CW /$cputype/bin/fb .
Examples in this discussion assume that you have run
.P1
bind -b /$cputype/bin/fb /bin
.P2
to merge the
.CW fb
commands with the regular contents of
.CW /bin .
For example, you might put this command in
.CW $home/lib/profile .
Alternatively, you can prefix any
.I program
name from Section 9 with the string
.CW fb/ ,
running it as
.CW fb/\f2program\f1.
.NH
Pictures and picfiles
.PP
For present purposes, a picture is a rectangular array of
.I n -byte
pixels.  A pixel of a full-color picture has three bytes,
encoding the brightness of a CRT's red, green and blue phosphors
at a particular spot on the screen.  A monochrome picture needs only
one byte per pixel to describe its brightness.  In either case,
a byte containing 0 is black and 255 is full intensity.
.PP
As a storage-saving compromise, some pictures are represented
by one-byte pixels with an associated
.I "color map" ,
a table of 256 3-byte entries.  In this case the pixel values
are used as indices to look up 3-byte colors in the color map.
.PP
The bounding rectangle
.I x0\f1,\f2y0\f1,\f2x1\f1,\f2y1 ) (
of a picture is represented as in the
.I graphics (2)
section of the Programmer's Manual.
(See the figure.)
That is,
.I x0\f1,\f2y0 ) (
is the coordinate of the upper-leftmost
pixel of the picture and
.I x1\f1-1,\f2y1 -1) (
is the coordinate of the lower-rightmost pixel.
.I X
increases from left to right, and
.I Y
increases from top to bottom.
Thus
.I x0\f1≤\f2x1
and
.I y0\f1≤\f2y1 .
Pixels in the order
they are scanned out on the CRT face
are in row-major order in the pixel array.
.PS
box wid 2 ht 1.4 at (4.2,6.8);
line from (3.2,7.3) to (5.2,7.3);
line from (3.2,7.1) to (4.63,7.1);
line from (3.2,6.9) to (4.26,6.9);
line from (3.2,6.7) to (4.08,6.7);
line from (3.2,6.5) to (3.72,6.5);
line from (3.2,6.3) to (3.54,6.3);
line from (3.4,7.5) to (3.4,6.1);
line from (3.6,7.5) to (3.6,6.42);
line from (3.8,7.5) to (3.8,6.45);
line from (4,7.5) to (4,6.82);
line from (4.2,7.5) to (4.2,7.04);
line from (4.4,7.5) to (4.4,7.22);
line from (4.6,7.5) to (4.6,7.23);
line from (4.8,7.5) to (4.8,7.24);
line from (5,7.5) to (5,7.18);
"(\fIx0,y0\fP)" at (2.435,7.57);
line -> from (2.75,7.57) to (3.3,7.4);
line from (5.2,6.3) to (4.75,6.3);
line from (5,6.1) to (5,6.4);
line -> from (5.4,6) to (5.1,6.2);
"(\fIx1\fP-1,\fIy1\fP-1)" at (5.895,6);
line -> from (3.2,7.7) to (4.01,7.7);
"increasing \fIx\fP" at (4.55,7.7);
line -> from (5.4,7.5) to (5.4,6.8);
"increasing \fIy\fP" at (5.94,6.71);
.PE
.PP
The picture file format described in
.I picfile (9.6)
is fairly simple.  Such a
.I picfile
encodes
a rectangular array of
.I n -channel
pixel records, each channel being a single byte.
The file contains a textual header
that describes the dimensions and encoding
of the picfile.  At the end of the header is
an empty line \(em two newline characters
in succession.  Following the header is a
binary encoding (possibly compressed) of the
pixel data in scan-line order.
The header may indicate that the picfile has
a color map, in which case a burst of 256
3-byte records separates the header from the
pixel data.
.PP
The lines of the picfile header are
.I attribute\f(CW=\fPvalue
pairs.  The value of the
.CW WINDOW=
attribute specifies the dimensions of the
picfile, and the
.CW CHAN=
attribute names the channels.
Most often you will see
.CW CHAN=rgb
for full-color pictures or
.CW CHAN=m
for monochrome or color mapped pictures.
Some pictures have
.CW CHAN=rgba .
These are full color with an α channel.
Most of the objects that we make pictures of are not rectangular,
contrary to the
.CW WINDOW=
attribute.  The α channel gives us a way of describing a picture's
shape.
Think of α as a fraction between 0 and 1 (represented by 255)
that indicates whether the picture covers the pixel or not.
Fractional α values indicate pixels in which the background should
shine through, because the foreground is translucent or only partly
covers the pixel.
The paper
``Compositing digital images'', by Thomas Porter and Tom Duff
(1984 SIGGRAPH Proceedings, pp. 253-258),
describes how α is used to control
anti-aliased picture compositing operations.
.PP
Here is a sample image, called
.CW pjw .
.BP pjw.ps  1.5 5.0 c
.LP
This is its header:
.P1
TYPE=runcode
WINDOW=0 0 320 240
NCHAN=1
CHAN=m
COMMAND=resample 320 pjw
COMMAND=transpose
COMMAND=resample 240
COMMAND=transpose
COMMAND=xpand pjw 0 255 0 251
.P2
The
.CW COMMAND=
lines were inserted by the various programs
used to create the file.  It was resampled to 320×240 resolution by
the pipeline
.P1
resample 320 pjw | transpose | resample 240 | transpose
.P2
and had its contrast adjusted by
.CW xpand .
.CW Resample ,
.CW transpose ,
and
.CW xpand
are discussed below.
.PP
The
.CW picinfo
command allows you to examine a picfile's header, but
.P1
sed '/^$/q' \fIfile\fP
.P2
works just as well.
.PP
The
.CW hist
and
.CW bbox
commands extract other interesting information from picfiles.
.CW Hist
prints a histogram of a picfile's pixel values, and
.CW bbox
prints the bounding rectangle of the non-zero
(or, when given an appropriate flag, any other value)
pixels of a picfile.
.PP
All commands that read or write picture files understand the names
.CW IN
and
.CW OUT
to be synonyms for standard input and output.
.NH
Displays
.PP
Plan 9 terminals have a CRT that displays the contents of a
.I "frame buffer" ,
a large memory containing a rectangular array of pixels.
Most Plan 9 terminals have one, two or eight-bit pixels.
In Plan 9, one-bit pixels are white when zero and black when one.
Two-bit pixels may take on four shades of grey, with white again
corresponding to zero.  Eight-bit frame buffers generally have
a color map.
.PP
The
.CW "colors [-gfr]
command creates a small window and fills it with a 16 by 16 array of squares,
each of a different color in the color map.
Pixel value zero is in the upper left-hand corner and colors
increase by ones across each row.
.PP
The color map can be loaded from a file by the command
.CW getmap
.I file .
.CW Getmap
searches for files in the current directory and in
.CW /lib/fb/cmap .
A color map
file contains 256 3-byte records.  The command
.P1
getmap bw
.P2
loads a color map containing 256 shades of grey, effectively
converting the terminal from color to monochrome, while
.P1
getmap 9
.P2
gets the Plan 9 default color map, loaded by
the kernel at boot time and expected by many programs.
.PP
The
.CW 9v
command creates a new window and displays a
picture file in it.  If the picture contains a color map,
.CW 9v
loads it into the display.
If the picture is full color (24 bits per pixel),
.CW 9v
converts it to monochrome before displaying it.
.PP
Non-Plan 9 systems support innumerable other picture
file formats.
.CW 9v
can decipher pictures in many foreign formats \(em it
uses their file names as a clue to the format, as in
this table:
.TS
center box;
l l.
Name	Format
_
\f(CW*.gif\fR	Compuserve GIF format
\f(CW*.ega\fR	IBM-PC EGA dump
\f(CW*.face\fR	USENIX facesaver format
\f(CW*.jpeg\fR	\fIjfif\fP-format \fIjpeg\fR
\f(CW*.jpg\fR	\fIjfif\fP-format \fIjpeg\fR
\f(CW*.pcx\fR	Paintbrush \fIpcx\fP format
\f(CW*.rle\fR	University of Utah Run-Length Encoded
\f(CW*.sgi\fR	Silicon Graphics image file
\f(CW*.tga\fR	Truevision TARGA format
\f(CW*.tif\fR	TIFF (Tagged Image File Format)
\f(CW*.tiff\fR	TIFF (Tagged Image File Format)
\f(CW*.xbm\fR	X Window System bitmap
.TE
.PP
Depending on the name of a file as a guide to its image format can be unreliable.  The
.CW cvt2pic
command recognizes foreign image files by their contents and
converts them to picture files, so
.P1
cvt2pic \fIfile\fP | 9v
.P2
may succeed where a naked
.CW 9v
command would fail.
There's much more to know about converting image file formats.
Look at
.I cvt2pic (9.1)
for more detail and
.I pic2ps (9.1)
for a token nod at converting picfiles into formats that may
be useful elsewhere.
.PP
.CW 9v 's
conversion of full-color pictures to monochrome
is often inappropriate.  In that case, the
.CW 3to1
command is a useful intermediary.  Its two arguments
are the name of a color map and a picfile (default
standard input).  It outputs an approximation of its input
picture using the given color map.  So, a full-featured
thunderclap to view a color picture in a foreign format is
.P1
cvt2pic \fIfile\fP | 3to1 9 | 9v
.P2
Beware \(em there's a limit to how well
.CW 3to1
can approximate its input.  In particular, if the
original file was a GIF or EGA-format
picture, it already had 8-bit pixels and a color map,
so running it through
.CW 3to1
wouldn't improve things and may very well make them much worse.
.NH
Picfile manipulation
.PP
Often you will need to convert a picture to have a particular size, or
with a particular set of channels.  The
.CW pcp
(picfile copy) command will often do the job.  For example,
.P1
pcp -w 0 70 320 125 pjw eyes
.P2
copies
.CW pjw
to
.CW eyes .
It copies only pixels in the window (0,70,320,125).
This is the result:
.BP eyes.ps 0.35 5.0 c
Generally,
.CW pcp
lets you select any window and any set of channels (in any order)
from the input picture and rename the channels arbitrarily.
It will use the NTSC luminance formula,
.CW m=.299×r+.587×g+.114×b ,
to synthesize
.CW CHAN=m
from
.CW CHAN=rgb
and vice versa.  Read
.I pcp (9.1)
for more details.
.PP
Several commands can alter a picfile's geometry in more complicated ways.
The
.CW resample
command will arbitrarily adjust the width of a picture by optimal anti-aliased resampling.
It does a very good job (the best possible, in some precise sense), but it
only works horizontally.  To resample vertically, you can use it in
conjunction with
.CW transpose ,
which, unsurprisingly, computes a picfile's transpose.
This pipeline rescales a picfile in both directions:
.P1
resample \fIxsize\fP \fIfile\fP | transpose | resample \fIysize\fP | transpose
.P2
The
.CW transpose
command has flags that will make it rotate multiples of 90° or mirror-reflect
a picture vertically or horizontally \(em the
.I transpose (9.1)
page in the Programmer's Manual gives more details, and also describes
.CW rotate ,
which rotates pictures by arbitrary angles.
.I Pdup (9,1)
describes simpler and faster program that magnifies by duplicating pixels, giving a
blocky appearance to its output.  For example,
.P1
pcp -w 120 80 153 100 pjw | pdup 8 8
.P2
produces this output:
.BP pdup.ps 1.5 5.0 c
.NH
Creating new images
.PP
The commands described so far view pictures, convert between formats, and adjust
their shapes and configurations in simple ways.  The following commands create
new pictures, either from whole cloth or by modifying and combining existing pictures.
.PP
The
.CW card
command outputs a picture file all of whose pixels are the same color.
The
.CW ramp
command makes slightly more exciting pictures that blend between two colors from
edge to edge.  For example,
.P1
ramp -v -w 0 0 320 240
.P2
creates a 320×240 vertical ramp:
.BP ramp.ps 1.5 5.0 c
.PP
There are three categories of program that transform a single image:
those that operate pointwise (that is, the output pixel values depend only on
the corresponding input pixel), `neighborhood' operations, for which the output
is some combination of several pixels surrounding the corresponding input pixel,
and half-toning or color-quantization programs.
.PP
Pointwise operations tweak the intensity or color of the input picture.
The
.CW xpand
command can expand or compress the range of pixel values, adjusting the picture's
contrast.  By default, it expands the range of pixel values in its input, mapping the
lowest input pixel value to zero and the highest to 255.  
Optionally, the range of output and/or input pixel values can be specified on the
command line.  Thus,
.P1
xpand pjw 255 0 0 255
.P2
interchanges black and white, producing a negative image:
.BP neg.ps 1.5 5.0 c
.CW He
(an abbreviation of Histogram Equalization) evens out its input's distribution
of pixel values, making it as even as possible.
The
.CW cmap
command maps its input's pixel values through a color map.  It works on
an original that has
.CW CHAN=rgb ,
and uses the input pixels' red, green and blue channels to index the
red, green and blue of the color map.
For example,
.P1
pcp -crgb pjw OUT | cmap 5.oclock.shado
.P2
produces this output:
.BP cmap.ps 1.5 5.0 c
Since
.CW pjw
is a
.CW CHAN=m
picture, we must use
.CW pcp
to convert it to
.CW CHAN=rgb
before passing it through
.CW cmap .
.PP
.CW Remap
tries to be the inverse of
.CW cmap .
It may not succeed because the
.CW cmap
mapping may not be invertible.
Its output has
.CW rgb
set to indices of the color map entries that
best approximate the input's
.CW rgb .
.PP
The
.I filters (9.1)
manual page describes a group of programs that operate on neighborhoods.
These include operations for blurring or sharpening images,
adaptively adjusting their contrast, detecting or enhancing edges, and
removing or exaggerating noise.
.PP
.CW Adapt
does adaptive contrast enhancement.  It finds the minimum and maximum
values in a 7×7 window around each pixel and
remaps the center pixel using the linear function that sends the minimum to zero
and the maximum to 255.  The result of
.CW "adapt pjw
is
.BP adapt.ps 1.5 5.0 c
.PP
.CW Ahe
does adaptive histogram equalization.  In 17×17 windows it counts the number of pixels
whose value is less than the center pixel, counting ½ for each pixel equal to the center
value.  The output is just the count scaled to be between 0 and 255.  For example,
.CW "ahe pjw"
produces this result:
.BP ahe.ps 1.5 5.0 c
.PP
.CW Crispen
and
.CW laplace
are high-pass filters that sharpen edges.
.CW Crispen
is more extreme than
.CW laplace .
Here is the output of
.CW "crispen pjw" :
.BP crispen.ps 1.5 5.0 c
.PP
.CW Edge ,
.CW edge2 ,
and
.CW edge3
are all edge-detecting filters.
.CW Edge2
usually produces the best results.  The output of
.CW "edge2 pjw"
is
.BP edge2.ps 1.5 5.0 c
Combining the output of an edge detection operator with the original image
enhances the edges.
.P1
edge2 pjw | lerp IN .3 pjw
.P2
produces this result:
.BP enhance.ps 1.5 5.0 c
.PP
In this example the
.CW lerp
command interpolates pixel values linearly between the two images.
The first image's pixels are multiplied by the coefficient .3 and
added to .7 (that is 1-.3) times the second.
.PP
.CW Median
and
.CW nonoise
are noise-reduction filters.
They try to reduce the amplitude of random signals without affecting
the underlying image.
.CW Smooth
low-pass filters the image, blurring all the details.  For example,
.P1
smooth pjw | smooth | smooth
.P2
produces this result:
.BP smooth.ps 1.5 5.0 c
.PP
The limited color resolution of many displays, and particularly of
hard-copy output devices, makes reducing the number of colors used
in an image a popular and complicated topic.
The
.I floyd (9.1)
page of the Programmer's Manual is devoted to programs that
reduce grey-scale images to one bit per pixel.  The best of these
is probably
.CW floyd .
The output of
.CW "floyd pjw
is
.BP floyd.ps 1.5 5.0 c
.PP
The
.I quantize (9.1)
page describes programs that try to reduce full-color (24 bit-per-pixel) pictures
to 8 bits per pixel.
.NH
Composite images
.PP
The
.I lam (9.1)
page describes four commands that read multiple picture files and paste them
together in different ways.
The
.CW piccat
and
.CW picjoin
commands conjoin their inputs top-to-bottom and left-to-right.
The
.CW lam
command overlays its inputs so that their coordinate systems match;
.CW posit
does likewise, but uses its inputs' α channels to let background images show through.
For example
.P1
pcp -crgba pjw OUT | posit 9ball IN
.P2
produces this result
.BP 9ball.ps 2.0 5.0 c
.PP
.CW Pjw
must first be passed through
.CW pcp
because
.CW posit
expects
.CW CHAN=
attributes of all its inputs to match.
