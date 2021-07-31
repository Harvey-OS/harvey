void	xmlAmap(Hio *hout, AMap *v, char *tag, int indent);
void	xmlArena(Hio *hout, Arena *v, char *tag, int indent);
void	xmlIndex(Hio *hout, Index *v, char *tag, int indent);

void	xmlAName(Hio *hout, char *v, char *tag);
void	xmlScore(Hio *hout, u8int *v, char *tag);
void	xmlSealed(Hio *hout, int v, char *tag);
void	xmlU32int(Hio *hout, u32int v, char *tag);
void	xmlU64int(Hio *hout, u64int v, char *tag);

void	xmlIndent(Hio *hout, int indent);
