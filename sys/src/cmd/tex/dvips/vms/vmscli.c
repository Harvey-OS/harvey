#ifdef __GNUC__
#include "ssdef.h"
#else
#include ssdef
#endif
/* this code is included into dvips.c; don't compile it separately. */
int vmscli()
    {long int k,scnt;
    int length,status,maxdrift,jext;
    float offset,conversion;
    char *u;
    static char dummy[100],prtr[20];
    static $DESCRIPTOR(qual1,"copies");
    static $DESCRIPTOR(qual2,"debug");
    static $DESCRIPTOR(qual3,"maxdrift");
    static $DESCRIPTOR(qual4,"filter");
    static $DESCRIPTOR(qual5,"last");
    static $DESCRIPTOR(qual6,"manual");
    static $DESCRIPTOR(qual7,"number");
    static $DESCRIPTOR(qual8,"output");
    static $DESCRIPTOR(qual9,"start");
    static $DESCRIPTOR(qual10,"quiet");
    static $DESCRIPTOR(qual11,"reverse");
    static $DESCRIPTOR(qual12,"sav_res");
    static $DESCRIPTOR(qual13,"mode");
    static $DESCRIPTOR(qual14,"magnification");
    static $DESCRIPTOR(qual15,"collate");
    static $DESCRIPTOR(qual16,"dpi");
    static $DESCRIPTOR(qual17,"ctrld");
    static $DESCRIPTOR(qual18,"inc_com");
    static $DESCRIPTOR(qual19,"comment");
    static $DESCRIPTOR(qual20,"vm_save");
    static $DESCRIPTOR(qual21,"h_dpi");
    static $DESCRIPTOR(qual22,"v_dpi");
    static $DESCRIPTOR(qual23,"compress");
    static $DESCRIPTOR(qual24,"printer");
    static $DESCRIPTOR(qual25,"header");
    static $DESCRIPTOR(qual26,"absolute");
    static $DESCRIPTOR(qual27,"epsf");
    static $DESCRIPTOR(qual28,"prescan");
    static $DESCRIPTOR(qual29,"pagecopies");
    static $DESCRIPTOR(qual30,"separate");
    static $DESCRIPTOR(qual31,"cropmarks");
    static $DESCRIPTOR(qual32,"sec_size");
    static $DESCRIPTOR(qual33,"help");
    static $DESCRIPTOR(qual34,"makefont");
    static $DESCRIPTOR(qual35,"psize");
    static $DESCRIPTOR(qual36,"poffset");
    static $DESCRIPTOR(dumdum,dummy);
    static $DESCRIPTOR(dvi_file,"dvifile");
    static $DESCRIPTOR(input_file,infnme);
    scnt=0;
    status = cli$present(&dvi_file);

    if(status == CLI$_PRESENT)
    {status = cli$get_value(&dvi_file,&input_file,&jext);
      length=0;
      for(k=0;k<252;k++)
      { if(infnme[k] == ':' || infnme[k] == ']')
          scnt=k+1;
      }
      for(k=scnt;k<252;k++)
    {if(infnme[k]=='.')
      {length=k;
       jext=k;
       }
     else
      {if(infnme[k]=='/' || infnme[k]==' ' || infnme[k]=='\0')
         infnme[k]='\0';
       }
    }
    if(length==0)
     {jext= strlen(infnme);
     strcat(infnme,".DVI");}

    iname = &infnme[0] ;
    length = 0;         /* Initialize length...VAX C bug? */

   }
    status = cli$present(&qual1);

    if(status == CLI$_PRESENT | status == CLI$_DEFAULTED)
        {status = cli$get_value(&qual1,&dumdum,&length);

        if(sscanf(&dummy[0],"%d",&numcopies) == 0)
         error("!Bad copies option");
        }
#ifdef DEBUG
    status = cli$present(&qual2);

    if(status == CLI$_PRESENT)
        {
        status = cli$get_value(&qual2,&dumdum,&length);

        if(sscanf(&dummy[0],"%d",&debug_flag) == 0)
         error("!Bad debug option");
        }
#else
         error("not compiled in debug mode");
#endif
    status = cli$present(&qual3);

    if(status == CLI$_PRESENT)
        {status = cli$get_value(&qual3,&dumdum,&length);

        if(sscanf(&dummy[0],"%d",&maxdrift) == 0 ||maxdrift <0)
            error("bad maxdrift option");
        vmaxdrift=maxdrift;
        }


    status = cli$present(&qual5);

    if(status == CLI$_PRESENT)
        {status = cli$get_value(&qual5,&dumdum,&length);

#ifdef SHORTINT
            switch(sscanf(&dummy[0], "%ld.%ld", &lastpage, &lastseq)) {
#else	/* ~SHORTINT */
            switch(sscanf(&dummy[0], "%ld.%ld", &lastpage, &lastseq)) {
#endif	/* ~SHORTINT */
case 1:        lastseq = 0 ;
case 2:        break ;
default:
               error("! Bad last page option.") ;
            }
            notlast = 1 ;
        }
    status = cli$present(&qual6);

    if(status == CLI$_PRESENT)
     manualfeed=1;
     else
     if(status == CLI$_NEGATED)
      manualfeed=0;

    status = cli$present(&qual7);
     if(status == CLI$_PRESENT)
     {status = cli$get_value(&qual7,&dumdum,&length);
#ifdef SHORTINT
            if (sscanf(&dummy[0], "%ld", &maxpages)==0)
#else	/* ~SHORTINT */
            if (sscanf(&dummy[0], "%d", &maxpages)==0)
#endif	/* ~SHORTINT */
               error("! Bad number of pages option.") ;
      }

    status = cli$present(&qual8);
     if(status == CLI$_PRESENT)
     {status = cli$get_value(&qual8,&dumdum,&length);
     sscanf(&dummy[0],"%s",ofnme);
     }
     else
     {for(k=scnt;k<jext;k++)ofnme[k-scnt]=infnme[k];
        strcat(ofnme,".PS");}
     oname = &ofnme[0] ;

    status = cli$present(&qual4);

    if(status == CLI$_PRESENT)
        {oname="";
         noenv=(1==1);
        }

    status = cli$present(&qual9);
     if(status == CLI$_PRESENT)
     {status = cli$get_value(&qual9,&dumdum,&length);
#ifdef SHORTINT
            switch(sscanf(&dummy[0], "%ld.%ld", &firstpage, &firstseq)) {
#else	/* ~SHORTINT */
            switch(sscanf(&dummy[0], "%ld.%ld", &firstpage, &firstseq)) {
#endif	/* ~SHORTINT */
case 1:        firstseq = 0 ;
case 2:        break ;
default:
               error("! Bad first page option.") ;
            }
            notfirst = 1 ;
     }

    status = cli$present(&qual10);

    if(status == CLI$_PRESENT)
     quiet=(1==1);
    else
    if(status == CLI$_NEGATED)
     quiet=(1==0);

    status = cli$present(&qual11);

    if(status == CLI$_PRESENT)
     reverse=1;
    else 
    if(status == CLI$_NEGATED)
     reverse=0;

    status = cli$present(&qual12);

    if(status == CLI$_PRESENT)
     safetyenclose=1;
    else
    if(status == CLI$_NEGATED)
     safetyenclose=0;

    status = cli$present(&qual13);
     if(status == CLI$_PRESENT | status == CLI$_DEFAULTED)
     {status = cli$get_value(&qual13,&dumdum,&length);
     sscanf(&dummy[0],"%s",pap);
     if (strcmp(&pap[0], "landscape") == 0)
         { if (hpapersize || vpapersize)
              error( "both landscape and papersize specified; ignoring landscape") ;
           else
              landscape = 1 ;
         	/* The following lines have been added by G.Bonacci */
         	if ((status=cli$get_value(&qual13,&dumdum,&length))==1)
         	{
         		sscanf(&dummy[0],"%s",pap);
         		paperfmt = &pap[0] ;
         	}
         	/* end G.Bonacci 20.3.92 */
         }
         else
         paperfmt = &pap[0] ;
     }

    status = cli$present(&qual14);

    if(status == CLI$_PRESENT)
        {status = cli$get_value(&qual14,&dumdum,&length);

        if(sscanf(&dummy[0],"%d",&mag) == 0 || mag < 10 || mag >100000)
         error("!Bad magnification parameter.");
         overridemag=1;
        }
    status = cli$present(&qual15);

    if(status == CLI$_PRESENT | status == CLI$_DEFAULTED)
        {status = cli$get_value(&qual15,&dumdum,&length);

        if(sscanf(&dummy[0],"%d",&collatedcopies) == 0)
         error("!Bad number of collated copies");
        }
    status = cli$present(&qual16);

    if(status == CLI$_PRESENT | status == CLI$_DEFAULTED)
        {status = cli$get_value(&qual16,&dumdum,&length);

        if(sscanf(&dummy[0],"%d",&actualdpi) == 0 ||
           actualdpi < 10 || actualdpi >10000)
	 error("!Bad dpi parameter.");
  	 vactualdpi=actualdpi;
        }

    status = cli$present(&qual17);

    if(status == CLI$_PRESENT | status==CLI$_DEFAULTED)
      sendcontrolD =1;
    else
    if(status == CLI$_NEGATED)
      sendcontrolD =0;

    status = cli$present(&qual18);

    if(status == CLI$_NEGATED)
     removecomments =0;
    else
    if(status == CLI$_NEGATED)
     removecomments =1;

    status = cli$present(&qual19);

    if(status == CLI$_NEGATED)
      disablecomments = 0;
    else
    if(status == CLI$_NEGATED)
      disablecomments = 1;

    status = cli$present(&qual20);

    if(status == CLI$_PRESENT)
      nosmallchars=1;
    else
    if(status == CLI$_NEGATED)
      nosmallchars=0;

    status = cli$present(&qual21);

    if(status == CLI$_PRESENT)
        {status = cli$get_value(&qual21,&dumdum,&length);

        if(sscanf(&dummy[0],"%d",&actualdpi) == 0 ||
           actualdpi < 10 || actualdpi >10000)
         error("!Bad dpi parameter in X.");
        }

    status = cli$present(&qual22);

    if(status == CLI$_PRESENT)
        {status = cli$get_value(&qual22,&dumdum,&length);

        if(sscanf(&dummy[0],"%d",&vactualdpi) == 0 ||
           vactualdpi < 10 || vactualdpi >10000)
         error("!Bad dpi parameter in Y.");
        }

    status = cli$present(&qual23);

    if(status == CLI$_PRESENT)
      compressed=1;
    else
    if(status == CLI$_NEGATED)
      compressed=0;

    status = cli$present(&qual24);
     if(status == CLI$_PRESENT)
     {status = cli$get_value(&qual24,&dumdum,&length);
     sscanf(&dummy[0],"%s",prtr);
     u = &dummy[0] ;
     strcpy(u,"config.");
     strcat(u,&prtr[0]);
     getdefaults("");
     noenv=1;
     getdefaults(u);
     }

    status = cli$present(&qual25);
     if(status == CLI$_PRESENT) {
       while (status != SS$_NORMAL) {
       status = cli$get_value(&qual25,&dumdum,&length);
       dummy[length] = '\0';
       if (strcmp(dummy, "-") == 0)
          headers_off = 1;
       else
          (void)add_header(dummy) ;
       }
     }
     else {
       if(status == CLI$_NEGATED)
          headers_off = 1;
     }

    status = cli$present(&qual26);
    if(status == CLI$_PRESENT) abspage = 1;

    status = cli$present(&qual27);
    if(status == CLI$_PRESENT | status==CLI$_DEFAULTED) tryepsf = 1;
    else
    if(status == CLI$_NEGATED) tryepsf = 0;

    status = cli$present(&qual28);
    if(status == CLI$_PRESENT | status==CLI$_DEFAULTED) dopprescan = 1;
    else
    if(status == CLI$_NEGATED) dopprescan = 0;

    status = cli$present(&qual29);

    if(status == CLI$_PRESENT | status == CLI$_DEFAULTED)
        {status = cli$get_value(&qual29,&dumdum,&length);

        if(sscanf(&dummy[0],"%d",&pagecopies) == 0)
         error("!Bad number of page copies option /PAGECOPIES");
        if (pagecopies < 1 || pagecopies > 1000)
               error("! can only print one to a thousand page copies") ;
        }

    status = cli$present(&qual30);
    if(status == CLI$_PRESENT | status==CLI$_DEFAULTED) sepfiles = 1;
    else
    if(status == CLI$_NEGATED) sepfiles = 0;

    status = cli$present(&qual31);
    if(status == CLI$_PRESENT | status==CLI$_DEFAULTED) cropmarks = 1;
    else
    if(status == CLI$_NEGATED) cropmarks = 0;

    status = cli$present(&qual32);
     if(status == CLI$_PRESENT)
     {status = cli$get_value(&qual32,&dumdum,&length);
#ifdef SHORTINT
            if (sscanf(&dummy[0], "%ld", &maxsecsize)==0)
#else	/* ~SHORTINT */
            if (sscanf(&dummy[0], "%d", &maxsecsize)==0)
#endif	/* ~SHORTINT */
               error("! Bad section size arg (/SEC_SIZE).") ;
     }

    status = cli$present(&qual33);
     if(status == CLI$_PRESENT)
     {  (void)fprintf(stderr, banner) ;
        help() ;
     }

    status = cli$present(&qual34);
    if(status == CLI$_NEGATED) dontmakefont = 1;
/*-----------------------------------------------------------------------
 * Currently one uses /PSIZE="5mm,10mm" This way CASE is preserved
 * Should I convert the PSIZE to allow a LIST => /PSIZE=(5mm,10mm) and
 * convert the case back to lowercase in here.
 *-----------------------------------------------------------------------*/
    status = cli$present(&qual35);
     if(status == CLI$_PRESENT) {
       status = cli$get_value(&qual35,&dumdum,&length);
       dummy[length] = '\0';
       handlepapersize(&dummy[0], &hpapersize, &vpapersize) ;
       if (landscape) {
          error("both landscape and papersize specified; ignoring landscape") ;
          landscape = 0 ;
       }
     }

    status = cli$present(&qual36);
     if(status == CLI$_PRESENT) {
       status = cli$get_value(&qual36,&dumdum,&length);
       dummy[length] = '\0';
       handlepapersize(&dummy[0], &hoff, &voff) ;
     }

}
