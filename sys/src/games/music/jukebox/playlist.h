
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
