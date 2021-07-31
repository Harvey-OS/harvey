#include "mplot.h"
void ppause(void){ 
	char	aa[4]; 
	fflush(stdout); 
	read(2, aa, 4);  
	erase(); 
}
