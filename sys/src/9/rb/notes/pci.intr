	/*
	 * much of this is inspired by the freebsd driver,
	 * but is hard to understand due to lack of documentation.
	 *
	 * only pci irqs 0-2 are valid.
	 * 0 seems to yield INTR6.
	 * pci irqs 1-2 yield spurious ether intrs.
	 */
	i = 0;
	*Pciintrsts = 0;
	*Pciintrmask = 1 << i;
	junk = *Pciintrmask;		/* flush the write */
