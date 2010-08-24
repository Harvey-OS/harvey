#include <u.h>
#include <libc.h>
#include <stdio.h>

#define R rand()&32767
#define T 0.125
#define CHOOSE(x) (x[(R)%(sizeof x / sizeof x[0])])
#define EQ(a,b) (strcmp(a,b)==0)
#define LAST(x) (x[strlen(x)-1])
#define VOWEL(x) (x=='a'||x=='e'||x=='i'||x=='o'||x=='u')
#define N 8

typedef struct xyz {
	char           *type;
	union {
		struct xyz     *x[N];
		char           *s[N];
	}               list;
}              *X, XX;
typedef struct {
	char           *number, *ending, *tense, *an, *unspec, *passive;
}              *E, EE;

char           *tense(), *number();
char            buff[1000];
int             io;
int             flag;
int		eqn=0, tbl=0, pic=0;
double          makeup = -1.;

X np(E);
X aux(E);
X passive(E);
X passprep(void);
X vp(E);
X art(E);
X modal(E);
X perf(E);
X prog(E);
X verb(E);
X noun(E);
X nounal(E);
char *prefix(void);
char *root(void);
int prob(double);
char *tense(void);
char *number(void);
X getxx(void);
X verbal(E);
X adverb(void);
X adjective(void);
X adjph(E);
X prep(void);
X comp(E);
X advp(E);
X vprep(void);
E getenvq(void);
X comma(E);
X conjadv(void);
X lconjadv(void);
X conjsub(void);
X lconjsub(void);
X conj(void);
X nomz(void);
X equation();
X turgid(E);
void pr(X);
void out(char *);
void caps(void);
void abo(void);
char *splitup(char *);

X 
nomq(E env) {
	X               v = getxx();
	v->type = "-nomq";
	if (EQ(env->number, "sing")) {
		if (EQ(tense(), "past"))
			v->list.s[0] = "there was";
		else
			v->list.s[0] = "there is";
	} else {
		if (EQ(tense(), "past"))
			v->list.s[0] = "there were";
		else
			v->list.s[0] = "there are";
	}
	if (prob(0.2))
		v->list.s[1] = " not";
	return v;
}

X 
rel(void) {
	static char    *c[] = {"that", "which"};
	X               v = getxx();
	v->type = "-rel";
	v->list.s[0] = CHOOSE(c);
	return v;
}

X 
sent(E env) {
	X               sentv = getxx();
	sentv->type = "sent";
	if (prob(0.09)) {
		env->unspec = "";
		sentv->list.x[1] = np(env);
		sentv->list.x[3] = aux(env);
		sentv->list.x[4] = vp(env);
		sentv->list.x[0] = nomq(env);
		sentv->list.x[2] = rel();
		return sentv;
	}
	sentv->list.x[0] = np(env);
	sentv->list.x[1] = aux(env);
	sentv->list.x[2] = vp(env);
	return sentv;
}

X 
nomy(void) {
	X               v = getxx();
	v->type = "-nomq";
	v->list.s[0] = "the fact that";
	return v;
}

X 
np(env)
	E               env;
{
	X               npv = getxx();
	EE              nenv;
	static EE       empty;
	npv->type = "np";
	if (prob(0.025)) {
		nenv = empty;
		npv->list.x[0] = nomy();
		npv->list.x[1] = sent(&nenv);
		if (env->number == 0)
			env->number = "sing";
		return npv;
	}
	npv->list.x[1] = nounal(env);
	npv->list.x[0] = art(env);
	return npv;
}

X 
aux(E env) {
	X               auxv = getxx();
	int             i = 0;
	auxv->type = "aux";
	if (env->tense == 0)
		env->tense = env->ending = tense();
	if (prob(0.25))
		auxv->list.x[i++] = modal(env);
	if (prob(0.25))
		auxv->list.x[i++] = perf(env);
	if (prob(0.25))
		auxv->list.x[i++] = prog(env);
	USED(i);
	return auxv;
}

X 
passive(env)
	E               env;
{
	X               v = getxx();
	v->type = "-passive";
	if (env->tense == 0)
		env->tense = env->ending = tense();
	if (env->number == 0)
		env->number = number();
	if (EQ(env->ending, "modal"))
		v->list.s[0] = "be";
	else if (EQ(env->ending, "-en"))
		v->list.s[0] = "been";
	else if (EQ(env->ending, "-ing"))
		v->list.s[0] = "being";
	else {
		if (EQ(env->tense, "past"))
			v->list.s[0] = EQ(env->number, "sing") ? "was" : "were";
		else
			v->list.s[0] = EQ(env->number, "sing") ? "is" : "are";
	}
	env->passive = env->ending = "pass";
	return v;
}

X 
passprep(void) {
	X               v = getxx();
	v->type = "-passprep";
	v->list.s[0] = "by";
	return v;
}

X 
vp(env)
	E               env;
{
	X               vpv = getxx();
	int             i = 0;
	vpv->type = "vp";
	if (prob(0.5))
		vpv->list.x[i++] = passive(env);
	vpv->list.x[i++] = verbal(env);
	vpv->list.x[i++] = comp(env);
	if (prob(0.10))
		vpv->list.x[i++] = advp(env);
	USED(i);
	return vpv;
}

X 
art(env)
	E               env;
{
	static char    *aspecsg[] = {"the", "the", "the", "the", "the", "this", "this", "that"};
	static char    *aspecpl[] = {"the", "the", "the", "the", "the", "these", "those"};
	static char    *aunssg[] = {"a", "a", "a", "a", "a", "a", "a", "much", "each", "any"};
	static char    *aunspl[] = {"some", "a few", "a couple", "several", "many", "all",
		"no",
		"an undue number of",
	"a number of"};
	X               artv = getxx();
	artv->type = "-art";
	if (env->number == 0)
		env->number = number();
	if (env->unspec == 0 && prob(0.33)) {
		if (EQ(env->number, "sing"))
			artv->list.s[0] = CHOOSE(aspecsg);
		else
			artv->list.s[0] = CHOOSE(aspecpl);
	} else if (prob(0.50) || env->an && EQ(env->number, "sing")) {
		if (EQ(env->number, "sing"))
			artv->list.s[0] = env->an ? "a" : CHOOSE(aunssg);
		else
			artv->list.s[0] = CHOOSE(aunspl);
		if (env->an && EQ(artv->list.s[0], "all"))
			artv->list.s[0] = "";
	} else
		artv->list.s[0] = "";
	env->unspec = 0;
	if (env->an && EQ(env->an, "an") && EQ(artv->list.s[0], "a"))
		artv->list.s[0] = "an";
	env->an = 0;
	return artv;
}

X 
modal(env)
	E               env;
{
	static char    *pres[] = {"can", "may", "must", "shall", "will"};
	static char    *past[] = {"could", "might", "should", "would"};
	X               modalv = getxx();
	modalv->type = "-modal";
	if (env->tense == 0)
		env->tense = env->ending = tense();
	if (EQ(env->ending, "pres"))
		modalv->list.s[0] = CHOOSE(pres);
	else
		modalv->list.s[0] = CHOOSE(past);
	env->ending = "modal";
	return modalv;
}

X 
perf(env)
	E               env;
{
	X               perfv = getxx();
	perfv->type = "-perf";
	if (env->tense == 0)
		env->tense = env->ending = tense();
	if (env->number == 0)
		env->number = number();
	if (EQ(env->ending, "past")) {
		perfv->list.s[0] = "had";
	} else if (EQ(env->ending, "pres")) {
		if (EQ(env->number, "sing"))
			perfv->list.s[0] = "had";
		else
			perfv->list.s[0] = "have";
	} else
		perfv->list.s[0] = "have";
	env->ending = "-en";
	return perfv;
}

X 
prog(env)
	E               env;
{
	X               progv = getxx();
	progv->type = "-prog";
	if (env->tense == 0)
		env->tense = env->ending = tense();
	if (env->number == 0)
		env->number = number();
	if (EQ(env->ending, "pres")) {
		if (EQ(env->number, "sing"))
			progv->list.s[0] = "is";
		else
			progv->list.s[0] = "are";
	} else if (EQ(env->ending, "past")) {
		if (EQ(env->number, "sing"))
			progv->list.s[0] = "was";
		else
			progv->list.s[0] = "were";
	} else if (EQ(env->ending, "-en")) {
		progv->list.s[0] = "been";
	} else if (EQ(env->ending, "modal")) {
		progv->list.s[0] = "be";
	}
	env->ending = "-ing";
	return progv;
}

X 
verb(env)
	E               env;
{
	/* they pres, he pres, they past, they perf, they prog, they pass */
	static char    *ends[][6] = {{"ate", "ates", "ated", "ated", "ating", "ated"},
	{"en", "ens", "ened", "ened", "ening", "ened"},
	{"esce", "esces", "esced", "esced", "escing", "esced"},
	{"fy", "fies", "fied", "fied", "fying", "fied"},
	{"ize", "izes", "ized", "ized", "izing", "ized"}};
	X               verbv = getxx();
	int             i;
	verbv->type = "-verb";
	if (env->tense == 0)
		env->tense = env->ending = tense();
	if (env->number == 0)
		env->number = number();
	if (0 && prob(0.1) && EQ(env->tense, env->ending)) {
		if (EQ(env->number, "sing")) {
			if (EQ(env->tense, "pres"))
				verbv->list.s[0] = "is";
			else
				verbv->list.s[0] = "was";
		} else {
			if (EQ(env->tense, "pres"))
				verbv->list.s[0] = "are";
			else
				verbv->list.s[0] = "were";
		}
	} else {
		verbv->list.s[0] = prefix();
		verbv->list.s[1] = root();
		if (EQ(env->ending, "pres") && EQ(env->number, "sing"))
			i = 1;
		else if (EQ(env->ending, "pres") || EQ(env->ending, "modal"))
			i = 0;
		else if (EQ(env->ending, "past"))
			i = 2;
		else if (EQ(env->ending, "-en"))
			i = 3;
		else if (EQ(env->ending, "-ing"))
			i = 4;
		else if (EQ(env->ending, "pass"))
			i = 5;
		else
			i = 0;
		verbv->list.s[2] = ends[R % (sizeof ends / sizeof *ends)][i];
	}
	env->ending = 0;
	return verbv;
}

static char    *nounlist[] = {"final completion",
	"final ending", "final outcome",
	"adaptation", "appearance", "argument", "circumstance",
	"confession", "confidence", "delimitation", "dilution",
	"dissertation", "distribution", "duplication",
	"entertainment", "equipment", "evolution",
	"existence", "expression", "generation", "impression",
	"integration", "interaction", "investment", "judgment",
	"population", "provision", "solution", "statement",
	"tradition", "transmission",
	"final result", "added increment", "assistance",
	"beneficial assistance", "mutual cooperation",
	"projection", "future projection",
	"capability", "conjecture", "consensus of opinion",
	"general consensus", "absence", "deficiency",
	"inadequacy", "insufficience", "insufficiency",
	"growing importance", "renewed emphasis",
	"renewed interest", "changing behavior",
	"critical thinking", "careful thinking",
	"comprehensive survey", "high standard",
	"basic foundation", "system testing",
	"serious discussion", "serious concern",
	"organizational framework", "prototype model",
	"uniform nomenclature", "greater cooperation",
	"uniform consistency", "early expectation",
	"standardization", "great similarity",
	"shortage", "presence", "sufficiency",
	"consequent result", "construct", "disutility",
	"early beginning", "emotional feeling", "endeavor",
	"authorization", "order of magnitude", "preference",
	"impact", "joint cooperation", "joint partnership",
	"main essential", "methodology", "modification",
	"necessary requisite", "past history", "situation",
	"effectuation", "clarification", "new doubt",
	"policy", "encouragement", "preparation",
	"criterion", "material", "interest", "acceptance",
	"rejection", "publication", "circulation",
	"protection", "insurance",
	"assignment", "identification",
	"submission", "request",
	"guidance", "correspondence", "inclusion",
	"attachment", "assumption",
	"recommendation", "prescription", "approval",
	"discretion", "responsibility", "relevance",
	"issuance", "termination", "total effect",
	"deleterious effect", "consolidation",
	"aggregation", "definiteness", "commencement",
	"actual experience", "experience",
	"combination", "accord", "filing",
	"idea", "abstraction", "method", "procedure",
	"complaint", "maintenance", "finance", "travel",
	"purchase", "repair", "routine",
	"development", "cancellation",
	"partitioning", "development effort",
	"project", "automation", "multilevel architecture",
	"multilevel heirarchy", "data stream",
	"objective",
	"individual assignment", "mode of operation",
	"clear community", "attendant interest",
	"task division", "well defined interfacing",
	"team report", "meeting time", "effective use",
	"friction",
	"major objective", "ownership",
	"overall project time constraint",
	"functional division", "requirements analysis",
	"code development", "charter",
	"requirements definition", "vertical division",
	"broad range", "strong feeling",
	"considerable latitude", "overall project constraint",
	"sufficient resource", "assigned task", "expectation",
	"critical aspect", "clear understanding",
	"computing load", "clean interfacing", "natural basis",
	"team activity", "team responsibility",
	"main function", "predominant portion",
	"work plan", "major breakpoint", "work module",
	"achievable accuracy", "supplementary work",
	"field version", "internal establishment",
	"internal communication", "development progress",
	"internal meeting", "experience level",
	"high level autonomy", "adherence",
	"feasibility demonstration", "persistent problem",
	"internal objective", "idea sharing",
	"improved performance", "unfamiliar methodology",
	"new methodology", "development experience",
	"module specification", "good progress",
	"optimal number", "natural division",
	"good relationship", "cross attendance",
	"attendance", "necessary communication",
	"evolving organization", "basic principle",
	"complete revision", "general information",
	"primary objective", "load-carrying capacity",
	"necessary revision", "major change",
	"clarified interpretation", "subsequent attempt",
	"basic objective", "full utilization",
	"practical consideration",
	"proportionate quantity", "substantial change",
	"database design", "unified framework",
	"customer service", "strong interest",
	"unified description", "necessary background information",
	"provisioning", "physical coverage", "general observation",
	"new technology", "validity determination",
	"relation", "regulation", "verification",
	"impediment", "portal", "practice", "premise",
	"basis", "movement", "question",
	"issue", "input", "output", "observation",
	"input", "output", "input", "output",
	"mechanization", "function", "evaluation",
	"result", "further consideration", "category",
	"performance indicator", "early warning",
	"analysis purpose", "measurement", "replacement",
	"utilitarian purpose",
	"quota", "proposed enhancement", "enhancement",
	"interfacing", "team organization", "module",
	"guideline", "continuing study",
	"required assistance", "major advance",
	"proposal", "hierarchy",
	"current view", "refinement", "activity",
	"external description", "tight schedule pressure",
	"internal conflict", "internal issue",
	"reasonable compromise", "next phase",
	"goal", "time constraint", "constraint",
	"outcome", "important outcome",
	"considerable experience", "intelligent choice",
	"deliverable documentation", "discussion",
	"timely delivery", "design issue", "large quantity",
	"general environment", "protocol",
	"transitioning", "modeling",
	"considerable difficulty", "abstract interfacing",
	"data structure", "consideration", "difficulty",
	"statistical accuracy",
	"agenda", "technique", "reordering",
	"reauthorization", "current proposal",
	"significant change", "criteria", "validation",
	"validity",
	"terminology", "current understanding",
	"incorporation", "staffing impact",
	"schedule impact", "cost tradeoff",
	"system architecture",
	"adequate capacity", "centralization",
	"current task", "system deployment",
	"attendant uncertainty", "process",
	"potential usefulness", "proposed method",
	"basic assumption", "anomaly",
	"available data", "potential improvement",
	"registration", "exemption", "exception",
	"follow-up", "service",
	"installation", "construction", "necessity",
	"occasion", "instrumentation", "disposal",
	"attractiveness",
	"convenience", "sponsoring",
	"signification", "meaningfulness",
	"significantness", "individuality",
	"specification", "determination", "affirmation",
	"recruitment", "supervision", "management",
	"oversight", "overview", "environment",
	"effectation", "circumvention", "location",
	"execution", "effectiveness", "consciousness",
	"notation", "confirmation", "restriction",
	"organization", "realization", "actification",
	"activation", "reification", "beginning", "conclusion",
	"ending", "finishing", "teamwork", "motivation",
	"attitude", "good attitude",
	"progress", "milestone", "deadline", "schedule",
	"allocation", "resource", "command", "concern",
	"time", "time frame", "reality",
	"behaviour", "ability", "advent", "increment",
	"opportunity", "accomplishment", "aggregate",
	"analysis", "totality", "matter",
	"date", "duration", "centrality",
	"proximity", "collection", "elimination",
	"investigation", "opinion", "debate",
	"decision", "benefit", "difference", "discontinuity",
	"fabrication", "plan", "chart", "forecast",
	"simplicity", "simplification", "maximization",
	"minimization", "direction",
	"agreement",
	"amount", "quantity", "quality", "essence",
	"description", "violation", "purpose",
	"primary purpose", "automatic control", "redefinition",
	"uniform emphasis", "study activity", "work activity",
	"concept stage", "concept activity",
	"possible potential", "summarization", "system function",
	"rationale", "significant enhancement", "diverse need",
	"diverse organization", "comprehensive plan", "interim",
	"functional overview", "system configuration",
	"configuration", "failure", "quantitative result",
	"major obstacle", "conception",
	"effectiveness", "final evaluation",
	"interrelationship", "functional requirement",
	"system philosophy", "verbal interchange",
	"perceived inadequacy", "primary emphasis",
	"intermingling", "cooperation", "partnership",
	"adjustment", "application", "implementation",
	"contact", "mention", "power",
	"nature", "invention", "importance",
	"ground", "reason", "permission", "size",
	"report", "documentation", "priority",
	"pursuance", "recurrance", "resumption",
	"presupposition", "continuance",
	"substantiation", "success", "action", "truth",
	"past experience", "greater acceptability",
	"organizational structure", "clear distinction",
	"clear definition",
	"significant use", "unmet need", "centralized organization",
	"vague concept", "negative impact", "detrimental effect",
	"modularization", "submodularization",
	"effect", "consistancy",
	"inconsistancy", "completion", "utilization",
	"reference", "doubt", "evidence",
	"viewpoint",
	"actual fact",
	"true fact", "underlying purpose", "viable alternative"};

static char    *adjlist[] = {"concrete", "abstract", "procedural",
	"real", "ideal", "functional", "prototype",
	"effective", "capable", "incremental",
	"perceived", "associated", "interdepartmental",
	"diverse", "characteristic", "worst-case",
	"qualitative", "fully automatic", "candidate",
	"consensual", "consequential", "conjectural",
	"constructive", "initial", "cooperative",
	"essential", "methodological", "requisite",
	"historical", "situational", "political",
	"prepared", "material", "defined", "well defined",
	"organizational", "projected", "overall",
	"accepted", "rejected", "corresponding",
	"committed", "environmental", "typical", "working", "timely",
	"growing", "unprecedented", "new", "renewed", "fresh",
	"rapid", "changing", "careful", "comprehensive", "broad",
	"massive", "huge", "enormous",
	"evaluated", "discresionary",
	"durable", "beneficial",
	"maximal", "tremendous", "minimal",
	"on-site", "standardized", "standard",
	"powerful", "natural", "necessary",
	"reasonable", "successful",
	"doubtful", "dubious", "certain",
	"unified", "different", "similar", "utilitarian",
	"realizable", "organizable", "motivated",
	"topical", "valuable", "feasible",
	"intelligent", "deliverable", "nontrivial",
	"worthwhile", "complicated",
	"organized", "organizing", "progressing",
	"schedulable", "resourceful", "commanding",
	"important", "allocatable", "temporal",
	"ponderable", "understandable", "comprehendable",
	"past", "present", "future",
	"obvious", "considerable", "finished", "completed",
	"unique", "abovementioned",
	"major", "minor", "tendentious", "activating",
	"actual", "added", "adequate", "affordable",
	"analyzable", "additional", "intuitive",
	"artificial", "good", "better",
	"worse", "bad", "basic", "fundamental", "brief",
	"general", "very unique", "extreme", "most unique",
	"central", "proximate", "approximate", "collected",
	"conductable", "comtemplatable",
	"continuing", "demonstrable", "desirable",
	"correctable", "foreseeable",
	"discontinued", "early", "beginning",
	"effectuated", "elucidated", "emotional",
	"enclosed", "enthused", "entire", "exact",
	"experimental", "fearful", "final",
	"following", "informative",
	"full", "complete", "indicated", "authorized",
	"modularized", "submodularized",
	"particular", "preferred", "satisfactory",
	"measurable", "referenced", "literal",
	"modified",
	"correct", "prioritized", "prolonged",
	"regrettable", "apparent",
	"continued", "subsequent", "sufficient",
	"suggestive", "true", "ultimate", "separate",
	"purposeful", "regarded", "resulting",
	"doubtful", "evident", "interesting", "worthy",
	"uniform", "vital", "viable",
	"worthwhile", "alternative",
	"sophisticated", "employed",
	"clear", "lucid", "simple", "perspicuous",
	"incomplete", "concerned"};

X 
noun(env)
	E               env;
{
	static char    *suff[] = {"ion", "sion", "tion", "age",
		"ness", "ment", "ure",
		"ity", "iety", "ty", "ence", "ency", "ance",
		"ancy", "tude", "hood", "ture", "ate", "art", "ard",
		"ism", "ine", "stress", "trix", "ess",
		"dom", "ship", "eer", "ster", "ant", "ent", "ary",
		"ery", "ory", "ette", "let", "ling", "ule", "kin",
		"ar", "or", "ist", "fulness",
		"kin", "cule", "icle", "y", "ability", "iosos"};
	X               nounv = getxx();
	int             i = 0;
	nounv->type = "-noun";
	if (env->number == 0)
		env->number = number();
	if (prob(makeup)) {
		if (prob(0.05)) {
			nounv->list.s[i++] = CHOOSE(adjlist);
			nounv->list.s[i++] = "ness";
		} else
			nounv->list.s[i++] = CHOOSE(nounlist);
	} else {
		nounv->list.s[i++] = prefix();
		nounv->list.s[i++] = root();
		nounv->list.s[i++] = CHOOSE(suff);
	}
	if (EQ(env->number, "plural")) {
		if (LAST(nounv->list.s[i - 1]) == 's')
			nounv->list.s[i] = "es";
		else if (LAST(nounv->list.s[i - 1]) == 'y')
			nounv->list.s[i] = "ies";
		else
			nounv->list.s[i] = "s";
	}
	return nounv;
}

X 
adjval(void) {
	X               adjvalv = getxx();
	int             i = 0;
	adjvalv->type = "adjval";
	if (prob(0.25)) {
		adjvalv->list.x[i++] = adverb();
	}
	do {
		adjvalv->list.x[i++] = adjective();
	} while (i < N - 1 && prob(0.25));
	return adjvalv;
}

X 
nounal(env)
	E               env;
{
	X               nounalv = getxx();
	int             i = 0;
	X               p;
	nounalv->type = "nounal";
	if (prob(0.15)) {
		nounalv->list.x[i++] = adjval();
	}
	nounalv->list.x[i++] = noun(env);
	if (prob(0.15)) {
		nounalv->list.x[i] = adjph(env);
	}
	env->an = "a";
	for (p = nounalv; p->type[0] != '-'; p = p->list.x[0]);
	for (i = 0; p->list.s[i]; i++) {
		if (p->list.s[i][0] == 0)
			continue;
		if (VOWEL(p->list.s[i][0])) {
			env->an = "an";
		}
		break;
	}
	return nounalv;
}

char           *
prefix(void) {
	static char    *pref[] = {
		"amb", "ambi", "super", "hyper", "an", "tra", "trans", "post", "palim",
		"omni", "pan", "circ", "circum", "peri", "a", "ab", "abs", "de", "apo",
		"re", "ana", "mal", "ante", "pre", "fore", "pro", "infra", "para",
		"inter", "ultra", "extra", "trans", "cata", "de", "oct", "octa",
		"octo", "equi", "pseudo", "prim", "prot", "proto", "pent", "penta",
		"quin", "quint", "quinque", "pro", "tetr", "tetra", "quad", "quadr",
		"quadri", "quartet", "off", "bene", "hemi", "demi", "semi", "crypto",
		"cent", "centi", "hecto", "en", "em", "in", "im", "intro", "be",
		"macro", "poly", "mult", "multi", "neo", "nona", "novem", "ennea",
		"in", "un", "im", "il", "ir", "non", "a", "nil", "paleo", "mon", "mono",
		"uni", "e", "ex", "ec", "ef", "super", "supr", "sur", "hyper", "vic",
		"vice", "hept", "hepta", "sept", "septe", "septem", "septi", "hex",
		"hexa", "sex", "dis", "deca", "deka", "deci", "kilo", "mill", "milli",
		"tri", "per", "dia", "ad", "com", "di", "amphi", "bi", "bin", "bis",
	"sub", "hypo", "epi", "eu", "holo"};
	if (prob(0.65))
		return "";
	return CHOOSE(pref);
}

char           *
root(void) {
	static char    *root[] = {
		"pan", "omni", "arch", "zo", "rog", "rogat", "cred", "flect", "flex",
		"test", "hem", "hemato", "nasc", "nat", "bibl", "fer", "voc", "port", "lat",
		"fortuna", "ped", "chrom", "vinc", "vict", "crea", "cise", "mort", "mors",
		"necr", "claim", "clam", "hetero", "pel", "puls", "vac", "iso", "phobe",
		"phobia", "prim", "prime", "flu", "flux", "sequ", "liber", "liver", "theo",
		"magna", "medi", "man", "manu", "pen", "pend", "pens", "eu", "capit",
		"iatr", "aud", "aus", "cor", "cord", "cour", "grav", "ten", "tain",
		"tent", "sacr", "sacer", "heiro", "sanct", "cide", "mega", "ultima",
		"ridi", "risi", "leg", "jus", "jur", "nom", "duc", "duct", "duce",
		"bio", "viv", "vivi", "vita", "lus", "lum", "luc", "photo",
		"min", "philo", "phile", "phila", "amic", "anthrop", "poly", "multi",
		"fac", "fact", "fic", "fect", "meter", "psych", "mod", "mot", "mov",
		"nov", "neo", "neg", "uni", "alter", "ali", "idio", "pop", "dem",
		"demo", "lic", "licit", "poten", "posse", "potes", "mem", "simul",
		"arch", "homo", "mar", "mer", "vis", "vid", "scope", "auto", "mitt",
		"miss", "ac", "acr", "brev", "clud", "clus", "dorm", "micro", "aster",
		"astro", "rect", "recti", "forc", "fort", "path", "cap", "cep", "cept",
		"put", "tempo", "tempor", "dent", "dont", "ver", "veri",
		"feder", "fide", "feal", "fid", "cosm", "migra", "hydro", "aqu",
		"endo", "gyn", "logo", "opus", "oper", "graph", "scrib", "scrip",
	"mis", "miso", "anni", "annu", "enni", "ced", "cede", "ceed", "cess"};
	return CHOOSE(root);
}

prob(f)
	double          f;
{
	return (R) < (f * 32767.0);
}

char           *
tense()
{
	return prob(0.5) ? "pres" : "past";
}

char           *
number()
{
	return prob(0.25) ? "plural" : "sing";
}

X 
getxx()
{
	X               rv;
	static XX       empty;

	rv = (X) malloc(sizeof *rv);
	if (rv == 0) {
		fprintf(stderr, "festoon: outa space\n");
		exits("space");
	}
	*rv = empty;
	return rv;
}

X 
verbal(env)
	E               env;
{
	X               verbalv = getxx();
	int             i = 0;
	verbalv->type = "verbal";
	if (prob(0.25))
		verbalv->list.x[i++] = adverb();
	verbalv->list.x[i] = verb(env);
	return verbalv;
}

static char    *advlist[] = {"absolutely", "functionally",
	"accordingly", "broadly", "actionably", "actually",
	"additionally",
	"ambiguously", "amply",
	"analogously",
	"aperiodically",
	"apparently", "appreciably",
	"appropriately", "approximately",
	"arbitrarily",
	"associatively",
	"automatically",
	"awfully",
	"axiomatically",
	"badly", "barely", "basically",
	"beneficially",
	"blatantly",
	"capably", "carefully", "carelessly",
	"casually", "causally", "cautiously",
	"centrally", "certainly",
	"cheaply", "cleanly",
	"closely", "coarsely", "cognizantly",
	"coincidentally", "collectively", "collaterally",
	"comparably",
	"competently", "completely", "comprehensibly",
	"concededly", "conceivably",
	"concisely", "conclusively", "concretely",
	"concurrently", "conjecturally",
	"currently",
	"conscientously", "consequently", "consequentially",
	"consistently", "constantly",
	"contemporaneuosly", "constructively",
	"continually", "continuously", "contractually",
	"contrarily", "contributatively", "conveniently",
	"conventionally",
	"correctively",
	"correctly",
	"crudely",
	"curiously",
	"decidedly",
	"deeply",
	"deficiently", "demandingly",
	"dependably", "desireably",
	"determinately", "diagnostically",
	"differentially", "differently",
	"directly", "discernibly",
	"distinctly", "doubtfully", "dramatically",
	"dynamically",
	"economically",
	"effecaciously", "efficiently",
	"elegantly",
	"emphatically", "encouragingly",
	"endlessly", "endurably",
	"entirely", "epistomologically",
	"functionally", "immediately",
	"equably", "equally", "equitably", "erroneously",
	"esoterically", "eternally", "evenly", "eventfully",
	"eventually", "evidently",
	"exceedingly", "exactly", "excellently",
	"exceptionally", "excessively", "exclusively",
	"experimentally",
	"explicitly", "extremely",
	"factually", "faithfully",
	"faultlessly", "feasibly",
	"finitely", "firmly", "forcefully",
	"formally", "formerly", "frankly", "freely",
	"frugally", "fully", "generally",
	"globally", "gradually",
	"harmlessly",
	"helpfully",
	"highly", "homogeneously",
	"hopefully",
	"ideally", "identically", "ideologically",
	"idiomatically", "idiosyncratically", "idly",
	"imaginably", "immaterially", "immensely",
	"impartially", "imperceptably", "imperfectly", "importantly",
	"improperly", "imprudently", "inaccurately", "inappropriately",
	"accurately",
	"inclusively", "incompletely", "incorrectly",
	"increasingly", "independently",
	"indirectly", "ineffectively", "ineffectually", "inefficiently",
	"infallibly", "instantaneously", "instantly",
	"insufficiently", "internally", "likely", "only",
	"invaluably", "inversely", "irrelevantly", "irrespectively",
	"largely", "lastly", "legitimately", "literally",
	"locally", "loosely", "manageably", "markedly",
	"memorably", "mildly", "mindfully", "moderately",
	"momentarily", "naturally", "needfully", "needlessly",
	"nominally", "normally", "objectively", "occasionally",
	"temporarily",
	"officially", "oppositely", "ordinarily", "ostensibly",
	"partially", "permissibly",
	"personally", "pertinently",
	"physically", "plainly", "plainly",
	"pleasingly", "politically",
	"potentially", "predictively",
	"predominantly", "prematurely", "preparedly", "presently",
	"previously", "primarily",
	"primely", "principally", "problematically",
	"productively", "promptly", "proportionately",
	"provably", "purely", "quickly", "radically", "randomly", "recently",
	"repeatedly", "secondarily", "separately",
	"usually", "specifically",
	"redundantly", "regardlessly", "reliably",
	"remarkably", "remotely", "respectively",
	"probably",
	"robustly", "seemingly",
	"sensibly", "singularly", "steadily",
	"strikingly", "substantially", "successfully",
	"supposedly", "systematically", "understandably",
	"necessarily", "unfortunately",
	"unnecessarily", "unmistakably", "usefully", "weakly"};
	
X 
adverb(void) {
	static char    *wordy[] = {"very ", "extremely ", "generally ",
		"reasonably ", "fundamentally ", "essentially ", "particularly ",
		"very ",
		"very ", "very ",
		"very ", "very ",
		"very ", "very ",
		"very ", "very ",
		"very ", "very ",
		"very ", "very ",
		"very ", "very ",
		"entirely ",
		"rather ", "fairly ", "relatively ", "comparatively ",
		"moderately ",
		"totally ", "very ", "quite "};
	static char    *suff[] = {"wardly", "ably", "wisely",
		"ably", "ily", "ly", "ly", "ly"};

	X               adverbv = getxx();
	int             i = 0;

	adverbv->type = "-adverb";
	if (prob(0.150)) {
		adverbv->list.s[i] = prob(.5) ? "simply" : "easily";
		return adverbv;
	}
	if (prob(0.4))
		adverbv->list.s[i++] = CHOOSE(wordy);
	if (prob(makeup))
		adverbv->list.s[i] = CHOOSE(advlist);
	else {
		adverbv->list.s[i++] = prefix();
		adverbv->list.s[i++] = root();
		adverbv->list.s[i] = CHOOSE(suff);
	}
	return adverbv;
}

X 
adjective(void) {
	static char    *suff[] = {"ive", "ful", "ous", "some", "oid",
	"ine", "esque", "en", "an",
	"ile", "able", "ible", "istic", "ic",
	"an", "ian", "ish", "ite", "al", "less"};
	X               adjv = getxx();
	int             i = 0;

	adjv->type = "-adjective";
	if (prob(0.2)) {
		adjv->list.s[i++] = "not ";
		adjv->list.s[i++] = "un";
	}
	if (prob(makeup)) {
		adjv->list.s[i] = CHOOSE(adjlist);
		return adjv;
	}
	adjv->list.s[i++] = prefix();
	adjv->list.s[i++] = root();
	adjv->list.s[i] = CHOOSE(suff);
	return adjv;
}

X 
adjph(env)
	E               env;
{
	X               adjv = getxx();
	EE              nenv;
	static EE       empty;
	int             i = 0;
	adjv->type = "adjph";
	if (prob(0.25)) {
		nenv = *env;
		nenv.tense = 0;
		adjv->list.x[i++] = rel();
		adjv->list.x[i++] = aux(&nenv);
		adjv->list.x[i] = vp(&nenv);
		return adjv;
	}
	nenv = empty;
	adjv->list.x[i++] = prep();
	adjv->list.x[i] = np(&nenv);
	return adjv;
}

static char    *preplist[] = {"across", "by", "in", "of",
	"near", "under", "over",
	"in back of", "below", "behind", "of", "of", "of", "of",
	"centered around", "centered about",
	"in close proximity to", "following after",
	"in between", "in conflict with", "in conjunction with",
	"in the area of", "in the neighborhood of", "in the proximity of",
	"in the field of", "for the purpose of",
	"giving rise to", "based upon", "being caused by",
	"of", "of", "of", "of",
	"being effectuated by", "being aggrevated by",
	"being used with",
	"being collected together with", "being combined together with",
	"connected up to", "exhibiting a tendency towards",
	"being facilitated by",
	"being employed with",
	"having a deleterious effect upon", "impacting",
	"being joined together with", "being merged together with",
	"in the vicinity of"};

X 
prep(void) {
	X               pv = getxx();
	pv->type = "-prep";
	pv->list.s[0] = CHOOSE(preplist);
	return pv;
}

X 
comp(env)
	E               env;
{
	X               v = getxx();
	EE              nenv;
	static EE       empty;
	int             i = 0;
	nenv = empty;
	v->type = "comp";
	if (0 && prob(0.001))
		v->list.x[i++] = adjective();
	else if (prob(0.1))
		v->list.x[i++] = advp(&nenv);
	else {
		if (env->passive)
			v->list.x[i++] = passprep();
		v->list.x[i++] = np(&nenv);
		env->passive = 0;
	}
	if (0 && prob(0.05))
		v->list.x[i++] = adverb();
	USED(i);
	return v;
}

X 
advp(env)
	E               env;
{
	X               v = getxx();
	v->type = "advp";
	v->list.x[0] = vprep();
	v->list.x[1] = np(env);
	return v;
}

static char    *vpreplist[] = {"to", "at", "by", "from", "with", "for"};

X 
vprep(void) {
	X               v = getxx();
	v->type = "-vprep";
	v->list.s[0] = CHOOSE(vpreplist);
	return v;
}

E 
getenvq()
{
	static EE       empty;
	E               v;
	v = (E) malloc(sizeof *v);
	if (v == 0) {
		printf("outa room\n");
		exits("room");
	}
	*v = empty;
	return v;
}

X 
comma(env)
	E               env;
{
	X               v = getxx();
	static EE       empty;

	v->type = "-comma";
	v->list.s[0] = ",";
	*env = empty;
	return v;
}

static char    *conjadvlist[] = {"we believe", "naturally", "therefore",
	"moreover", "obviously"};

X 
conjadv(void) {
	X               v = getxx();

	v->type = "-conjadv";
	v->list.s[0] = CHOOSE(conjadvlist);
	return v;
}

static char    *lconjlist[] = {"therefore", "however", "nevertheless",
	"consequently", "also", "in addition", "moreover",
	"accordingly", "essentially", "presumably", "actually",
	"basically", "importantly", "clearly", "obviously",
	"needless to say", "as already stated",
	"generally", "approximately", "presently",
	"hopefully", "usually", "in the great majority of cases",
	"seen in the above light", "most significantly",
	"when the need arises",
	"in a large number of cases", "after this is accomplished",
	"in all cases",
	"having been made aware concerning these matters",
	"as an example of this", "as a consequence of this",
	"as a matter of fact", "as is often the case",
	"as of this date", "assuming that this is the case",
	"at the present moment in time", "at this time",
	"as a consequent result of this", "as a desireable benefit of this",
	"if at all possible", "similarly", "in the same connection",
	"in large measure", "in many cases", "in rare cases",
	"in some cases", "in the interim", "in the last analysis",
	"in light of these facts", "in the majority of instances",
	"in the not too distant future", "in the same way as described above",
	"in this case", "for all intents and purposes",
	"to arrive at an approximation", "for this reason",
	"for many reasons, then",
	"as is often the case", "last but not least",
	"later on", "on a few occasions", "on this occasion",
	"in summary", "taking this into consideration",
	"with this in mind",
	"substantially", "ultimately"};

X 
lconjadv(void) {

	X               v = getxx();
	v->type = "-lconjadv";
	v->list.s[0] = CHOOSE(lconjlist);
	return v;
}

X 
conjsub(void) {
	static char    *conjsublist[] = {"although", "even though",
		"despite the fact that",
		"for the simple reason that",
		"because", "due to the fact that", "since",
		"whether or not",
		"inasmuch as",
	"as"};
	X               v = getxx();
	v->type = "-conjsub";
	v->list.s[0] = CHOOSE(conjsublist);
	return v;
}

static char    *lconjsublist[] = {"although", "even though",
	"despite the fact that",
	"because", "due to the fact that", "since",
	"if", "anytime that", "in the case that",
	"as a consequence of the fact that",
	"as regards the fact that",
	"as a desireable benefit of the fact that",
	"with reference to the fact that",
	"as long as",
	"as an important essential of the fact that",
	"in conjunction with the fact that",
	"in the light of the fact that",
	"if", "if", "if", "if",
	"leaving out of consideration the fact that",
	"just as",
	"inasmuch as", "until such time as",
	"as soon as", "being as", "in the same way as",
	"with the exception of the fact that",
	"notwithstanding the fact that",
	"on the grounds that",
	"on the basis of the fact that",
	"persuant to the fact that",
	"although it seems apparent that",
	"with regard to the fact that",
	"as can be seen from the fact that",
	"as"};

X 
lconjsub(void) {
	X               v = getxx();
	v->type = "-lconjsub";
	v->list.s[0] = CHOOSE(lconjsublist);
	return v;
}

static char    *conjlist[] = {"and", "but", "yet", "and", "and"};

X 
conj(void) {
	X               v = getxx();
	v->type = "-conj";
	v->list.s[0] = CHOOSE(conjlist);
	return v;
}
static char    *nomzlist[] = {"it is easy to see that",
	"it is a basic fact that",
	"it is obvious that", "it is not unimportant that",
	"it is easy to overlook the fact that",
	"it is within the realm of possibility that",
	"it is apparent that",
	"this is indicitive of the fact that",
	"this is in substantial agreement with the fact that",
	"this demonstrates the fact that",
	"this leaves out of consideration the fact that",
	"it is of the utmost importance that",
	"the truth is that",
	"the fact is that",
	"it turns out that", "it will turn out to be true that",
	"it should be noted that",
	"it stands to reason that",
	"it would not be unreasonable to assume that",
	"it is interesting to note that",
	"this can be proved by:",
	"this is a trivial consequence of",
	"it is assumed that",
	"it remains to be shown that",
	"it is left to the reader to prove"
};

X 
nomz(void) {
	X               v = getxx();
	v->type = "-nomz";
	v->list.s[0] = CHOOSE(nomzlist);
	return v;
}

X
equation(void) {
	X		v = getxx();
	static char eqnbuff[100], x;
	static char *eqnelem[] = {"int", "sum", "prod", "union", "inter"};
	static char *eqnfn[] = { "sin", "cos", "tan", "arc", "det",
		"log", "exp", "f", "g", "sinh", "O", "J sub 0", "J sub 1",
		"P sub i", "gamma", "zeta" };
	static char *eqnval[] = { "0", "DELTA", "GAMMA", "LAMBDA",
		"OMEGA", "PHI", "PSI", "SIGMA", "THETA", "UPSILON",
		"XI", "alpha", "beta", "gamma", "delta", "epsilon",
		"eta", "kappa","lambda", "mu", "omega", "x", "zeta", "inf"};
	static char *eqnrel[] = {"=", "<=", ">=", "==", "!=", "approx"};

	x = 'a' + (R)%26;
	v->type = "-eqn";
	sprintf(eqnbuff,"$%s from %c=%d to %s %s ( %c ) d%c %s %s$",
		CHOOSE(eqnelem), x, (R)&077, CHOOSE(eqnval), CHOOSE(eqnfn),
		x, x, CHOOSE(eqnrel), CHOOSE(eqnval));
	v->list.s[0] = eqnbuff;
	return v;
}

X 
turgid(env)
	E               env;
{
	X               v = getxx();
	int             i = 0;

	v->type = "turgid";
	if (prob(T * 1.5)) {
		v->list.x[i++] = lconjadv();
		v->list.x[i++] = comma(env);
		v->list.x[i++] = sent(env);
	} else if (prob(2 * T)) {
		v->list.x[i++] = turgid(env);
		v->list.x[i++] = comma(env);
		v->list.x[i++] = conj();
		v->list.x[i++] = sent(env);
	} else if (prob(1.5 * T)) {
		v->list.x[i++] = lconjsub();
		v->list.x[i++] = sent(env);
		v->list.x[i++] = comma(env);
		v->list.x[i++] = sent(env);
	} else if (prob(T * .5)) {
		v->list.x[i++] = sent(env);
		v->list.x[i++] = comma(env);
		v->list.x[i++] = conjadv();
	} else if (prob(T)) {
		v->list.x[i++] = turgid(env);
		v->list.x[i++] = comma(env);
		v->list.x[i++] = conjsub();
		v->list.x[i++] = sent(env);
	} else if (prob(.5 * T)) {
		v->list.x[i++] = nomz();
		if (eqn && prob(.5)) {
			v->list.x[i++] = equation();
			v->list.x[i++] = comma(env);
			v->list.x[i++] = conj();
		}
		v->list.x[i++] = sent(env);
	} else
		v->list.x[i++] = sent(env);
	USED(i);
	return v;
}

char *
splitup(char *strlab)  {
	static char label[64];
	int j, c;

	label[0]='"';
	for (j=1; j<60 &&
		(c = *strlab++) != '\0'; j++)
		if (c == ' ') {
			label[j++]='"';
			label[j++]=' ';
			label[j]='"';
		}
		else
			label[j] = c;
	label[j++] = '"'; label[j] = '\0';
	return(label);
}

void
abo(void) {
	fprintf(stderr, "usage: festoon [-pet] [-sSEED] [SENTENCES] [%%-invented-nouns]\n");
	exits("usage");
}

void
caps(void) {
	int             i;
	for (i = 1; i < io; i++)
		if (buff[i - 1] == ' ' && buff[i] <= 'z' && buff[i] >= 'a')
			buff[i] += 'A' - 'a';
}

void
main(int argc, char *argv[]) {
	static char    *furniture[] = {"WASTEBASKET", "ASHTRAY", "TABLE",
	"DESK DRAWER", "COAT LOCKER", "BOOKSHELF"};

	static char    *ccto[] = {
		"J. N. Akkerhuis",
		"J. J. Argosy",
		"M. D. Banal",
		"H. V. Bandersnatch",
		"L. Bimmler",
		"F. W. Blivet",
		"Z. Brazen",
		"M. Bushido",
		"J. D. Carbuncle",
		"N. Crab",
		"R. H. deTruckle",
		"R. L. Drechsler",
		"C. B. Dudgeon",
		"R. T. Dun",
		"G. R. Emlin",
		"W. G. Fallow",
		"R. S. Flummox",
		"R. N. Fribble",
		"C. R. Glitch",
		"R. H. Hardin",
		"S. A. Hobble",
		"B. W. Kernighan",
		"D. B. Knudsen",
		"C. L'Hommedieu",
		"R. S. Limn",
		"S. T. Livid",
		"Mrs. B. R. Mauve",
		"C. Mee",
		"N-P. Nelson",
		"C. H. Russet",
		"M. Shayegan",
		"M. H. Simper",
		"B. R. Sorrel",
		"G. Swale",
		"R. R. Swarthy",
		"P. Terra-Cotta",
		"U. G. Winnow"};

	static char	*picelem[] = { "box", "ellipse", "box", "box"};
	static char	*piccon[] = { "arrow", "line", "line <-", "line <->",
	"spline", "spline <-", "spline <->"};
	static char	*picdir[] = { "right", "down right", "down",
	"left", "up left", "left", "down", "down right", NULL};
	E               env;
	X               tree;
	int             i, j = 0, k = 0;
	int             lim = 0;
	long            t = 0;
	char            c, **str;
	int		junk, junk2;

	for (i = 1, ++argv; i < argc; i++, argv++)
		if (*argv[0] == '-')	/* -pet -snnn */
			while (c = *++argv[0])
				switch (c) {
				case 'z':
					flag = 1;
					continue;
				case 'p':
					pic = 1;
					continue;
				case 't':
					tbl = 1;
					continue;
				case 'e':
					eqn = 1;
					continue;
				case 's':
				t = atoi(argv[0]+1); argv[0][1] = '\0';
					continue;
				default:
					abo();	/* illegal option */
				}
		else if (lim == 0)
			lim = atoi(argv[0]);
		else
			makeup = 1.0 - (float) atoi(argv[0]) / 100.0;
	USED(i);
	if (t == 0)
		time(&t);
	if (makeup < 0. || makeup > 1.0)
		makeup = 1.0;
	if (lim <= 0)
		lim = 25;
	srand((int) t);

	printf(".TL\n");
	env = getenvq();
	tree = np(env);
	io = 0;
	pr(tree);
	buff[io] = 0;
	caps();
	printf("%s\n", buff);
	printf(".AU \"C. C. Festoon\" CCF Headquarters %d\n", t);
	if (eqn)
		printf(".EQ\ndelim $$\n.EN\n");
	printf(".AS\n");
	free(env);
	do {
		env = getenvq();
		tree = turgid(env);
		io = 0;
		pr(tree);
		buff[io] = 0;
		printf("%s.\n", buff);
		free(env);
	} while (prob(0.75));
	printf(".AE\n");
	printf(".MT \"MEMORANDUM FOR %s\"\n.hy 1\n",
	       CHOOSE(furniture));
	while (i++ < lim) {
		if (i % 23 == 0) {	/* Time for a section header */
			env = getenvq();
			tree = np(env);
			io = 0;
			printf(".H 1 \"");
			pr(tree);
			buff[io] = 0;
			caps();
			printf("%s\"\n", buff);
			free(env);
		}
		if (i % 27 == 0 && pic) {	/* Time for a picture */
			printf(".DS CB\n.ps -1\n.PS\n");
			str = &(CHOOSE(picdir));
			if (*str == NULL) str = &picdir[0];
 			junk2 = (R&07) + 3;
			for(junk = 1; junk < junk2; junk++) {
				printf("%s; ", *str);
				if (str == &picdir[0]) {
					pic = 2; printf("A: ");
				}
				printf("%s %s ht %3.1f wid %3.1f\n",
				CHOOSE(picelem), splitup(CHOOSE(nounlist)),
				0.4+0.5/junk2, 0.8+0.6/junk2);
				printf("%s %s %3.1f ",
				CHOOSE(piccon), *str, 0.2+.3/junk2);
				if (*++str == NULL) str = &picdir[0];
				printf("then %s %3.1f %s\n",
				*str, 0.3+.2/junk2,
				splitup(CHOOSE(adjlist)));
			}
			printf("circle rad .3 \"process\" \"completion\"\n");
			if (pic == 2) {
				pic =1;
				printf("line <- dashed up .25 from A.n\n");
				printf("circle rad .3 \"process\" \"start\"\n");
			}
			printf(".PE\n.ps +1\n.DE\n");
			printf(".ce\n\\fBFigure %d\\fP\n", i/27);
		}
		if (i % 41 == 0 && tbl) {	/* Time for a table */
			printf(".TS\n");
			printf("box, center;\nc\ts\ts\n");
			printf("n | l | lw(%3.1fi).\n", 2.0+(41.0+(t&07))/i);
			printf("Action Plan %d\n=\n", i);
			printf("Item\tWho\tAction\n");
			for (junk = 1; junk < (i&17)+4; junk++) {
				printf("_\n%d\t", t/i+junk);
				printf("%s\tT{\n", CHOOSE(ccto));
				env = getenvq();
				io = 0;
				tree = sent(env);
				pr(tree);
				buff[io] = 0;
				printf("%s.\nT}\n", buff);
				free(env);
			}
			printf(".TE\n");
			printf(".ce\n\\fBTable %d\\fP\n", i/41);
		}
		env = getenvq();
		tree = turgid(env);
		io = 0;
		pr(tree);
		buff[io] = 0;
		if (++k % 13 == 0 && prob(0.35)) {	/* Bullet list */
			printf("%s:\n", buff);
			printf(".BL\n");
			do {
				printf(".LI\n");
				free(env);
				env = getenvq();
				io = 0;
				tree = sent(env);
				pr(tree);
				buff[io] = 0;
				printf("%s.\n", buff);
			} while (prob(.83));
			printf(".LE\n");
			printf(".P\n");
		} else {
			if (k % 11 == 0 && prob(.21)) {	/* do as footnote */
				printf("%s\\*F.\n", buff);
				free(env);
				env = getenvq();
				io = 0;
				tree = sent(env);
				pr(tree);
				buff[io] = 0;
				printf(".FS\n%s.\n.FE\n", buff);
			}
			else printf("%s.\n", buff);	/* normal flush */
		}
		if (++j > 2 && prob(0.4))
			printf(".P\n"), j = 0;
		free(env);
	}
	USED(i);
	printf(".SG\n");
	printf(".NS 0\n");
	for (j = 0; j == 0;) {
		for (i = 0; i < sizeof ccto / sizeof *ccto; i++) {
			if (prob(.10))
				j = 1, printf("%s\n", ccto[i]);
		}
	}
	printf(".NE\n");
	USED(i);
	exits("");
}

void
pr(tree)
	X               tree;
{
	int             i;
	if (flag ) {
		out("<");
		out(tree->type);
		out(">");
	}
	if (tree->type[0] == '-') {
		out(" ");
		for (i = 0; tree->list.s[i]; i++) {
			out(tree->list.s[i]);
		}
	} else
		for (i = 0; tree->list.x[i]; i++) {
			pr(tree->list.x[i]);
		}
	free(tree);
	USED(i);
	return;
}

void
out(s)
	char           *s;
{
	if (io == 0 && *s == ' ')
		return;
	if (io == 0) {
		for (; s[io]; io++)
			buff[io] = s[io];
		buff[0] += 'A' - 'a';
		return;
	}
	if ((buff[io - 1] == ' ' || buff[io - 1] == '\n' ) && *s == ' ')
		return;
	if (buff[io - 1] == ' ' && *s == ',') {
		buff[io - 1] = ',';
		buff[io++] = '\n';
		return;
	}
	if (buff[io - 1] == 'y' && *s == 'i' && s[1] == 'e')
		io--;
	else if (*s == buff[io - 1] && *s != 's' && *s != 'n')
		io--;
	else if (*s == 'e' && buff[io - 1] == 'a')
		io--;
	for (; *s;)
		buff[io++] = *s++;
	return;
}
