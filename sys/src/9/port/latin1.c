#include <u.h>

/*
 * The code makes two assumptions: strlen(ld) is 1 or 2; latintab[i].ld can be a
 * prefix of latintab[j].ld only when j<i.
 */
struct cvlist
{
	char	*ld;		/* must be seen before using this conversion */
	char	*si;		/* options for last input characters */
	Rune	*so;		/* the corresponding Rune for each si entry */
} latintab[] = {
	" ",  " i",		L"␣ı",
	"!~", "-=~",		L"≄≇≉",
	"!",  "!<=>?bmp",	L"¡≮≠≯‽⊄∉⊅",
	"\"*","IUiu",		L"ΪΫϊϋ",
	"\"", "AEIOUY\"aeiouy",	L"ÄËÏÖÜŸ¨äëïöüÿ",
	"$*", "fhk",		L"ϕϑϰ",
	"$",  "BEFHILMoRVaefglpv",	L"ℬℰℱℋℐℒℳℴℛƲɑℯƒℊℓ℘ʋ",
	"'\"","Uu",		L"Ǘǘ",
	"'",  "'ACEILNORSUYZacegilnorsuyz",
				L"´ÁĆÉÍĹŃÓŔŚÚÝŹáćéģíĺńóŕśúýź",
	"*",  "*ABCDEFGHIKLMNOPQRSTUWXYZabcdefghiklmnopqrstuwxyz",
		L"∗ΑΒΞΔΕΦΓΘΙΚΛΜΝΟΠΨΡΣΤΥΩΧΗΖαβξδεφγθικλμνοπψρστυωχηζ",
	"+",  "-O",		L"±⊕",
	",",  ",ACEGIKLNORSTUacegiklnorstu",
				L"¸ĄÇĘĢĮĶĻŅǪŖŞŢŲąçęģįķļņǫŗşţų",
	"-*", "l",		L"ƛ",
	"-",  "+-2:>DGHILOTZbdghiltuz~",
				L"∓­ƻ÷→ÐǤĦƗŁ⊖ŦƵƀðǥℏɨłŧʉƶ≂",
	".",  ".CEGILOZceglz",	L"·ĊĖĠİĿ⊙Żċėġŀż",
	"/",  "Oo",		L"Øø",
	"1",  "234568",		L"½⅓¼⅕⅙⅛",
	"2",  "-35",		L"ƻ⅔⅖",
	"3",  "458",		L"¾⅗⅜",
	"4",  "5",		L"⅘",
	"5",  "68",		L"⅚⅝",
	"7",  "8",		L"⅞",
	":",  "-=",		L"÷≔",
	"<!", "=~",		L"≨⋦",
	"<",  "-<=>~",		L"←«≤≶≲",
	"=",  ":<=>OV",		L"≕⋜≡⋝⊜⇒",
	">!", "=~",		L"≩⋧",
	">",  "<=>~",		L"≷≥»≳",
	"?",  "!?",		L"‽¿",
	"@@",  "'EKSTYZekstyz",	L"ьЕКСТЫЗекстыз",
	"@'",  "'",	L"ъ",
	"@C",  "Hh",	L"ЧЧ",
	"@E",  "Hh",	L"ЭЭ",
	"@K",  "Hh",	L"ХХ",
	"@S",  "CHch",	L"ЩШЩШ",
	"@T",  "Ss",	L"ЦЦ",
	"@Y",  "AEOUaeou",	L"ЯЕЁЮЯЕЁЮ",
	"@Z",  "Hh",	L"ЖЖ",
	"@c",  "h",	L"ч",
	"@e",  "h",	L"э",
	"@k",  "h",	L"х",
	"@s",  "ch",	L"щш",
	"@t",  "s",	L"ц",
	"@y",  "aeou",	L"яеёю",
	"@z",  "h",	L"ж",
	"@",  "ABDFGIJLMNOPRUVXabdfgijlmnopruvx",
				L"АБДФГИЙЛМНОПРУВХабдфгийлмнопрувх",
	"A",  "E",		L"Æ",
	"C",  "ACU",		L"⋂ℂ⋃",
	"Dv", "Zz",		L"Ǆǅ",
	"D",  "-e",		L"Ð∆",
	"G",  "-",		L"Ǥ",
	"H",  "-H",		L"Ħℍ",
	"I",  "-J",		L"ƗĲ",
	"L",  "&-Jj|",		L"⋀ŁǇǈ⋁",
	"N",  "JNj",		L"Ǌℕǋ",
	"O",  "*+-./=EIcoprx",	L"⊛⊕⊖⊙⊘⊜ŒƢ©⊚℗®⊗",
	"P",  "P",		L"ℙ",
	"Q",  "Q",		L"ℚ",
	"R",  "R",		L"ℝ",
	"S",  "S",		L"§",
	"T",  "-u",		L"Ŧ⊨",
	"V",  "=",		L"⇐",
	"Y",  "R",		L"Ʀ",
	"Z",  "-Z",		L"Ƶℤ",
	"^",  "ACEGHIJOSUWYaceghijosuwy",
				L"ÂĈÊĜĤÎĴÔŜÛŴŶâĉêĝĥîĵôŝûŵŷ",
	"_\"","AUau",		L"ǞǕǟǖ",
	"_.", "Aa",		L"Ǡǡ",
	"_,", "Oo",		L"Ǭǭ",
	"_",  "_AEIOUaeiou",	L"¯ĀĒĪŌŪāēīōū",
	"`\"","Uu",		L"Ǜǜ",
	"`",  "AEIOUaeiou",	L"ÀÈÌÒÙàèìòù",
	"a",  "ben",		L"↔æ∠",
	"b",  "()+-0123456789=bknpqru",
				L"₍₎₊₋₀₁₂₃₄₅₆₇₈₉₌♝♚♞♟♛♜•",
	"c",  "$Oagu",		L"¢©∩≅∪",
	"dv", "z",		L"ǆ",
	"d",  "-adegz",		L"ð↓‡°†ʣ",
	"e",  "ls",		L"⋯∅",
	"f",  "a",		L"∀",
	"g",  "$-r",		L"¤ǥ∇",
	"h",  "-v",		L"ℏƕ",
	"i",  "-bfjps",		L"ɨ⊆∞ĳ⊇∫",
	"l",  "\"$&'-jz|",	L"“£∧‘łǉ⋄∨",
	"m",  "iou",		L"µ∈×",
	"n",  "jo",		L"ǌ¬",
	"o",  "AOUaeiu",	L"Å⊚Ůåœƣů",
	"p",  "Odgrt",		L"℗∂¶∏∝",
	"r",  "\"'O",		L"”’®",
	"s",  "()+-0123456789=abnoprstu",
				L"⁽⁾⁺⁻⁰¹²³⁴⁵⁶⁷⁸⁹⁼ª⊂ⁿº⊃√ß∍∑",
	"t",  "-efmsu",		L"ŧ∃∴™ς⊢",
	"u",  "-AEGIOUaegiou",	L"ʉĂĔĞĬŎŬ↑ĕğĭŏŭ",
	"v\"","Uu",		L"Ǚǚ",
	"v",  "ACDEGIKLNORSTUZacdegijklnorstuz",
				L"ǍČĎĚǦǏǨĽŇǑŘŠŤǓŽǎčďěǧǐǰǩľňǒřšťǔž",
	"w",  "bknpqr",		L"♗♔♘♙♕♖",
	"x",  "O",		L"⊗",
	"y",  "$",		L"¥",
	"z",  "-",		L"ƶ",
	"|",  "Pp|",		L"Þþ¦",
	"~!", "=",		L"≆",
	"~",  "-=AINOUainou~",	L"≃≅ÃĨÑÕŨãĩñõũ≈",
	0,	0,		0
};

/*
 * Given 5 characters k[0]..k[4], find the rune or return -1 for failure.
 */
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

/*
 * Given n characters k[0]..k[n-1], find the corresponding rune or return -1 for
 * failure, or something < -1 if n is too small.  In the latter case, the result
 * is minus the required n.
 */
long
latin1(uchar *k, int n)
{
	struct cvlist *l;
	int c;
	char* p;

	if(k[0] == 'X')
		if(n>=5)
			return unicode(k);
		else
			return -5;
	for(l=latintab; l->ld!=0; l++)
		if(k[0] == l->ld[0]){
			if(n == 1)
				return -2;
			if(l->ld[1] == 0)
				c = k[1];
			else if(l->ld[1] != k[1])
				continue;
			else if(n == 2)
				return -3;
			else
				c = k[2];
			for(p=l->si; *p!=0; p++)
				if(*p == c)
					return l->so[p - l->si];
			return -1;
		}
	return -1;
}
