#!/bin/sh

#################################################################################################
# Functions

#####
# WriteXMLHeader writes the beginning of the XML project file
#####
WriteXMLHeader()
{
echo "<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>"
echo "<?codewarrior exportversion="1.0.1" ideversion="4.1" ?>"
echo ""
echo "<!DOCTYPE PROJECT ["
echo "<!ELEMENT PROJECT (TARGETLIST, TARGETORDER, GROUPLIST, DESIGNLIST?)>"
echo "<!ELEMENT TARGETLIST (TARGET+)>"
echo "<!ELEMENT TARGET (NAME, SETTINGLIST, FILELIST?, LINKORDER?, SEGMENTLIST?, OVERLAYGROUPLIST?, SUBTARGETLIST?, SUBPROJECTLIST?)>"
echo "<!ELEMENT NAME (#PCDATA)>"
echo "<!ELEMENT USERSOURCETREETYPE (#PCDATA)>"
echo "<!ELEMENT PATH (#PCDATA)>"
echo "<!ELEMENT FILELIST (FILE*)>"
echo "<!ELEMENT FILE (PATHTYPE, PATHROOT?, ACCESSPATH?, PATH, PATHFORMAT?, ROOTFILEREF?, FILEKIND?, FILEFLAGS?)>"
echo "<!ELEMENT PATHTYPE (#PCDATA)>"
echo "<!ELEMENT PATHROOT (#PCDATA)>"
echo "<!ELEMENT ACCESSPATH (#PCDATA)>"
echo "<!ELEMENT PATHFORMAT (#PCDATA)>"
echo "<!ELEMENT ROOTFILEREF (PATHTYPE, PATHROOT?, ACCESSPATH?, PATH, PATHFORMAT?)>"
echo "<!ELEMENT FILEKIND (#PCDATA)>"
echo "<!ELEMENT FILEFLAGS (#PCDATA)>"
echo "<!ELEMENT FILEREF (TARGETNAME?, PATHTYPE, PATHROOT?, ACCESSPATH?, PATH, PATHFORMAT?)>"
echo "<!ELEMENT TARGETNAME (#PCDATA)>"
echo "<!ELEMENT SETTINGLIST ((SETTING|PANELDATA)+)>"
echo "<!ELEMENT SETTING (NAME?, (VALUE|(SETTING+)))>"
echo "<!ELEMENT PANELDATA (NAME, VALUE)>"
echo "<!ELEMENT VALUE (#PCDATA)>"
echo "<!ELEMENT LINKORDER (FILEREF*)>"
echo "<!ELEMENT SEGMENTLIST (SEGMENT+)>"
echo "<!ELEMENT SEGMENT (NAME, ATTRIBUTES?, FILEREF*)>"
echo "<!ELEMENT ATTRIBUTES (#PCDATA)>"
echo "<!ELEMENT OVERLAYGROUPLIST (OVERLAYGROUP+)>"
echo "<!ELEMENT OVERLAYGROUP (NAME, BASEADDRESS, OVERLAY*)>"
echo "<!ELEMENT BASEADDRESS (#PCDATA)>"
echo "<!ELEMENT OVERLAY (NAME, FILEREF*)>"
echo "<!ELEMENT SUBTARGETLIST (SUBTARGET+)>"
echo "<!ELEMENT SUBTARGET (TARGETNAME, ATTRIBUTES?, FILEREF?)>"
echo "<!ELEMENT SUBPROJECTLIST (SUBPROJECT+)>"
echo "<!ELEMENT SUBPROJECT (FILEREF, SUBPROJECTTARGETLIST)>"
echo "<!ELEMENT SUBPROJECTTARGETLIST (SUBPROJECTTARGET*)>"
echo "<!ELEMENT SUBPROJECTTARGET (TARGETNAME, ATTRIBUTES?, FILEREF?)>"
echo "<!ELEMENT TARGETORDER (ORDEREDTARGET|ORDEREDDESIGN)*>"
echo "<!ELEMENT ORDEREDTARGET (NAME)>"
echo "<!ELEMENT ORDEREDDESIGN (NAME, ORDEREDTARGET+)>"
echo "<!ELEMENT GROUPLIST (GROUP|FILEREF)*>"
echo "<!ELEMENT GROUP (NAME, (GROUP|FILEREF)*)>"
echo "<!ELEMENT DESIGNLIST (DESIGN+)>"
echo "<!ELEMENT DESIGN (NAME, DESIGNDATA)>"
echo "<!ELEMENT DESIGNDATA (#PCDATA)>"
echo "]>"
echo ""
}

#####
# WriteFILE generates a complete <FILE>...</FILE> entry
#####
WriteFILE()
{
    echo "<FILE>"
    echo "	<PATHTYPE>Name</PATHTYPE>"
    echo "	<PATH>$1</PATH>"
    echo "	<PATHFORMAT>MacOS</PATHFORMAT>"
    echo "	<FILEKIND>Text</FILEKIND>"
    echo "	<FILEFLAGS>Debug</FILEFLAGS>"
    echo "</FILE>"
}

#####
# WriteFILEREF generates a complete <FILEREF>...</FILEREF> entry
#####
WriteFILEREF()
{
    echo "<FILEREF>"
    if [ $# -ge 2 ]; then
    echo "	<TARGETNAME>$2</TARGETNAME>"
    fi
    echo "	<PATHTYPE>Name</PATHTYPE>"
    echo "	<PATH>$1</PATH>"
    echo "	<PATHFORMAT>MacOS</PATHFORMAT>"
    echo "</FILEREF>"
}


#####
# WriteValueSetting generates a complete value entry
#####
WriteValueSetting()
{
    SETTINGNAME=$1
    VALUE=$2
    
    echo "<SETTING><NAME>$SETTINGNAME</NAME><VALUE>$VALUE</VALUE></SETTING>"
}



#####
# WritePathSetting generates a complete path entry
#####
WritePathSetting()
{
    SETTINGNAME=$1
    PATH=$2
    PATHFORMAT=$3
    PATHROOT=$4
    
    echo "<SETTING>"
        echo "<SETTING><NAME>$SETTINGNAME</NAME>"
            echo "<SETTING><NAME>Path</NAME><VALUE>$PATH</VALUE></SETTING>"
            echo "<SETTING><NAME>PathFormat</NAME><VALUE>$PATHFORMAT</VALUE></SETTING>"
            echo "<SETTING><NAME>PathRoot</NAME><VALUE>$PATHROOT</VALUE></SETTING>"
        echo "</SETTING>"
        echo "<SETTING><NAME>Recursive</NAME><VALUE>true</VALUE></SETTING>"
        echo "<SETTING><NAME>HostFlags</NAME><VALUE>All</VALUE></SETTING>"
    echo "</SETTING>"
}

#####
# WriteSETTINGLIST generates a complete <SETTINGLIST>...</SETTINGLIST> entry
#####
WriteSETTINGLIST()
{
    TARGETNAME=$1
    OUTPUTNAME=$2
    
    echo "<SETTINGLIST>"
        
        echo "<!-- Settings for "Target Settings" panel -->"
        WriteValueSetting Linker "MacOS PPC Linker"
        WriteValueSetting PreLinker ""
        WriteValueSetting PostLinker ""
        WriteValueSetting Targetname "$TARGETNAME"
        echo "<SETTING><NAME>OutputDirectory</NAME>"
            echo "<SETTING><NAME>Path</NAME><VALUE>:Output:</VALUE></SETTING>"
            echo "<SETTING><NAME>PathFormat</NAME><VALUE>MacOS</VALUE></SETTING>"
            echo "<SETTING><NAME>PathRoot</NAME><VALUE>Project</VALUE></SETTING>"
        echo "</SETTING>"
        WriteValueSetting SaveEntriesUsingRelativePaths false
        
        echo "<!-- Settings for "Access Paths" panel -->"
        WriteValueSetting AlwaysSearchUserPaths false
        WriteValueSetting InterpretDOSAndUnixPaths true
        echo "<SETTING><NAME>UserSearchPaths</NAME>"
            WritePathSetting SearchPath ":src:" MacOS Project
            WritePathSetting SearchPath ":obj:" MacOS Project
            WritePathSetting SearchPath ":" MacOS Project
        echo "</SETTING>"
        echo "<SETTING><NAME>SystemSearchPaths</NAME>"
            WritePathSetting SearchPath ":jbig2dec:" MacOS Project
            WritePathSetting SearchPath ":jasper/src/libjasper/include:" MacOS Project
            WritePathSetting SearchPath ":obj:" MacOS Project
            WritePathSetting SearchPath ":MacOS Support:" MacOS CodeWarrior
            WritePathSetting SearchPath ":MSL:MSL_C" MacOS CodeWarrior
            WritePathSetting SearchPath ":" MacOS CodeWarrior
            WritePathSetting SearchPath ":" MacOS Project
        echo "</SETTING>"
        
        echo "<!-- Settings for "Build Extras" panel -->"
        WriteValueSetting CacheModDates true
	WriteValueSetting ActivateBrowser true
	WriteValueSetting DumpBrowserInfo false
	WriteValueSetting CacheSubprojects true
        
	echo "<!-- Settings for "PPC Project" panel -->"
	WriteValueSetting MWProject_PPC_type SharedLibrary
	WriteValueSetting MWProject_PPC_outfile "$OUTPUTNAME"
	WriteValueSetting MWProject_PPC_filecreator 1061109567
	WriteValueSetting MWProject_PPC_filetype 1936223330
	WriteValueSetting MWProject_PPC_size 0
	WriteValueSetting MWProject_PPC_minsize 0
	WriteValueSetting MWProject_PPC_stacksize 0
	WriteValueSetting MWProject_PPC_flags 0
	WriteValueSetting MWProject_PPC_symfilename ""
	WriteValueSetting MWProject_PPC_rsrcname ""
	WriteValueSetting MWProject_PPC_rsrcheader Native
	WriteValueSetting MWProject_PPC_rsrctype 1061109567
	WriteValueSetting MWProject_PPC_rsrcid 0
	WriteValueSetting MWProject_PPC_rsrcflags 0
	WriteValueSetting MWProject_PPC_rsrcstore 0
	WriteValueSetting MWProject_PPC_rsrcmerge 0
        
	echo "<!-- Settings for "C/C++ Compiler" panel -->"
	WriteValueSetting MWFrontEnd_C_cplusplus 0
	WriteValueSetting MWFrontEnd_C_checkprotos 0
	WriteValueSetting MWFrontEnd_C_arm 0
	WriteValueSetting MWFrontEnd_C_trigraphs 0
	WriteValueSetting MWFrontEnd_C_onlystdkeywords 0
	WriteValueSetting MWFrontEnd_C_enumsalwaysint 0
	WriteValueSetting MWFrontEnd_C_mpwpointerstyle 1
	
	# install the carbon prefix file for carbon targets
	if test "$OUTPUTNAME" = "GhostscriptLib Carbon"; then
	  if test "$TARGETNAME" = "GhostscriptLib Carbon (Debug)"; then
	    WriteValueSetting MWFrontEnd_C_prefixname macos_carbon_d_pre.h
	  else
	    WriteValueSetting MWFrontEnd_C_prefixname macos_carbon_pre.h
	  fi
	else
	  if test "$TARGETNAME" = "GhostscriptLib PPC (Debug)"; then
	    WriteValueSetting MWFrontEnd_C_prefixname macos_classic_d_pre.h
	  else
	    WriteValueSetting MWFrontEnd_C_prefixname
	  fi
	fi
	
	WriteValueSetting MWFrontEnd_C_ansistrict 0
	WriteValueSetting MWFrontEnd_C_mpwcnewline 0
	WriteValueSetting MWFrontEnd_C_wchar_type 1
	WriteValueSetting MWFrontEnd_C_enableexceptions 0
	WriteValueSetting MWFrontEnd_C_dontreusestrings 0
	WriteValueSetting MWFrontEnd_C_poolstrings 0
	WriteValueSetting MWFrontEnd_C_dontinline 1
	WriteValueSetting MWFrontEnd_C_useRTTI 0
	WriteValueSetting MWFrontEnd_C_multibyteaware 0
	WriteValueSetting MWFrontEnd_C_unsignedchars 0
	WriteValueSetting MWFrontEnd_C_autoinline 0
	WriteValueSetting MWFrontEnd_C_booltruefalse 0
	WriteValueSetting MWFrontEnd_C_direct_to_som 0
	WriteValueSetting MWFrontEnd_C_som_env_check 0
	WriteValueSetting MWFrontEnd_C_alwaysinline 0
	WriteValueSetting MWFrontEnd_C_inlinelevel 0
	WriteValueSetting MWFrontEnd_C_ecplusplus 0
	WriteValueSetting MWFrontEnd_C_objective_c 0
	WriteValueSetting MWFrontEnd_C_defer_codegen 0
        
	echo "<!-- Settings for "C/C++ Warnings" panel -->"
	WriteValueSetting MWWarning_C_warn_illpragma 1
	WriteValueSetting MWWarning_C_warn_emptydecl 0
	WriteValueSetting MWWarning_C_warn_possunwant 0
	WriteValueSetting MWWarning_C_warn_unusedvar 1
	WriteValueSetting MWWarning_C_warn_unusedarg 0
	WriteValueSetting MWWarning_C_warn_extracomma 1
	WriteValueSetting MWWarning_C_pedantic 1
	WriteValueSetting MWWarning_C_warningerrors 0
	WriteValueSetting MWWarning_C_warn_hidevirtual 1
	WriteValueSetting MWWarning_C_warn_implicitconv 0
	WriteValueSetting MWWarning_C_warn_notinlined 0
	WriteValueSetting MWWarning_C_warn_structclass 0
        
	echo "<!-- Settings for "PPC CodeGen" panel -->"
	WriteValueSetting MWCodeGen_PPC_structalignment PPC
	WriteValueSetting MWCodeGen_PPC_tracebacktables Inline
	WriteValueSetting MWCodeGen_PPC_processor Generic
	WriteValueSetting MWCodeGen_PPC_readonlystrings 0
	WriteValueSetting MWCodeGen_PPC_tocdata 1
	WriteValueSetting MWCodeGen_PPC_profiler 0
	WriteValueSetting MWCodeGen_PPC_fpcontract 0
	WriteValueSetting MWCodeGen_PPC_schedule 1
	WriteValueSetting MWCodeGen_PPC_peephole 1
	WriteValueSetting MWCodeGen_PPC_processorspecific 0
	WriteValueSetting MWCodeGen_PPC_altivec 0
	WriteValueSetting MWCodeGen_PPC_vectortocdata 0
	WriteValueSetting MWCodeGen_PPC_vrsave 0
	
	echo "<!-- Settings for "PPC Global Optimizer" panel -->"
	WriteValueSetting GlobalOptimizer_PPC_optimizationlevel Level0
	WriteValueSetting GlobalOptimizer_PPC_optfor Speed
        
	echo "<!-- Settings for "PPC Linker" panel -->"
	WriteValueSetting MWLinker_PPC_linksym 1
	WriteValueSetting MWLinker_PPC_symfullpath 1
	WriteValueSetting MWLinker_PPC_linkmap 0
	WriteValueSetting MWLinker_PPC_nolinkwarnings 0
	WriteValueSetting MWLinker_PPC_dontdeadstripinitcode 0
	WriteValueSetting MWLinker_PPC_permitmultdefs 0
	WriteValueSetting MWLinker_PPC_linkmode Fast
	WriteValueSetting MWLinker_PPC_initname "__initialize"
	WriteValueSetting MWLinker_PPC_mainname ""
	WriteValueSetting MWLinker_PPC_termname "__terminate"
        
	echo "<!-- Settings for "PPC PEF" panel -->"
	WriteValueSetting MWPEF_exports Pragma
	WriteValueSetting MWPEF_libfolder 0
	WriteValueSetting MWPEF_sortcode None
	WriteValueSetting MWPEF_expandbss 0
	WriteValueSetting MWPEF_sharedata 0
	WriteValueSetting MWPEF_olddefversion 0
	WriteValueSetting MWPEF_oldimpversion 0
	WriteValueSetting MWPEF_currentversion 0
	WriteValueSetting MWPEF_fragmentname ""
	WriteValueSetting MWPEF_collapsereloads 0
        
    echo "</SETTINGLIST>"
}


#####
# WriteTARGET generates a complete <TARGET>...</TARGET> entry
#####
WriteTARGET()
{
    TARGETNAME=$1
    OUTPUTNAME=$2
    shift 2
    
    echo "<TARGET>"
        
        echo "<NAME>$TARGETNAME</NAME>"
        
        WriteSETTINGLIST "$TARGETNAME" "$OUTPUTNAME"
        
        echo "<FILELIST>"
            for file in "$@"; do
                WriteFILE "$file"
            done
        echo "</FILELIST>"
        
        echo "<LINKORDER>"
            for file in "$@"; do
                WriteFILEREF "$file"
            done
        echo "</LINKORDER>"
        
    echo "</TARGET>"
}


#####
# WriteGROUP generates a complete <GROUP>...</GROUP> entry
#####
WriteGROUP()
{
    GROUPNAME=$1
    TARGETNAME=$2
    shift 2
    
    echo "<GROUP><NAME>$GROUPNAME</NAME>"
        
        for file in "$@"; do
            WriteFILEREF "$file" "$TARGETNAME"
        done
        
    echo "</GROUP>"
}


#################################################################################################
# the start of the script

#####
# first create a list of .c files that will be part of the project, we need it several times
#####

CFILES=
while [ $# -ge 1 ]; do
    case $1 in
        \\);;
        *.o)
            # strip path before file name and convert .o to .c, then append file name to CFILES
            CFILES=$CFILES\ `echo $1 | sed -e 's/\.\/.*\///' -e 's/\.\///' -e 's/\.o/\.c/'`
            ;;
    esac
    shift
done

# libs for codewarrior 6
#LIBS="console.stubs.c MSL\ ShLibRuntime.Lib MSL\ RuntimePPC.Lib"
#CLASSICLIBS="MSL\ C.PPC.Lib InterfaceLib FontManager MathLib"

# libs for codewarrior 7 & 8
LIBS=""
CARBONLIBS="MSL_All_Carbon.Lib CarbonLib"
CLASSICLIBS="MSL_All_PPC.Lib InterfaceLib FontManager MathLib"
CLASSICLIBS="$CLASSICLIBS TextCommon UnicodeConverter UTCUtils"

#####
# 
#####

GSNAME="GhostscriptLib"
CLASSICGSNAME="$GSNAME PPC"
CARBONGSNAME="$GSNAME Carbon"
CLASSICDEBUGTARGETNAME="$CLASSICGSNAME (Debug)"
CLASSICFINALTARGETNAME="$CLASSICGSNAME (Final)"
CARBONDEBUGTARGETNAME="$CARBONGSNAME (Debug)"
CARBONFINALTARGETNAME="$CARBONGSNAME (Final)"

WriteXMLHeader

echo "<PROJECT>"
    
    echo "<TARGETLIST>"
    WriteTARGET "$CARBONDEBUGTARGETNAME" "$CARBONGSNAME" $CFILES $LIBS $CARBONLIBS
    WriteTARGET "$CLASSICDEBUGTARGETNAME" "$CLASSICGSNAME" $CFILES $LIBS $CLASSICLIBS
    echo "</TARGETLIST>"
    
    echo "<TARGETORDER>"
        echo "<ORDEREDTARGET><NAME>$CARBONDEBUGTARGETNAME</NAME></ORDEREDTARGET>"
        echo "<ORDEREDTARGET><NAME>$CLASSICDEBUGTARGETNAME</NAME></ORDEREDTARGET>"
    echo "</TARGETORDER>"
    
    echo "<GROUPLIST>"
        WriteGROUP "Ghostscript Sources" "$CARBONDEBUGTARGETNAME" $CFILES
#        WriteGROUP "Libraries" "$CARBONDEBUGTARGETNAME" $LIBS $CARBONLIBS $CLASSICLIBS
#        WriteGROUP "Libraries" "$CARBONDEBUGTARGETNAME" "console.stubs.c" "MSL ShLibRuntime.Lib" "MSL RuntimePPC.Lib" "MSL C.Carbon.Lib" "CarbonLib" "MSL C.PPC.Lib" "InterfaceLib" "FontManager" "MathLib"

# nb: this code doesn't work if there are spaces in the library filenames        
        echo "<GROUP><NAME>Libraries</NAME>"
	for lib in $LIBS; do
            WriteFILEREF "$lib" "$CARBONDEBUGTARGETNAME"
	    WriteFILEREF "$lib" "$CLASSICDEBUGTARGETNAME"
	done
	for lib in $CARBONLIBS; do
            WriteFILEREF "$lib" "$CARBONDEBUGTARGETNAME"
	done
	for lib in $CLASSICLIBS; do
            WriteFILEREF "$lib" "$CLASSICDEBUGTARGETNAME"
	done
        echo "</GROUP>"
        
    echo "</GROUPLIST>"

echo "</PROJECT>"

exit 0
