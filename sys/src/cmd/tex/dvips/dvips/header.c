/*
 *   This routine handles the PostScript prologs that might
 *   be included through:
 *
 *      - Default
 *      - Use of PostScript fonts
 *      - Specific inclusion through specials, etc.
 *      - Use of graphic specials that require them.
 *
 *   Things are real simple.  We build a linked list of headers to
 *   include.  Then, when the time comes, we simply copy those
 *   headers down.
 */
#include "structures.h" /* The copyright notice in that file is included too! */
struct header_list *header_head ;
/*
 *   The external routines we use.
 */
extern char *newstring() ;
extern void error() ;
extern void copyfile() ;
extern FILE *search() ;
extern long lastheadermem ;
extern char errbuf[] ;
extern integer fontmem, swmem ;
extern char *headerpath ;
#ifdef DEBUG
extern integer debug_flag ;
#endif
/*
 *   This more general routine adds a name to a list of unique
 *   names.
 */
int
add_name(s, what)
   char *s ;
   struct header_list **what ;
{
   struct header_list *p, *q ;

   for (p = *what ; p != NULL; p = p->next)
      if (strcmp(p->name, s)==0)
         return 0 ;
   q = (struct header_list *)mymalloc((integer)sizeof(struct header_list)
                                          + strlen(s)) ;
   q->next = NULL ;
   strcpy(q->name, s) ;
   if (*what == NULL)
      *what = q ;
   else {
      for (p = *what; p->next != NULL; p = p->next) ;
      p->next = q ;
   }
   return 1 ;
}
/*
 *   This routine is responsible for adding a header file.  We also
 *   calculate the VM usage.  If we can find a VMusage comment, we
 *   use that; otherwise, we use the length of the file.
 */
int
add_header(s)
char *s ;
{
   int r ;

   r = add_name(s, &header_head) ;
   if (r) {
      FILE *f = search(headerpath, s, READ) ;

      if (f==0) {
         (void)sprintf(errbuf, "! Couldn't find header file %s", s) ;
         error(errbuf) ;
      } else {
         int len, i, j ;
         long mem = -1 ;
         char buf[1024] ;

         len = fread(buf, sizeof(char), 1024, f) ;
         for (i=0; i<len-20; i++)
            if (buf[i]=='%' && strncmp(buf+i, "%%VMusage:", 10)==0) {
               if (sscanf(buf+i+10, "%d %ld", &j, &mem) != 2)
                  mem = -1 ;
               break ;
            }
         if (mem == -1) {
            mem = 0 ;
            while (len > 0) {
               mem += len ;
               len = fread(buf, sizeof(char), 1024, f) ;
            }
         }
         if (mem < 0)
            mem = DNFONTCOST ;
         fclose(f) ;
         lastheadermem = mem ;
#ifdef DEBUG
         if (dd(D_HEADER))
            (void)fprintf(stderr, "Adding header file \"%s\" %ld\n",
                                   s, mem) ;
#endif
         fontmem -= mem ;
         if (fontmem > 0) /* so we don't count it twice. */
            swmem -= mem ;
      }
   }
   return r ;
}
/*
 *   This routine runs down a list, returning each in order.
 */
char *
get_name(what)
   struct header_list **what ;
{
   if (what && *what) {
      char *p = (*what)->name ;
      *what =  (*what)->next ;
      return p ;
   } else
      return 0 ;
}
/*
 *   This routine actually sends the headers.
 */
void
send_headers() {
   struct header_list *p = header_head ;
   char *q ;

   while (q=get_name(&p)) {
#ifdef DEBUG
      if (dd(D_HEADER))
         (void)fprintf(stderr, "Sending header file \"%s\"\n", q) ;
#endif
      copyfile(q) ;
   }
}
