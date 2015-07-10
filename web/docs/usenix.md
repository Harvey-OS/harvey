# Harvey at USENIX 2015

It's usenix and we have a BOF tomorrow night, so we have to get the
minicluster going. Five AMD Persimmon boards in a stack and 1 Minnow
MAX.

We had really high hopes for the minnow max but it is a bit of a
disappointment. Super neat size -- see the little blue box hanging
from an ethernet cable in the 5th picture? It was kind of exciting to
see so much in such a small box.

We've concluded that to make this board usable for Harvey we're going
to need to swap out UEFI for some other firmware. it took us 15
minutes just to walk through enough of the commands and dialogues to
realize we can only boot from a FAT-formatted SD card. FAT
formatted. 2015. What's wrong with this picture?

So then tried PXE boot. See John taking a movie of the TV? It's
because when UEFI pxeboot fails, it puts the failure frame up for 1/30
second and clears it. So John took a movie, and then we watched the
movie frame by frame to see the error. 

Then we hit the next problem: once we
got the special version of GRUB booting over the LAN, it told
us we wouldn't have a visible console. We need a console.

The AMD stack, with three coreboot nodes and two AMI BIOS nodes,
worked better. The only thing that went wrong is that the AMI BIOS
breaks pxelinux.0 -- it loads and gets to some point, and then instant
reset. We can still boot harvey on the AMI BIOS nodes, but only from a USB stick. 
We'll be reflashing these too. 

![mini cluster](img/usenix2015/mini-cluster.jpg)
Harvey mini-cluster ready to fire up (or catch fire)

![boffins to blame](img/usenix2015/boffins-to-blame.jpg)
John on the floor, trying to make the minnowmax boot
Harvey, Aki on the chair making Harvey build on a mac and boot off of one.

![video of error](img/usenix2015/video-of-error.jpg)
John decided taking a video of the boot sequence would
be the best way to capture the sub-second error message.
