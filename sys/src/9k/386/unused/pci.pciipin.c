uchar
pciipin(Pcidev *pci, uchar pin)
{
	uchar intl;

	if (pci == nil)
		pci = pcilist;
	for (; pci; pci = pci->list)
		if (pcicfgr8(pci, PciINTP) == pin &&
		    pci->intl != 0 && pci->intl != 0xff)
			return pci->intl;
		else if (pci->bridge && (intl = pciipin(pci->bridge, pin)) != 0)
			return intl;
	return 0;
}

