/*
 *      Version numbering for LAME.
 *
 *      Copyright (c) 1999 A.L. Faber
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*!
  \file   version.c
  \brief  Version numbering for LAME.

  Contains functions which describe the version of LAME.

  \author A.L. Faber
  \version \$Id: version.c,v 1.17 2001/02/20 18:17:59 aleidinger Exp $
  \ingroup libmp3lame

  \todo The mp3x version should be located in the mp3x source,
        not in the libmp3lame source.
*/


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif


#include <stdio.h>
#include <lame.h>
#include "version.h"    /* macros of version numbers */

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

//! Stringify \a x.
#define STR(x)   #x
//! Stringify \a x, perform macro expansion.
#define XSTR(x)  STR(x)

#if defined(MMX_choose_table)
# define V1  "MMX "
#else
# define V1  ""
#endif

#if defined(KLEMM)
# define V2  "KLM "
#else
# define V2  ""
#endif

#if defined(RH)
# define V3  "RH "
#else
# define V3  ""
#endif

//! Compile time features.
#define V   V1 V2 V3

//! Get the LAME version string.
/*!
  \param void
  \return a pointer to a string which describes the version of LAME.
*/
const char*  get_lame_version ( void )		/* primary to write screen reports */
{
    /* Here we can also add informations about compile time configurations */

#if   LAME_ALPHA_VERSION > 0
    static /*@observer@*/ const char *const str =
        XSTR(LAME_MAJOR_VERSION) "." XSTR(LAME_MINOR_VERSION) " " V
        "(alpha " XSTR(LAME_ALPHA_VERSION) ", " __DATE__ " " __TIME__ ")";
#elif LAME_BETA_VERSION > 0
    static /*@observer@*/ const char *const str =
        XSTR(LAME_MAJOR_VERSION) "." XSTR(LAME_MINOR_VERSION) " " V
        "(beta " XSTR(LAME_BETA_VERSION) ", " __DATE__ ")";
#else
    static /*@observer@*/ const char *const str =
        XSTR(LAME_MAJOR_VERSION) "." XSTR(LAME_MINOR_VERSION) " " V;
#endif

    return str;
}


//! Get the short LAME version string.
/*!
  It's mainly for inclusion into the MP3 stream.

  \param void   
  \return a pointer to the short version of the LAME version string.
*/
const char*  get_lame_short_version ( void )
{
    /* adding date and time to version string makes it harder for output
       validation */

#if   LAME_ALPHA_VERSION > 0
    static /*@observer@*/ const char *const str =
        XSTR(LAME_MAJOR_VERSION) "." XSTR(LAME_MINOR_VERSION) " (alpha)";
#elif LAME_BETA_VERSION > 0
    static /*@observer@*/ const char *const str =
        XSTR(LAME_MAJOR_VERSION) "." XSTR(LAME_MINOR_VERSION) " (beta)";
#else
    static /*@observer@*/ const char *const str =
        XSTR(LAME_MAJOR_VERSION) "." XSTR(LAME_MINOR_VERSION)
#endif

    return str;
}


//! Get the version string for GPSYCHO.
/*!
  \param void
  \return a pointer to a string which describes the version of GPSYCHO.
*/
const char*  get_psy_version ( void )
{
#if   PSY_ALPHA_VERSION > 0
    static /*@observer@*/ const char *const str =
        XSTR(PSY_MAJOR_VERSION) "." XSTR(PSY_MINOR_VERSION)
        " (alpha " XSTR(PSY_ALPHA_VERSION) ", " __DATE__ " " __TIME__ ")";
#elif PSY_BETA_VERSION > 0
    static /*@observer@*/ const char *const str =
        XSTR(PSY_MAJOR_VERSION) "." XSTR(PSY_MINOR_VERSION)
        " (beta " XSTR(PSY_BETA_VERSION) ", " __DATE__ ")";
#else
    static /*@observer@*/ const char *const str =
        XSTR(PSY_MAJOR_VERSION) "." XSTR(PSY_MINOR_VERSION);
#endif

    return str;
}


//! Get the mp3x version string.
/*!
  \param void
  \return a pointer to a string which describes the version of mp3x.
*/
const char*  get_mp3x_version ( void )
{
#if   MP3X_ALPHA_VERSION > 0
    static /*@observer@*/ const char *const str =
        XSTR(MP3X_MAJOR_VERSION) "." XSTR(MP3X_MINOR_VERSION)
        " (alpha " XSTR(MP3X_ALPHA_VERSION) ", " __DATE__ " " __TIME__ ")";
#elif MP3X_BETA_VERSION > 0
    static /*@observer@*/ const char *const str =
        XSTR(MP3X_MAJOR_VERSION) "." XSTR(MP3X_MINOR_VERSION)
        " (beta " XSTR(MP3X_BETA_VERSION) ", " __DATE__ ")";
#else
    static /*@observer@*/ const char *const str =
        XSTR(MP3X_MAJOR_VERSION) "." XSTR(MP3X_MINOR_VERSION);
#endif

    return str;
}


//! Get the URL for the LAME website.
/*!
  \param void
  \return a pointer to a string which is a URL for the LAME website.
*/
const char*  get_lame_url ( void )
{
    static /*@observer@*/ const char *const str = LAME_URL;

    return str;
}    


//! Get the numerical representation of the version.
/*!
  Writes the numerical representation of the version of LAME and
  GPSYCHO into lvp.

  \param lvp    
*/
void get_lame_version_numerical ( lame_version_t *const lvp )
{
    static /*@observer@*/ const char *const features = V;

    /* generic version */
    lvp->major = LAME_MAJOR_VERSION;
    lvp->minor = LAME_MINOR_VERSION;
    lvp->alpha = LAME_ALPHA_VERSION;
    lvp->beta  = LAME_BETA_VERSION;

    /* psy version */
    lvp->psy_major = PSY_MAJOR_VERSION;
    lvp->psy_minor = PSY_MINOR_VERSION;
    lvp->psy_alpha = PSY_ALPHA_VERSION;
    lvp->psy_beta  = PSY_BETA_VERSION;

    /* compile time features */
    /*@-mustfree@*/
    lvp->features = features;
    /*@=mustfree@*/
}

/* end of version.c */
