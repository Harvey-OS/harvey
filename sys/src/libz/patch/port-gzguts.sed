# do not include stdio
/stdio.h/D

# do not try to define memory functions
/extern voidp  malloc OF((uInt size));/D
/extern void   free   OF((voidpf ptr));/D
/extern voidp  calloc OF((uInt items, uInt size));/D
