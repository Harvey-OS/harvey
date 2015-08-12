/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* sorted by unicode value */
Htmlchar htmlchars[] = {
    {"\"", "&quot;"},
    {"&", "&amp;"},
    {"<", "&lt;"},
    {">", "&gt;"},
    {"¡", "&iexcl;"},
    {"¢", "&cent;"},
    {"£", "&pound;"},
    {"¤", "&curren;"},
    {"¥", "&yen;"},
    {"¦", "&brvbar;"},
    {"§", "&sect;"},
    {"¨", "&uml;"},
    {"©", "&copy;"},
    {"ª", "&ordf;"},
    {"«", "&laquo;"},
    {"¬", "&not;"},
    {"-", "&ndash;"},
    {"­", "&ndash;"},
    {"®", "&reg;"},
    {"¯", "&macr;"},
    {"°", "&deg;"},
    {"±", "&plusmn;"},
    {"²", "&sup2;"},
    {"³", "&sup3;"},
    {"´", "&acute;"},
    {"µ", "&micro;"},
    {"¶", "&para;"},
    {"·", "&middot;"},
    {"¸", "&cedil;"},
    {"¹", "&sup1;"},
    {"º", "&ordm;"},
    {"»", "&raquo;"},
    {"¼", "&frac14;"},
    {"½", "&frac12;"},
    {"¾", "&frac34;"},
    {"¿", "&iquest;"},
    {"À", "&Agrave;"},
    {"Á", "&Aacute;"},
    {"Â", "&Acirc;"},
    {"Ã", "&Atilde;"},
    {"Ä", "&Auml;"},
    {"Å", "&Aring;"},
    {"Æ", "&AElig;"},
    {"Ç", "&Ccedil;"},
    {"È", "&Egrave;"},
    {"É", "&Eacute;"},
    {"Ê", "&Ecirc;"},
    {"Ë", "&Euml;"},
    {"Ì", "&Igrave;"},
    {"Í", "&Iacute;"},
    {"Î", "&Icirc;"},
    {"Ï", "&Iuml;"},
    {"Ð", "&ETH;"},
    {"Ñ", "&Ntilde;"},
    {"Ò", "&Ograve;"},
    {"Ó", "&Oacute;"},
    {"Ô", "&Ocirc;"},
    {"Õ", "&Otilde;"},
    {"Ö", "&Ouml;"},
    {"×", "x"},
    {"Ø", "&Oslash;"},
    {"Ù", "&Ugrave;"},
    {"Ú", "&Uacute;"},
    {"Û", "&Ucirc;"},
    {"Ü", "&Uuml;"},
    {"Ý", "&Yacute;"},
    {"Þ", "&THORN;"},
    {"ß", "&szlig;"},
    {"à", "&agrave;"},
    {"á", "&aacute;"},
    {"â", "&acirc;"},
    {"ã", "&atilde;"},
    {"ä", "&auml;"},
    {"å", "&aring;"},
    {"æ", "&aelig;"},
    {"ç", "&ccedil;"},
    {"è", "&egrave;"},
    {"é", "&eacute;"},
    {"ê", "&ecirc;"},
    {"ë", "&euml;"},
    {"ì", "&igrave;"},
    {"í", "&iacute;"},
    {"î", "&icirc;"},
    {"ï", "&iuml;"},
    {"ð", "&eth;"},
    {"ñ", "&ntilde;"},
    {"ò", "&ograve;"},
    {"ó", "&oacute;"},
    {"ô", "&ocirc;"},
    {"õ", "&otilde;"},
    {"ö", "&ouml;"},
    {"ø", "&oslash;"},
    {"ù", "&ugrave;"},
    {"ú", "&uacute;"},
    {"û", "&ucirc;"},
    {"ü", "&uuml;"},
    {"ý", "&yacute;"},
    {"þ", "&thorn;"},
    {"ÿ", "&yuml;"},
    {"•", "*"},
    {"™", "(tm)"},
    {"←", "&larr;"},
    {"↑", "&uarr;"},
    {"→", "&rarr;"},
    {"↓", "&darr;"},
    {"≠", "!="},
    {"≤", "&le;"},
    /*	{ "□",	"&#164;" },
            { "◊",	"&#186;" }, */
};

/* unsorted */
Troffchar troffchars[] = {
    {
     "A*", "&Aring;",
    },
    {
     "o\"", "&ouml;",
    },
    {
     "ff", "ff",
    },
    {
     "fi", "fi",
    },
    {
     "fl", "fl",
    },
    {
     "Fi", "ffi",
    },
    {
     "ru", "_",
    },
    {
     "em", "--",
    },
    {
     "en", "-",
    },
    {
     "\\-", "&ndash;",
    },
    {
     "14", "&#188;",
    },
    {
     "12", "&#189;",
    },
    {
     "co", "&#169;",
    },
    {
     "de", "&#176;",
    },
    {
     "dg", "&#161;",
    },
    {
     "fm", "&#180;",
    },
    {
     "rg", "&#174;",
    },
    {
     "bu", "*",
    },
    {
     "sq", "&#164;",
    },
    {
     "hy", "&ndash;",
    },
    {
     "pl", "+",
    },
    {
     "mi", "-",
    },
    {
     "mu", "&#215;",
    },
    {
     "di", "&#247;",
    },
    {
     "eq", "=",
    },
    {
     "==", "==",
    },
    {
     ">=", ">=",
    },
    {
     "<=", "<=",
    },
    {
     "!=", "!=",
    },
    {
     "+-", "&#177;",
    },
    {
     "no", "&#172;",
    },
    {
     "sl", "/",
    },
    {
     "ap", "&",
    },
    {
     "~=", "~=",
    },
    {
     "pt", "oc",
    },
    {
     "gr", "GRAD",
    },
    {
     "->", "->",
    },
    {
     "<-", "<-",
    },
    {
     "ua", "^",
    },
    {
     "da", "v",
    },
    {
     "is", "Integral",
    },
    {
     "pd", "DIV",
    },
    {
     "if", "oo",
    },
    {
     "sr", "-/",
    },
    {
     "sb", "(~",
    },
    {
     "sp", "~)",
    },
    {
     "cu", "U",
    },
    {
     "ca", "(^)",
    },
    {
     "ib", "(=",
    },
    {
     "ip", "=)",
    },
    {
     "mo", "C",
    },
    {
     "es", "&Oslash;",
    },
    {
     "aa", "&#180;",
    },
    {
     "ga", "`",
    },
    {
     "ci", "O",
    },
    {
     "L1", "DEATHSTAR",
    },
    {
     "sc", "&#167;",
    },
    {
     "dd", "++",
    },
    {
     "lh", "<=",
    },
    {
     "rh", "=>",
    },
    {
     "lt", "(",
    },
    {
     "rt", ")",
    },
    {
     "lc", "|",
    },
    {
     "rc", "|",
    },
    {
     "lb", "(",
    },
    {
     "rb", ")",
    },
    {
     "lf", "|",
    },
    {
     "rf", "|",
    },
    {
     "lk", "|",
    },
    {
     "rk", "|",
    },
    {
     "bv", "|",
    },
    {
     "ts", "s",
    },
    {
     "br", "|",
    },
    {
     "or", "|",
    },
    {
     "ul", "_",
    },
    {
     "rn", " ",
    },
    {
     "**", "*",
    },
    {
     nil, nil,
    },
};
