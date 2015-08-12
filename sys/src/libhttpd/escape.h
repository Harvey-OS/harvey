/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

Htmlesc htmlesc[] =
    {
     {
      "&#161;", L'¡',
     },
     {
      "&#162;", L'¢',
     },
     {
      "&#163;", L'£',
     },
     {
      "&#164;", L'¤',
     },
     {
      "&#165;", L'¥',
     },
     {
      "&#166;", L'¦',
     },
     {
      "&#167;", L'§',
     },
     {
      "&#168;", L'¨',
     },
     {
      "&#169;", L'©',
     },
     {
      "&#170;", L'ª',
     },
     {
      "&#171;", L'«',
     },
     {
      "&#172;", L'¬',
     },
     {
      "&#173;", L'­',
     },
     {
      "&#174;", L'®',
     },
     {
      "&#175;", L'¯',
     },
     {
      "&#176;", L'°',
     },
     {
      "&#177;", L'±',
     },
     {
      "&#178;", L'²',
     },
     {
      "&#179;", L'³',
     },
     {
      "&#180;", L'´',
     },
     {
      "&#181;", L'µ',
     },
     {
      "&#182;", L'¶',
     },
     {
      "&#183;", L'·',
     },
     {
      "&#184;", L'¸',
     },
     {
      "&#185;", L'¹',
     },
     {
      "&#186;", L'º',
     },
     {
      "&#187;", L'»',
     },
     {
      "&#188;", L'¼',
     },
     {
      "&#189;", L'½',
     },
     {
      "&#190;", L'¾',
     },
     {
      "&#191;", L'¿',
     },
     {
      "&Agrave;", L'À',
     },
     {
      "&Aacute;", L'Á',
     },
     {
      "&Acirc;", L'Â',
     },
     {
      "&Atilde;", L'Ã',
     },
     {
      "&Auml;", L'Ä',
     },
     {
      "&Aring;", L'Å',
     },
     {
      "&AElig;", L'Æ',
     },
     {
      "&Ccedil;", L'Ç',
     },
     {
      "&Egrave;", L'È',
     },
     {
      "&Eacute;", L'É',
     },
     {
      "&Ecirc;", L'Ê',
     },
     {
      "&Euml;", L'Ë',
     },
     {
      "&Igrave;", L'Ì',
     },
     {
      "&Iacute;", L'Í',
     },
     {
      "&Icirc;", L'Î',
     },
     {
      "&Iuml;", L'Ï',
     },
     {
      "&ETH;", L'Ð',
     },
     {
      "&Ntilde;", L'Ñ',
     },
     {
      "&Ograve;", L'Ò',
     },
     {
      "&Oacute;", L'Ó',
     },
     {
      "&Ocirc;", L'Ô',
     },
     {
      "&Otilde;", L'Õ',
     },
     {
      "&Ouml;", L'Ö',
     },
     {
      "&215;", L'×',
     },
     {
      "&Oslash;", L'Ø',
     },
     {
      "&Ugrave;", L'Ù',
     },
     {
      "&Uacute;", L'Ú',
     },
     {
      "&Ucirc;", L'Û',
     },
     {
      "&Uuml;", L'Ü',
     },
     {
      "&Yacute;", L'Ý',
     },
     {
      "&THORN;", L'Þ',
     },
     {
      "&szlig;", L'ß',
     },
     {
      "&agrave;", L'à',
     },
     {
      "&aacute;", L'á',
     },
     {
      "&acirc;", L'â',
     },
     {
      "&atilde;", L'ã',
     },
     {
      "&auml;", L'ä',
     },
     {
      "&aring;", L'å',
     },
     {
      "&aelig;", L'æ',
     },
     {
      "&ccedil;", L'ç',
     },
     {
      "&egrave;", L'è',
     },
     {
      "&eacute;", L'é',
     },
     {
      "&ecirc;", L'ê',
     },
     {
      "&euml;", L'ë',
     },
     {
      "&igrave;", L'ì',
     },
     {
      "&iacute;", L'í',
     },
     {
      "&icirc;", L'î',
     },
     {
      "&iuml;", L'ï',
     },
     {
      "&eth;", L'ð',
     },
     {
      "&ntilde;", L'ñ',
     },
     {
      "&ograve;", L'ò',
     },
     {
      "&oacute;", L'ó',
     },
     {
      "&ocirc;", L'ô',
     },
     {
      "&otilde;", L'õ',
     },
     {
      "&ouml;", L'ö',
     },
     {
      "&247;", L'÷',
     },
     {
      "&oslash;", L'ø',
     },
     {
      "&ugrave;", L'ù',
     },
     {
      "&uacute;", L'ú',
     },
     {
      "&ucirc;", L'û',
     },
     {
      "&uuml;", L'ü',
     },
     {
      "&yacute;", L'ý',
     },
     {
      "&thorn;", L'þ',
     },
     {
      "&yuml;", L'ÿ',
     },

     {
      "&quot;", L'"',
     },
     {
      "&#39;", L'\'',
     }, /* Note &apos; is valid XML but not valid HTML */
     {
      "&amp;", L'&',
     },
     {
      "&lt;", L'<',
     },
     {
      "&gt;", L'>',
     },

     {
      "CAP-DELTA", L'Δ',
     },
     {
      "ALPHA", L'α',
     },
     {
      "BETA", L'β',
     },
     {
      "DELTA", L'δ',
     },
     {
      "EPSILON", L'ε',
     },
     {
      "THETA", L'θ',
     },
     {
      "MU", L'μ',
     },
     {
      "PI", L'π',
     },
     {
      "TAU", L'τ',
     },
     {
      "CHI", L'χ',
     },

     {
      "<-", L'←',
     },
     {
      "^", L'↑',
     },
     {
      "->", L'→',
     },
     {
      "v", L'↓',
     },
     {
      "!=", L'≠',
     },
     {
      "<=", L'≤',
     },
     {nil, 0},
};
