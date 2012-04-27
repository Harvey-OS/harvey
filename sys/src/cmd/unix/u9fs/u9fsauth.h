typedef struct Auth Auth;
struct Auth {
	char *name;

	char *(*session)(Fcall*, Fcall*);
	char *(*attach)(Fcall*, Fcall*);
};
