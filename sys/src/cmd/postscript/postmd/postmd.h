/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 *
 * An interval list used to map matrix elements into integers in the range 0 to
 * 254 representing shades of gray on a PostScript printer. The list can be given
 * using the -i option or can be set in the optional header that can preceed each
 * matrix. The list should be a comma or space separated list that looks like,
 *
 *		num1,num2, ... ,numn
 *
 * where each num is a floating point number. The list must be given in increasing
 * numerical order. The n numbers in the list partion the real line into 2n+1
 * regions given by,
 *
 *		region1		element < num1
 *		region2		element = num1
 *		region3		element < num2
 *		region4		element = num3
 *		   .		     .
 *		   .		     .
 *		   .		     .
 *		region2n	element = numn
 *		region2n+1	element > numn
 *
 * Every number in a given region is mapped into an integer in the range 0 to 254
 * and that number, when displayed on a PostScript printer using the image operator,
 * prints as a square filled with a gray scale that reflects the integer that was
 * chosen. 0 maps to black and 255 white (that's why 255 is normally omitted).
 *
 * The shades of gray chosen by the program are normally generated automatically,
 * but can be reassigned using the -g option or by including a grayscale line in
 * the optional header. The grayscale list is comma or space separated list of
 * integers between 0 and 255 that's used to map individual regions into arbitray
 * shade of gray, thus overriding the default choice made in the program. The list
 * should look like,
 *
 *		color1,color2, ... ,color2n+1
 *
 * where color1 applies to region1 and color2n+1 applies to region2n+1. If less
 * than 2n+1 numbers are given the default assignments will be used for the missing
 * regions. Each color must be an integer in the range 0 to 255.
 *
 * The default interval list is given below. The default grayscale maps 254 (almost
 * white) into the first region and 0 (black) into the last.
 *
 */

#define DFLTILIST	"-1,0,1"

/*
 *
 * The active interval list is built from an interval string and stored in an array
 * whose elements are of type Ilist.
 *
 */

typedef struct  {
	double	val;			/* only valid in kind is ENDPOINT */
	int	color;			/* gray scale color */
	long	count;			/* statistics for each region */
} Ilist;

/*
 *
 * Non-integer function declarations.
 *
 */

char	*savestring();
