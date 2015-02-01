/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */


typedef struct Playlistentry {
	char	*file;
	char	*onum;
} Playlistentry;

typedef struct Playlist {
	QLock;
	int		nentries;
	int		selected;
	Playlistentry	*entry;
} Playlist;

extern Playlist	playlist;
extern char	*playctlfile;
extern char	*srvmount;
extern int	playctlfd;

void	playctlproc(void*a);
void	playlistproc(void*);
void	playvolproc(void*a);
void	sendplayctl(char *fmt, ...);
void	sendplaylist(char*, char*);
void	setvolume(char *volume);
void	updateplaylist(int);
void	volumeproc(void *arg);
