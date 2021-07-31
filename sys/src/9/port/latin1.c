#include "u.h"

struct latin
{
	Rune	l;
	uchar	c1;
	uchar	c2;
}latintab[] = {
	L'¡',	'!','!',	/* spanish initial ! */
	L'¢',	'c','$',	/* cent */
	L'£',	'l','$',	/* pound sterling */
	L'¤',	'g','$',	/* general currency */
	L'¥',	'y','$',	/* yen */
	L'¦',	'|','|',	/* broken vertical bar */
	L'§',	'S','S',	/* section symbol */
	L'¨',	'\"','\"',	/* dieresis */
	L'©',	'c','O',	/* copyright */
	L'ª',	's','a',	/* super a, feminine ordinal */
	L'«',	'<','<',	/* left angle quotation */
	L'¬',	'n','o',	/* not sign, hooked overbar */
	L'­',	'-','-',	/* soft hyphen */
	L'®',	'r','O',	/* registered trademark */
	L'¯',	'_','_',	/* macron */
	L'°',	'd','e',	/* degree */
	L'±',	'+','-',	/* plus-minus */
	L'²',	's','2',	/* sup 2 */
	L'³',	's','3',	/* sup 3 */
	L'´',	''',''',	/* acute accent */
	L'µ',	'm','i',	/* micron */
	L'¶',	'p','g',	/* paragraph (pilcrow) */
	L'·',	'.','.',	/* centered . */
	L'¸',	',',',',	/* cedilla */
	L'¹',	's','1',	/* sup 1 */
	L'º',	's','o',	/* super o, masculine ordinal */
	L'»',	'>','>',	/* right angle quotation */
	L'¼',	'1','4',	/* 1/4 */
	L'½',	'1','2',	/* 1/2 */
	L'¾',	'3','4',	/* 3/4 */
	L'¿',	'?','?',	/* spanish initial ? */
	L'À',	'`','A',	/* A grave */
	L'Á',	''','A',	/* A acute */
	L'Â',	'^','A',	/* A circumflex */
	L'Ã',	'~','A',	/* A tilde */
	L'Ä',	'\"','A',	/* A dieresis */
	L'Å',	'o','A',	/* A circle */
	L'Æ',	'A','E',	/* AE ligature */
	L'Ç',	',','C',	/* C cedilla */
	L'È',	'`','E',	/* E grave */
	L'É',	''','E',	/* E acute */
	L'Ê',	'^','E',	/* E circumflex */
	L'Ë',	'\"','E',	/* E dieresis */
	L'Ì',	'`','I',	/* I grave */
	L'Í',	''','I',	/* I acute */
	L'Î',	'^','I',	/* I circumflex */
	L'Ï',	'\"','I',	/* I dieresis */
	L'Ð',	'D','-',	/* Eth */
	L'Ñ',	'~','N',	/* N tilde */
	L'Ò',	'`','O',	/* O grave */
	L'Ó',	''','O',	/* O acute */
	L'Ô',	'^','O',	/* O circumflex */
	L'Õ',	'~','O',	/* O tilde */
	L'Ö',	'\"','O',	/* O dieresis */
	L'×',	'm','u',	/* times sign */
	L'Ø',	'/','O',	/* O slash */
	L'Ù',	'`','U',	/* U grave */
	L'Ú',	''','U',	/* U acute */
	L'Û',	'^','U',	/* U circumflex */
	L'Ü',	'\"','U',	/* U dieresis */
	L'Ý',	''','Y',	/* Y acute */
	L'Þ',	'|','P',	/* Thorn */
	L'ß',	's','s',	/* sharp s */
	L'à',	'`','a',	/* a grave */
	L'á',	''','a',	/* a acute */
	L'â',	'^','a',	/* a circumflex */
	L'ã',	'~','a',	/* a tilde */
	L'ä',	'\"','a',	/* a dieresis */
	L'å',	'o','a',	/* a circle */
	L'æ',	'a','e',	/* ae ligature */
	L'ç',	',','c',	/* c cedilla */
	L'è',	'`','e',	/* e grave */
	L'é',	''','e',	/* e acute */
	L'ê',	'^','e',	/* e circumflex */
	L'ë',	'\"','e',	/* e dieresis */
	L'ì',	'`','i',	/* i grave */
	L'í',	''','i',	/* i acute */
	L'î',	'^','i',	/* i circumflex */
	L'ï',	'\"','i',	/* i dieresis */
	L'ð',	'd','-',	/* eth */
	L'ñ',	'~','n',	/* n tilde */
	L'ò',	'`','o',	/* o grave */
	L'ó',	''','o',	/* o acute */
	L'ô',	'^','o',	/* o circumflex */
	L'õ',	'~','o',	/* o tilde */
	L'ö',	'\"','o',	/* o dieresis */
	L'÷',	'-',':',	/* divide sign */
	L'ø',	'/','o',	/* o slash */
	L'ù',	'`','u',	/* u grave */
	L'ú',	''','u',	/* u acute */
	L'û',	'^','u',	/* u circumflex */
	L'ü',	'\"','u',	/* u dieresis */
	L'ý',	''','y',	/* y acute */
	L'þ',	'|','p',	/* thorn */
	L'ÿ',	'\"','y',	/* y dieresis */
	L'♔',	'w','k',	/* chess white king */
	L'♕',	'w','q',	/* chess white queen */
	L'♖',	'w','r',	/* chess white rook */
	L'♗',	'w','b',	/* chess white bishop */
	L'♘',	'w','n',	/* chess white knight */
	L'♙',	'w','p',	/* chess white pawn */
	L'♚',	'b','k',	/* chess black king */
	L'♛',	'b','q',	/* chess black queen */
	L'♜',	'b','r',	/* chess black rook */
	L'♝',	'b','b',	/* chess black bishop */
	L'♞',	'b','n',	/* chess black knight */
	L'♟',	'b','p',	/* chess black pawn */
	L'α',	'*','a',	/* alpha */
	L'β',	'*','b',	/* beta */
	L'γ',	'*','g',	/* gamma */
	L'δ',	'*','d',	/* delta */
	L'ε',	'*','e',	/* epsilon */
	L'ζ',	'*','z',	/* zeta */
	L'η',	'*','y',	/* eta */
	L'θ',	'*','h',	/* theta */
	L'ι',	'*','i',	/* iota */
	L'κ',	'*','k',	/* kappa */
	L'λ',	'*','l',	/* lambda */
	L'μ',	'*','m',	/* mu */
	L'ν',	'*','n',	/* nu */
	L'ξ',	'*','c',	/* xsi */
	L'ο',	'*','o',	/* omicron */
	L'π',	'*','p',	/* pi */
	L'ρ',	'*','r',	/* rho */
	L'ς',	't','s',	/* terminal sigma */
	L'σ',	'*','s',	/* sigma */
	L'τ',	'*','t',	/* tau */
	L'υ',	'*','u',	/* upsilon */
	L'φ',	'*','f',	/* phi */
	L'χ',	'*','x',	/* chi */
	L'ψ',	'*','q',	/* psi */
	L'ω',	'*','w',	/* omega */	
	L'Α',	'*','A',	/* Alpha */
	L'Β',	'*','B',	/* Beta */
	L'Γ',	'*','G',	/* Gamma */
	L'Δ',	'*','D',	/* Delta */
	L'Ε',	'*','E',	/* Epsilon */
	L'Ζ',	'*','Z',	/* Zeta */
	L'Η',	'*','Y',	/* Eta */
	L'Θ',	'*','H',	/* Theta */
	L'Ι',	'*','I',	/* Iota */
	L'Κ',	'*','K',	/* Kappa */
	L'Λ',	'*','L',	/* Lambda */
	L'Μ',	'*','M',	/* Mu */
	L'Ν',	'*','N',	/* Nu */
	L'Ξ',	'*','C',	/* Xsi */
	L'Ο',	'*','O',	/* Omicron */
	L'Π',	'*','P',	/* Pi */
	L'Ρ',	'*','R',	/* Rho */
	L'Σ',	'*','S',	/* Sigma */
	L'Τ',	'*','T',	/* Tau */
	L'Υ',	'*','U',	/* Upsilon */
	L'Φ',	'*','F',	/* Phi */
	L'Χ',	'*','X',	/* Chi */
	L'Ψ',	'*','Q',	/* Psi */
	L'Ω',	'*','W',	/* Omega */
	L'←',	'<','-',	/* left arrow */
	L'↑',	'u','a',	/* up arrow */
	L'→',	'-','>',	/* right arrow */
	L'↓',	'd','a',	/* down arrow */
	L'↔',	'a','b',	/* arrow both */
	L'⇐',	'V','=',	/* left double-line arrow */
	L'⇒',	'=','V',	/* right double-line arrow */
	L'∀',	'f','a',	/* forall */
	L'∃',	't','e',	/* there exists */
	L'∂',	'p','d',	/* partial differential */
	L'∅',	'e','s',	/* empty set */
	L'∆',	'D','e',	/* delta */
	L'∇',	'g','r',	/* gradient */
	L'∈',	'm','o',	/* element of */
	L'∉',	'!','m',	/* not element of */
	L'∍',	's','t',	/* such that */
	L'∗',	'*','*',	/* math asterisk */
	L'∙',	'b','u',	/* bullet */
	L'√',	's','r',	/* radical */
	L'∝',	'p','t',	/* proportional */
	L'∞',	'i','f',	/* infinity */
	L'∠',	'a','n',	/* angle */
	L'∧',	'l','&',	/* logical and */
	L'∨',	'l','|',	/* logical or */
	L'∩',	'c','a',	/* intersection */
	L'∪',	'c','u',	/* union */
	L'∫',	'i','s',	/* integral */
	L'∴',	't','f',	/* therefore */
	L'≃',	'~','=',	/* asymptotically equal */
	L'≅',	'c','g',	/* congruent */
	L'≈',	'~','~',	/* almost equal */
	L'≠',	'!','=',	/* not equal */
	L'≡',	'=','=',	/* equivalent */
	L'≦',	'<','=',	/* less than or equal */
	L'≧',	'>','=',	/* greater than or equal */
	L'⊂',	's','b',	/* proper subset */
	L'⊃',	's','p',	/* proper superset */
	L'⊄',	'!','b',	/* not subset */
	L'⊆',	'i','b',	/* reflexive subset */
	L'⊇',	'i','p',	/* reflexive superset */
	L'⊕',	'O','+',	/* circle plus */
	L'⊖',	'O','-',	/* circle minus */
	L'⊗',	'O','x',	/* circle multiply */
	L'⊢',	't','u',	/* turnstile */
	L'⊨',	'T','u',	/* valid */
	L'⋄',	'l','z',	/* lozenge */
	L'⋯',	'e','l',	/* ellipses */
	0,	0,
};

long
latin1(uchar *k)
{
	struct latin *l;

	for(l=latintab; l->l; l++)
		if(k[0]==l->c1 && k[1]==l->c2)
			return l->l;
	return -1;
}

long
unicode(uchar *k)
{
	long i, c;

	k++;	/* skip 'X' */
	c = 0;
	for(i=0; i<4; i++,k++){
		c <<= 4;
		if('0'<=*k && *k<='9')
			c += *k-'0';
		else if('a'<=*k && *k<='f')
			c += 10 + *k-'a';
		else if('A'<=*k && *k<='F')
			c += 10 + *k-'A';
		else
			return -1;
	}
	return c;
}
