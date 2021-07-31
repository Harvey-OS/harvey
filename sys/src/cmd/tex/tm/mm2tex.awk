# awk script to translate troff/mm->LaTeX/tm.sty
# Part of filter from troff/mm to LaTeX using tm.sty
# Created by Terry L Anderson
# modified by Terry L Anderson starting 91-May-28
# last modified 91-Jun-05
BEGIN	{ FS = " "; OFS = " ";
	# Modify these for local formats
	defaultptsize=10
	currentptsize=defaultptsize
	previousptsize=defaultptsize
	defaultFont="rm"
	currentFont=defaultFont
	previousFont=defaultFont
	F=0
	p=0
	Fg=0
	Tb=0
	Rf=0
	R=0
	Refs=0
	reffile="ref.ref"
	listlevel=0
	listtype[listlevel]="none"
# set mm vars


# print headers

	print "% -*-latex-*-"
	print "% Translated from mm by mm2tex"
	print "% mm2tex filter written by Terry L Anderson"
	print "\\documentstyle[]{tm}"
	print "\\newcommand{\\X}[1]{{#1}\\index{{#1}}}"
	print "%\\chargecase{978899-0100}"
	print "%\\filecase{61093}"
	print "%\\documentno{900000}{02}{TMS}"
	print "\\title{\\ }"
	print "\\keywords{}"
	print "\\mercurycode{CMP}"
	print "\\date{\\today}"
	print "\\memotype{TECHNICAL MEMORANDUM}"
	print "%\\abstract{}"
	print "\\organizationalapproval"
	print "%\\markdraft*"
	print "\\markproprietary"
	titleon=0
	preamble=1
	hasatitle=0; hasanauthor=0
	}
END	{if(hasrefs) {print "\\begin{thebibliography}{99}";
#		print "\\input ref.ref"; 
		while (getline refline <"ref.ref" >0) print refline
		print "\\end{thebibliography}"
		}
	if(tc) print "\\tableofcontents"	
	if(coversheet) print "\\coversheet"
	print"\\end{document}"}

# note if mm instruction
/^\./	{MM=1
	if (titleon) {titleon=0;print "}"}
	}
/^\.AU/	{hasanauthor=1
	print "\\author{"$2"}"
	print "\\initials{"$3"}"
	print "\\location{"$4"}{"$7"}{"$6"}"
	print "\\department{"$5"}"
	if (NF>7) print "\\eaddress{"$8"}"
	next
	}
/^\.AT/	{printf "\\signatureextra{"
	for (f=2;f<NF;f++) printf $f"\\\\"
	print $NF"}"
	next}
/^\.TL/	{if (NF>1) print "\\chargecase{"$2"}\\filecase{"$3"}"
	 printf "\\title{"
	 titleon=1
	 hasatitle=1
	 next
	}
/^\.OK/	{printf "\\keywords{"
 	for(f=2;f<NF;f++) printf $f " "
	print $NF"}"
	next
	}
/^\.SG/	{print "\\makesignature"
	next
	}	
/^\.ND/	{print "\\date{"$2"}"
	next
	}
/^\.AS/	{printf "\\abstract{"
	 abstracton=1
	 hasabstract=1
	 next
	}
/^\.AE/	{print "}"
	 abstracton=0
	next
	}
/^\.MT/	{if ((NF<2) || ($2==0)) print "\\memotype{\\ }"
	 else if ($2==1) print "\\memotype{TECHNICAL MEMORANDUM}"
	 else if ($2==2) print "\\memotype{INTERNAL MEMORANDUM}"
	 else print "\\memotype{"$2"}"
	 next
	}
/^\.TM/	{DNNF=split($2,DN,"-");
	 if (DNNF==3){ 
	  printf "\\department{"DN[1]"}"
	  print "\\documentno{"DN[2]"}{"substr(DN[3],1,2)"}{"substr(DN[3],3)"}"
	  }
	 next
	}
/^\.CS/ {coversheet=1;next}
/^\.TC/ {tc=1;next}
/^\.PM/ {if (NF<2) print "\\marknone"
#	 else if ($2=="P") print "\\markprivate"
#        Gehani says P is private but our strings.mm has it as propri
	 else if ($2=="P") print "\\markproprietary"
	 else if (($2=="BP")||($2=="PM1")) print "\\markproprietary"
	 else if (($2=="BR")||($2=="PM2") || ($2=="RS")) 
		print "\\markrestricted"
	 else if (($2=="RG")||($2=="PM3")) print "\\markregistered"
	next
	}
# string registers (others in mm2tex.sed)
/\$\\backslash\$\*F/ {
	gsub(/\$\\backslash\$\*F/,"\\footnotemark["++p"]");
	}
/\$\\backslash\$\*\(Rf/	{
	sub(/\$\\backslash\$\*\(Rf/,"\\cite{bib:ref"++R"}")
	}
/\$\\backslash\$\*\(EM/ {
	sub(/\$\\backslash\$\*\(EM/,"---")
	}
/\$\\backslash\$\*\(em/ {
	sub(/\$\\backslash\$\*\(em/,"---")
	}
# number registers (others in mm2tex.sed)
/\$\\backslash\$n\(Rf/	{
	sub(/\$\\backslash\$n\(Rf/,Rf)
	}
/\$\\backslash\$n\(Fg/ {
	gsub(/\$\\backslash\$n\(Fg[^\+]/,Fg);
	gsub(/\$\\backslash\$n\(Fg\+./,Fg+1)
	}
/\$\\backslash\$n\(Tb/ {
	gsub(/\$\\backslash\$n\(Tb[^\+]/,Tb);
	gsub(/\$\\backslash\$n\(Tb\+./,Tb+1)
	}

# fonts -- the inlines implemented here rather than in sed so that they can 
# interact with the others for previous font ...
# the inlines must be handled by a single target so order is maintained
/(\$\\backslash\$f)(I|CW|R|B|P)/ 	{
	while(pos=match($0,/(\$\\backslash\$f)(I|CW|R|B|P)/)) {
	char=substr($0,pos+13,1)
	if (char=="C") char2=substr($0,pos+15,1) 
	else char2=substr($0,pos+14,1) 
	if (char=="P") currentFont=previousFont
	else {previousFont=currentFont
	  if (char=="I") currentFont="it"
	  else if (char=="R") currentFont="rm"
	  else if (char=="B") currentFont="bf"
	  else if (char=="C") currentFont="tt"
	  }
	if ((char2==" ") || (char2=="")) 
	  sub(/(\$\\backslash\$f)(I|CW|R|B|P)/,"\\"currentFont"\\$.$",$0)
	else sub(/(\$\\backslash\$f)(I|CW|R|B|P)/,"\\"currentFont"$.$",$0)
	}}
/^\.ft/	{if(NF==1) {currentFont=previousFont}
	else {previousFont=currentFont;
	  if ($2=="R") currentFont="rm"
	  if ($2=="I") currentFont="it"
	  if ($2=="B") currentFont="bf"
	  if ($2=="CW") currentFont="tt"
	  }
	printf "\\"currentFont" "
	next}
/^\.B /	{if(NF==1) {previousFont=currentFont; currentFont="bf"; printf "\\bf "}
	next}
/^\.B$/	{if(NF==1) {previousFont=currentFont; currentFont="bf"; prinft "\\bf "}
	next}
/^\.I /	{if(NF==1) {previousFont=currentFont; currentFont="it"; printf "\\it "}
	next}
/^\.I$/	{if(NF==1) {previousFont=currentFont; currentFont="it"; printf "\\it "}
	next}
/^\.R /	{if(NF==1) {previousFont=currentFont; currentFont="rm"; printf "\\rm "}
	next}
/^\.R$/	{if(NF==1) {previousFont=currentFont; currentFont="rm"; printf "\\rm "}
	next}
#/^\.S$/ {currentptsize=previousptsize
#	printf "<pts "currentptsize">";next }
#/^\.S /  {if(NF==1) currentptsize=previousptsize
#	  else if(substr($2,1,1)=="P")currentptsize=previousptsize
#	  else if(substr($2,1,1)=="C"){}
#	  else if(substr($2,1,1)=="D")currentptsize=defaultptsize
#	  else if(substr($2,1,1)=="+")currentptsize+=substr($2,2) 
#	  else if(substr($2,1,1)=="-")currentptsize-=substr($2,2)
#	  else currentptsize=$2
#	printf "<pts "currentptsize">";next }
# BL and DL both give itemized lists and do not force bullets and dashes
/^\.DL/	{listlevel++; listtype[listlevel]="itemize"; 
	printf("\\begin{%s}\n",listtype[listlevel])
	next }
/^\.BL/	{listlevel++; listtype[listlevel]="itemize"; 
	printf("\\begin{%s}\n",listtype[listlevel])
	next }
/^\.ML/	{listlevel++; listtype[listlevel]="list"; 
	printf("\\begin{%s}{"$2"}{}\n",listtype[listlevel])
	next }
/^\.VL/	{listlevel++; listtype[listlevel]="list"; 
	printf("\\begin{%s}{}{}\n",listtype[listlevel])
	next }
/^\.AL/	{listlevel++; listtype[listlevel]="enumerate"; 
	printf("\\begin{%s}\n",listtype[listlevel])
	next }
/^\.H /	{print ""; #note headings 3 and higher all subsubsection
	if ($2=="1") printf("\\section")
	else if ($2=="2") printf("\\subsection")
	else if ($2>2) printf("\\subsubsection")
	printf "{"
	for(f=3;f<=NF;f++) printf $f " "
	printf "}\n"
	next}

/^\.FS/	{#like troff .FS incr F and \*F incr p (:p in troff); why?
	if (NF>1) printf "\\markedfootnote{"$2"}{"
	else printf "\\footnotetext["++F"]{"
	next}
/^\.FE/	{print "}"
	next}
/^\.LI/	{printf("\\item");
	if (NF>1) printf "["$2"]"
	else printf " "
	next}
/^\.LE/	{printf("\\end{%s}\n",listtype[listlevel])
	listtype[listlevel]="none";listlevel--;
	next}
/^\.NS/	{nfon++
	 if ((NF<2) || ($2<3) || ($2==10) || ($2==11)) print "\\copyto{"
	 else {
	   print "{\\noindent" 
	   if ($2==3) print "Att.\\\\"
	   else if ($2==4) print "Atts.\\\\"
	   else if ($2==5) print "Enc.\\\\"
	   else if ($2==6) print "Encs.\\\\"
	   else if ($2==7) print "U.\ S.\ C.\\\\"
	   else if ($2==8) print "Letter to\\\\"
	   else if ($2==9) print "Memorandum to\\\\"
	   else if ($2==12) print "Abstract Only to\\\\"
	   else if ($2==13) print "Complete Memorandum to\\\\"
	   }
	next}
/^\.NE/	{nfon--;printf "}"
	next}
/^\.P/	{print ""
	next}
#  in Tex spaces at top or bottom of text column are ignored.  By
#  using `keep with' a space can be glued to things like table and
#  figure captions.  We'll keep .SP and .sp different as to whether
#  space is above or below so there is some flexibility.  .sp should
#  be used when kept with something blow like figure captions and .SP
#  when kept with something above like table captions.

/^\.sp/	{  #currently behaves like .sp
	if(NF>1) spacesize = $2*12 
	else spacesize = 12;
	printf "\\vspace*{"spacesize" pt}\n"
	next}

/^\.SP/	{  #currently supports only unscaled args
	if(NF>1) spacesize = $2*12 
	else spacesize = 12;
	printf "\\vspace*{"spacesize" pt}\n"
	next} 
/^\.SK/	{for (np=0;np<$2;np++) print "\\newpage" ; next}
/^\.bp/ {print "\\newpage"}
/^\.br/	{print "\\\\";next}
/^\.nf/	{printf "\\begin{verbatim}"; next }  # no fill
/^\.fi/	{printf "\\end{verbatim}"; next }  # fill again
/^\.TS/	{nfon++; next }  # table start -- treat as verbatim
/^\.TE/	{nfon--; next }  # table end
/^\.TB/ {printf("\n") ; 
	if (!indisplay)printf "\\begin{table}"
	# since display may be started as figure must force caption type
	printf "\\typedcaption{table}{" ; printf substr($0,5); print "}"
	if (!indisplay)printf "\\end{table}"
	Tb++;next } # table caption
/^\.FG/ {printf("\n") ; 
	if (!indisplay)printf "\\begin{figure}"
	printf "\\caption{" ;printf substr($0,5); print "}"
	if (!indisplay)printf "\\end{figure}"
	Fg++;next } # figure caption
/^\.DS/	{print "\\begin{figure}[htbp]";indisplay++
	# display (static) start - quess that it is a figure
	# if not fix at caption
	next}
/^\.DF/	{print "\\begin{figure}";indisplay++
	# display (floating) start - quess that it is a figure
	next }
/^\.DE/	{print "\\end{figure}";indisplay--;next }  # display end
#/^\.ce/	{printf("\n") ; print "<Title>"; next}
# divert references
/^\.RS/	{#lets divert them to a file like with frame
	#like troff .RS incr Rf and \*(Rf incr R (:R in troff); why?
	if (!hasrefs) {print "%bibitems generated by mm2tex" >reffile
		hasrefs=1
		}
	printf "\\bibitem{bib:ref"++Rf"}" >>reffile
	refdivon=1
	next}
/^\.RF/	{refdivon=0;next}
# macros to ignore & suppress printing
/^\.ds/ {next}
/^\.nr/ {next}
/^\.de/ {# ignore till ..
	getline
	while(substr($0,1,2)!="..")getline
	next
	}
/^\.\$\\backslash\$"/	{next}  # scratch comment. Extra verb stuff from sed
/^\./	{if (!preamble) {
		printf("\n") 
		print ;
		}
	} 
		
!/^\./	{MM=0
	if (preamble &&	 !titleon && !abstracton) {
		preamble=0
		if (!hasanauthor) {
			print "\\author{\\ }"
			print "\\initials{\\ }"
			print "\\department{\\ }"
			print "\\location{\\ }"
			}
		if (!hasatitle) print "\\title{\\ }"
	  	print "\\begin{document}"
		print "\\bibliographystyle{atttm}"
		print "\\makehead"
		if (hasabstract) print "\\makeabstract"
		}
	if(nfon) print $0"\\\\"
	else if(refdivon) {print >> reffile}
	else print
	next
	}

