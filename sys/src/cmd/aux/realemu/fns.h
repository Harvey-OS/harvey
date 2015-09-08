/* arg */
Iarg *adup(Iarg *x);
Iarg *areg(Cpu *cpu, unsigned char len, unsigned char reg);
Iarg *amem(Cpu *cpu, unsigned char len, unsigned char sreg, unsigned long off);
Iarg *afar(Iarg *mem, unsigned char len, unsigned char alen);
Iarg *acon(Cpu *cpu, unsigned char len, unsigned long val);
unsigned long ar(Iarg *a);
long ars(Iarg *a);
void aw(Iarg *a, unsigned long w);

/* decode */
void decode(Iarg *ip, Inst *i);

/* xec */
void trap(Cpu *cpu, int e);
int intr(Cpu *cpu, int v);
int xec(Cpu *cpu, int n);

#pragma varargck type "I" Inst*
#pragma varargck type "J" unsigned long
#pragma varargck type "C" Cpu*

int instfmt(Fmt *fmt);
int flagfmt(Fmt *fmt);
int cpufmt(Fmt *fmt);

/* pit */
void clockpit(Pit *pit, long long cycles);
void setgate(Pit *ch, unsigned char gate);
unsigned char rpit(Pit *pit, unsigned char addr);
void wpit(Pit *pit, unsigned char addr, unsigned char data);
