#ifdef VMS
/* The replacement ftell and fseek functions in the vaxvms.c file dated 
 * [02-Apr-87] do not work properly under OpenVMS 1.5 on DEC Alpha, when
 * compiled using DEC C 1.3. However, the DEC supplied ftell and fseek 
 * functions work fine. They should also work for DEC C on a VAX.
 */ 
#ifndef __DECC
#define ftell vms_ftell
#define fseek vms_fseek
#endif /* __DECC */
#define getchar vms_getchar
#define getenv vms_getenv
#define ungetc vms_ungetc
#define getname vms_getname
#endif /* VMS */
