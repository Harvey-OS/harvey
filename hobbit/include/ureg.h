struct Ureg {
	ulong cause;
	ulong user;
	ulong id;
	ulong fault;
	union {
		ulong sp;
		ulong usp;
	};
	ulong msp;
	ulong pc;
	ulong psw;
};
