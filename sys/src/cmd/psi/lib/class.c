#include <u.h>
#include <libc.h>
#include "system.h"
# include "stdio.h"
# include "defines.h"
#include "object.h"
# include "class.h"

unsigned char map[] =
{
ERROR,		/*	nul	*/
ERROR,		/*	soh	*/
ERROR,		/*	stx	*/
ERROR,		/*	etx	*/
ERROR,		/*	eot	*/
ERROR,		/*	enq	*/
ERROR,		/*	ack	*/
ERROR,		/*	bel	*/

ERROR,		/*	bs	*/
WS,		/*	ht	*/
WS,		/*	nl	*/
ERROR,		/*	vt	*/
ERROR,		/*	np	*/
WS,		/*	cr	*/
ERROR,		/*	so	*/
ERROR,		/*	si	*/

ERROR,		/*	dle	*/
ERROR,		/*	dc1	*/
ERROR,		/*	dc2	*/
ERROR,		/*	dc3	*/
ERROR,		/*	dc4	*/
ERROR,		/*	nak	*/
ERROR,		/*	syn	*/
ERROR,		/*	etb	*/

ERROR,		/*	can	*/
ERROR,		/*	em	*/
E_OF,		/*	sub	*/
ERROR,		/*	esc	*/
ERROR,		/*	fs	*/
ERROR,		/*	gs	*/
ERROR,		/*	rs	*/
ERROR,		/*	us	*/

WS,		/*	sp	*/
REG,		/*	!	*/
REG,		/*	"	*/
POUND,		/*	#	*/
REG,		/*	$	*/
PERC,		/*	%	*/
REG,		/*	&	*/
REG,		/*	'	*/

LPAR,		/*	(	*/
REG,		/*	)	*/
REG,		/*	*	*/
SIGN,		/*	+	*/
REG,		/*	,	*/
SIGN,		/*	-	*/
DOT,		/*	.	*/
SLASH,		/*	/	*/

NUMBER,		/*	0	*/
NUMBER,		/*	1	*/
NUMBER,		/*	2	*/
NUMBER,		/*	3	*/
NUMBER,		/*	4	*/
NUMBER,		/*	5	*/
NUMBER,		/*	6	*/
NUMBER,		/*	7	*/

NUMBER,		/*	8	*/
NUMBER,		/*	9	*/
REG,		/*	:	*/
REG,		/*	;	*/
LT,		/*	<	*/
REG,		/*	=	*/
REG,		/*	>	*/
REG,		/*	?	*/

REG,		/*	@	*/
REG,		/*	A	*/
REG,		/*	B	*/
REG,		/*	C	*/
REG,		/*	D	*/
EXP,		/*	E	*/
REG,		/*	F	*/
REG,		/*	G	*/

REG,		/*	H	*/
REG,		/*	I	*/
REG,		/*	J	*/
REG,		/*	K	*/
REG,		/*	L	*/
REG,		/*	M	*/
REG,		/*	N	*/
REG,		/*	O	*/

REG,		/*	P	*/
REG,		/*	Q	*/
REG,		/*	R	*/
REG,		/*	S	*/
REG,		/*	T	*/
REG,		/*	U	*/
REG,		/*	V	*/
REG,		/*	W	*/

REG,		/*	X	*/
REG,		/*	Y	*/
REG,		/*	Z	*/
LBRK,		/*	[	*/
REG,		/*	\	*/
RBRK,		/*	]	*/
REG,		/*	^	*/
REG,		/*	_	*/

REG,		/*	`	*/
REG,		/*	a	*/
REG,		/*	b	*/
REG,		/*	c	*/
REG,		/*	d	*/
EXP,		/*	e	*/
REG,		/*	f	*/
REG,		/*	g	*/

REG,		/*	h	*/
REG,		/*	i	*/
REG,		/*	j	*/
REG,		/*	k	*/
REG,		/*	l	*/
REG,		/*	m	*/
REG,		/*	n	*/
REG,		/*	o	*/

REG,		/*	p	*/
REG,		/*	q	*/
REG,		/*	r	*/
REG,		/*	s	*/
REG,		/*	t	*/
REG,		/*	u	*/
REG,		/*	v	*/
REG,		/*	w	*/

REG,		/*	x	*/
REG,		/*	y	*/
REG,		/*	z	*/
LBRC,		/*	/*	*/
REG,		/*	|	*/
RBRC,		/*	}	*/
REG,		/*	~	*/
ERROR,		/*	del	*/
} ;
