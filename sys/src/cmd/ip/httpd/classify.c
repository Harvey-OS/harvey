#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>
#include "whois.h"

typedef struct Country Country;

struct Country
{
	char *code;
	char *name;
};

Country badc[] =
{
	{"af", "afghanistan"},
	{"cu", "cuba"},
	{"ir", "iran"},
	{"iq", "iraq"},
	{"ly", "libya"},
	{"kp", "north korea"},
	{"sd", "sudan"},
	{"sy", "syria"},
	{ 0, 0 }
};

Country goodc[] =
{
	// the original, us and canada
	{"us", "united states of america"},
	{"ca", "canada"},
	{"gov", "gov"},
	{"mil", "mil"},

	// the european union
	{ "eu",	"european union" },
	{ "be",	"belgium" },
	{ "de",	"germany" },
	{ "fr",	"france" },
	{ "it",	"italy" },
	{ "lu",	"luxembourg" },
	{ "nl",	"netherlands" },
	{ "dk",	"denmark" },
	{ "ie",	"ireland" },
	{ "gb",	"great britain" },
	{ "uk",	"united kingdom" },
	{ "gr",	"greece" },
	{ "es",	"spain" },
	{ "pt",	"portugal" },
	{ "au",	"australia" },
	{ "fi",	"finland" },
	{ "se",	"sweden" },

	// the rest
	{"au", "australia"},
	{"no", "norway"},
	{"cz", "czech republic"},
	{"hu", "hungary"},
	{"pl", "poland"},
	{"jp", "japan"},
	{"ch", "switzerland"},
	{"nz", "new zealand"},
	{ 0, 0 }
};

char *gov[] =
{
	"gov",
	"gouv",
	"mil",
	"government",
	0,
};

Country allc[] =
{
	{ "ad",	"andorra" },
	{ "ae",	"united arab emirates" },
	{ "af",	"afghanistan" },
	{ "ag",	"antigua and barbuda" },
	{ "ai",	"anguilla" },
	{ "al",	"albania" },
	{ "am",	"armenia" },
	{ "an",	"netherlands antilles" },
	{ "ao",	"angola" },
	{ "aq",	"antarctica" },
	{ "ar",	"argentina" },
	{ "as",	"american samoa" },
	{ "at",	"austria" },
	{ "au",	"australia" },
	{ "aw",	"aruba" },
	{ "az",	"azerbaijan" },
	{ "ba",	"bosnia and herzegovina" },
	{ "bb",	"barbados" },
	{ "bd",	"bangladesh" },
	{ "be",	"belgium" },
	{ "bf",	"burkina faso" },
	{ "bg",	"bulgaria" },
	{ "bh",	"bahrain" },
	{ "bi",	"burundi" },
	{ "bj",	"benin" },
	{ "bm",	"bermuda" },
	{ "bn",	"brunei darussalam" },
	{ "bo",	"bolivia" },
	{ "br",	"brazil" },
	{ "bs",	"bahamas" },
	{ "bt",	"bhutan" },
	{ "bu",	"burma" },
	{ "bv",	"bouvet island" },
	{ "bw",	"botswana" },
	{ "by",	"belarus" },
	{ "bz",	"belize" },
	{ "ca",	"canada" },
	{ "cc",	"cocos (keeling) islands" },
	{ "cf",	"central african republic" },
	{ "cg",	"congo" },
	{ "ch",	"switzerland" },
	{ "ci",	"cote d'ivoire (ivory coast)" },
	{ "ck",	"cook islands" },
	{ "cl",	"chile" },
	{ "cm",	"cameroon" },
	{ "cn",	"china" },
	{ "co",	"colombia" },
	{ "cr",	"costa rica" },
	{ "cs",	"czechoslovakia (former)" },
	{ "ct",	"canton and enderbury island" },
	{ "cu",	"cuba" },
	{ "cv",	"cape verde" },
	{ "cx",	"christmas island" },
	{ "cy",	"cyprus" },
	{ "cz",	"czech republic" },
	{ "dd",	"german democratic republic" },
	{ "de",	"germany" },
	{ "dj",	"djibouti" },
	{ "dk",	"denmark" },
	{ "dm",	"dominica" },
	{ "do",	"dominican republic" },
	{ "dz",	"algeria" },
	{ "ec",	"ecuador" },
	{ "ee",	"estonia" },
	{ "eg",	"egypt" },
	{ "eh",	"western sahara" },
	{ "er",	"eritrea" },
	{ "es",	"spain" },
	{ "et",	"ethiopia" },
	{ "eu",	"european union" },
	{ "fi",	"finland" },
	{ "fj",	"fiji" },
	{ "fk",	"falkland islands (malvinas)" },
	{ "fm",	"micronesia" },
	{ "fo",	"faroe islands" },
	{ "fr",	"france" },
	{ "fx",	"france, metropolitan" },
	{ "ga",	"gabon" },
	{ "gb",	"great britain (uk)" },
	{ "gd",	"grenada" },
	{ "ge",	"georgia" },
	{ "gf",	"french guiana" },
	{ "gh",	"ghana" },
	{ "gi",	"gibraltar" },
	{ "gl",	"greenland" },
	{ "gm",	"gambia" },
	{ "gn",	"guinea" },
	{ "gp",	"guadeloupe" },
	{ "gq",	"equatorial guinea" },
	{ "gr",	"greece" },
	{ "gs",	"s. georgia and s. sandwich isls." },
	{ "gt",	"guatemala" },
	{ "gu",	"guam" },
	{ "gw",	"guinea-bissau" },
	{ "gy",	"guyana" },
	{ "hk",	"hong kong" },
	{ "hm",	"heard and mcdonald islands" },
	{ "hn",	"honduras" },
	{ "hr",	"croatia (hrvatska)" },
	{ "ht",	"haiti" },
	{ "hu",	"hungary" },
	{ "id",	"indonesia" },
	{ "ie",	"ireland" },
	{ "il",	"israel" },
	{ "in",	"india" },
	{ "io",	"british indian ocean territory" },
	{ "iq",	"iraq" },
	{ "ir",	"iran" },
	{ "is",	"iceland" },
	{ "it",	"italy" },
	{ "jm",	"jamaica" },
	{ "jo",	"jordan" },
	{ "jp",	"japan" },
	{ "jt",	"johnston island" },
	{ "ke",	"kenya" },
	{ "kg",	"kyrgyzstan" },
	{ "kh",	"cambodia (democratic kampuchea)" },
	{ "ki",	"kiribati" },
	{ "km",	"comoros" },
	{ "kn",	"saint kitts and nevis" },
	{ "kp",	"korea (north)" },
	{ "kr",	"korea (south)" },
	{ "kw",	"kuwait" },
	{ "ky",	"cayman islands" },
	{ "kz",	"kazakhstan" },
	{ "la",	"laos" },
	{ "lb",	"lebanon" },
	{ "lc",	"saint lucia" },
	{ "li",	"liechtenstein" },
	{ "lk",	"sri lanka" },
	{ "lr",	"liberia" },
	{ "ls",	"lesotho" },
	{ "lt",	"lithuania" },
	{ "lu",	"luxembourg" },
	{ "lv",	"latvia" },
	{ "ly",	"libya" },
	{ "ma",	"morocco" },
	{ "mc",	"monaco" },
	{ "md",	"moldova" },
	{ "mg",	"madagascar" },
	{ "mh",	"marshall islands" },
	{ "mi",	"midway islands" },
	{ "mk",	"macedonia" },
	{ "ml",	"mali" },
	{ "mm",	"myanmar" },
	{ "mn",	"mongolia" },
	{ "mo",	"macau" },
	{ "mp",	"northern mariana islands" },
	{ "mq",	"martinique" },
	{ "mr",	"mauritania" },
	{ "ms",	"montserrat" },
	{ "mt",	"malta" },
	{ "mu",	"mauritius" },
	{ "mv",	"maldives" },
	{ "mw",	"malawi" },
	{ "mx",	"mexico" },
	{ "my",	"malaysia" },
	{ "mz",	"mozambique" },
	{ "na",	"namibia" },
	{ "nc",	"new caledonia" },
	{ "ne",	"niger" },
	{ "nf",	"norfolk island" },
	{ "ng",	"nigeria" },
	{ "ni",	"nicaragua" },
	{ "nl",	"netherlands" },
	{ "no",	"norway" },
	{ "np",	"nepal" },
	{ "nq",	"dronning maud land" },
	{ "nr",	"nauru" },
	{ "nt",	"neutral zone" },
	{ "nu",	"niue" },
	{ "nz",	"new zealand (aotearoa)" },
	{ "om",	"oman" },
	{ "pa",	"panama" },
	{ "pc",	"pacific islands" },
	{ "pe",	"peru" },
	{ "pf",	"french polynesia" },
	{ "pg",	"papua new guinea" },
	{ "ph",	"philippines" },
	{ "pk",	"pakistan" },
	{ "pl",	"poland" },
	{ "pm",	"st. pierre and miquelon" },
	{ "pn",	"pitcairn" },
	{ "pr",	"puerto rico" },
	{ "pu",	"united states misc. pacific islands" },
	{ "pt",	"portugal" },
	{ "pw",	"palau" },
	{ "py",	"paraguay" },
	{ "qa",	"qatar" },
	{ "re",	"reunion" },
	{ "ro",	"romania" },
	{ "ru",	"russian federation" },
	{ "rw",	"rwanda" },
	{ "sa",	"saudi arabia" },
	{ "sb",	"solomon islands" },
	{ "sc",	"seychelles" },
	{ "sd",	"sudan" },
	{ "se",	"sweden" },
	{ "sg",	"singapore" },
	{ "sh",	"st. helena" },
	{ "si",	"slovenia" },
	{ "sj",	"svalbard and jan mayen islands" },
	{ "sk",	"slovak republic" },
	{ "sl",	"sierra leone" },
	{ "sm",	"san marino" },
	{ "sn",	"senegal" },
	{ "so",	"somalia" },
	{ "sr",	"suriname" },
	{ "st",	"sao tome and principe" },
	{ "su",	"ussr (former)" },
	{ "sv",	"el salvador" },
	{ "sy",	"syria" },
	{ "sz",	"swaziland" },
	{ "tc",	"turks and caicos islands" },
	{ "td",	"chad" },
	{ "tf",	"french southern territories" },
	{ "tg",	"togo" },
	{ "th",	"thailand" },
	{ "tj",	"tajikistan" },
	{ "tk",	"tokelau" },
	{ "tm",	"turkmenistan" },
	{ "tn",	"tunisia" },
	{ "to",	"tonga" },
	{ "tp",	"east timor" },
	{ "tr",	"turkey" },
	{ "tt",	"trinidad and tobago" },
	{ "tv",	"tuvalu" },
	{ "tw",	"taiwan" },
	{ "tz",	"tanzania" },
	{ "ua",	"ukraine" },
	{ "ug",	"uganda" },
	{ "uk",	"united kingdom" },
	{ "um",	"us minor outlying islands" },
	{ "us",	"united states" },
	{ "uy",	"uruguay" },
	{ "uz",	"uzbekistan" },
	{ "va",	"vatican city state (holy see)" },
	{ "vc",	"saint vincent and the grenadines" },
	{ "ve",	"venezuela" },
	{ "vg",	"virgin islands (british)" },
	{ "vi",	"virgin islands (u.s.)" },
	{ "vn",	"viet nam" },
	{ "vu",	"vanuatu" },
	{ "wf",	"wallis and futuna islands" },
	{ "wk",	"wake island" },
	{ "ws",	"samoa" },
	{ "yd",	"democratic yemen" },
	{ "ye",	"yemen" },
	{ "yt",	"mayotte" },
	{ "yu",	"yugoslavia" },
	{ "za",	"south africa" },
	{ "zm",	"zambia" },
	{ "zr",	"zaire" },
	{ "zw",	"zimbabwe" },

	{"gov", "gov"},
	{"mil", "mil"},

	{ 0, 0 }
};

int classdebug;

static int
incountries(char *s, Country *cp)
{
	for(; cp->code != 0; cp++)
		if(cistrcmp(s, cp->code) == 0
		|| cistrcmp(s, cp->name) == 0)
			return 1;
	return 0;
}

static int
indomains(char *s, char **dp)
{
	for(; *dp != nil; dp++)
		if(cistrcmp(s, *dp) == 0)
			return 1;

	return 0;
}

int
classify(char *ip, Ndbtuple *t)
{
	int isgov, iscountry, isbadc, isgoodc;
	char dom[256];
	char *df[128];
	Ndbtuple *nt, *x;
	int n;

	isgov = iscountry = isbadc = 0;
	isgoodc = 1;
	
	for(nt = t; nt != nil; nt = nt->entry){
		if(strcmp(nt->attr, "country") == 0){
			iscountry = 1;
			if(incountries(nt->val, badc)){
				if(classdebug)fprint(2, "isbadc\n");
				isbadc = 1;
				isgoodc = 0;
			} else if(!incountries(nt->val, goodc)){
				if(classdebug)fprint(2, "!isgoodc\n");
				isgoodc = 0;
			}
		}

		/* domain names can always hurt, even without forward verification */
		if(strcmp(nt->attr, "dom") == 0){
			strncpy(dom, nt->val, sizeof dom);
			dom[sizeof(dom)-1] = 0;
			n = getfields(dom, df, nelem(df), 0, ".");

			/* a bad country in a domain name is always believed */
			if(incountries(df[n-1], badc)){
				if(classdebug)fprint(2, "isbadc dom\n");
				isbadc = 1;
				isgoodc = 0;
			}

			/* a goverment in a domain name is always believed */
			if(n > 1 && indomains(df[n-2], gov))
				isgov = 1;
		}
	}
	if(iscountry == 0){
		/* did the forward lookup work? */
		for(nt = t; nt != nil; nt = nt->entry){
			if(strcmp(nt->attr, "ip") == 0 && strcmp(nt->val, ip) == 0)
				break;
		}

		/* see if the domain name ends in a country code */
		if(nt != nil && (x = ndbfindattr(t, nt, "dom")) != nil){
			strncpy(dom, x->val, sizeof dom);
			dom[sizeof(dom)-1] = 0;
			n = getfields(dom, df, nelem(df), 0, ".");
			if(incountries(df[n-1], allc))
				iscountry = 1;
		}
	}
	if(iscountry == 0)
		return Cunknown;
	if(isbadc)
		return Cbadc;
	if(!isgoodc && isgov)
		return Cbadgov;
	return Cok;
}
