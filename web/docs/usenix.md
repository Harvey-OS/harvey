# Harvey at USENIX 2015

It's usenix and we have a BOF tomorrow night, so we have to get the minicluster going. Five AMD Persimmon boards in a stack and 1 Minnow MAX. 

We had really high hopes for the minnow max but it is a bit of a disappointment. Super neat size -- see the little blue box hanging from an ethernet cable in the 5th picture? It was kind of exciting to see so much in such a small box. 

But, it has uefi, surely the most unusable firmware out there. The UEFI on MInnow MAX somehow is less usable and harder to work with than firmware I was writing 30 years ago on 6809 CPUs ... it took us 15 minutes just to walk through enough of the unhelpful commands and unusable dialogues to realize we really only can boot from a FAT-formatted SD card. FAT formatted. 2015. What's wrong with this picture?

So then we got to PXE boot. See John taking a movie of the TV? It's because when UEFI pxeboot fails, it puts the failure frame up for 1/30 second and clears it. So John took a movie, and then we watched the movie frame by frame to see the error. But UEFI is so difficult to use on so many fronts that this is but one of many problems we faced. I can't wait to just scrape UEFI off the Minnow MAX and replace it with coreboot. But for now we've had to give up. The final straw, once we got the special version of GRUB booting over the LAN, was to have it tell us we wouldn't have a visible console. Why?

Of course, the docs note it comes with a serial port. Sadly, the cable to connect to the serial port is not included. And, you can't use said serial port and use the case -- I'll need to cut a hole in the case to use the serial port! Oh, well.

The AMD stack, with three coreboot nodes, worked better. The only thing that went wrong is that two nodes have AMI BIOS and that seems to give pxelinux.0 fits -- pxelinux.0 loads and gets to some point, instant reset. Ah well. ﻿

![mini cluster](img/usenix2015/mini-cluster.jpg)
Harvey mini-cluster ready to fire up (or catch fire)

![boffins to blame](img/usenix2015/boffins-to-blame.jpg)
John on the floor, trying to make the minnowmax boot
Harvey, Aki on the chair making Harvey build on a mac and boot off of one.

![video of error](img/usenix2015/video-of-error.jpg)
John decided taking a video of the boot sequence would
be the best way to capture the sub-second error message.
