/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <plan9.h>
#include <fcall.h>
#include <u9fs.h>

/*
 * return whether the user is authenticated.
 * uses berkeley-style rhosts ``authentication''.
 * this is only a good idea behind a firewall,
 * where you trust your network, and even then
 * not such a great idea.  it's grandfathered.
 */

static char*
rhostsauth(Fcall *rx, Fcall *tx)
{
	USED(rx);
	USED(tx);

	return "u9fs rhostsauth: no authentication required";
}

static char*
rhostsattach(Fcall *rx, Fcall *tx)
{
	USED(tx);

	if(ruserok(remotehostname, 0, rx->uname, rx->uname) < 0){
		fprint(2, "ruserok(%s, %s) not okay\n", remotehostname, rx->uname);
		return "u9fs: rhosts authentication failed";
	}
	return 0;
}

Auth authrhosts = {
	"rhosts",
	rhostsauth,
	rhostsattach,
};
