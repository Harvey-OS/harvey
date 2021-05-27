/* arg */
Iarg *adup(Iarg *x);
Iarg *areg(Cpu *cpu, uchar len, uchar reg);
Iarg *amem(Cpu *cpu, uchar len, uchar sreg, ulong off);
Iarg *afar(Iarg *mem, uchar len, uchar alen);
Iarg *acon(Cpu *cpu, uchar len, ulong val);
ulong ar(Iarg *a);
long ars(Iarg *a);
void aw(Iarg *a, ulong w);

/* decode */
void decode(Iarg *ip, Inst *i);

/* xec */
void trap(Cpu *cpu, int e);
int intr(Cpu *cpu, int v);
int xec(Cpu *cpu, int n);

#pragma varargck type "I" Inst*
#pragma varargck type "J" ulong
#pragma varargck type "C" Cpu*

int instfmt(Fmt *fmt);
int flagfmt(Fmt *fmt);
int cpufmt(Fmt *fmt);

/* pit */
void clockpit(Pit *pit, vlong cycles);
void setgate(Pit *ch, uchar gate);
uchar rpit(Pit *pit, uchar addr);
void wpit(Pit *pit, uchar addr, uchar data);
