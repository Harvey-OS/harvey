#ifndef  _PROC_H
#define _PROC_H

typedef int bool_t;

#define E_SCALL		101
#define E_IO		102
#define E_NO_MEM	103
#define E_FORK		104

extern bool_t arr2envblk(char **arr, char **envblk_out);

#endif
