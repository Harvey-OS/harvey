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

#ifndef LAME_VERSION_H
#define LAME_VERSION_H

# include <stdio.h>

/* 
 * To make a string from a token, use the # operator:
 *
 * #define __STR(x)  #x
 * #define STR(x)    __STR(x)
 */

# define LAME_URL              "http://www.mp3dev.org/"

# define LAME_MAJOR_VERSION      3      /* Major version number */
# define LAME_MINOR_VERSION     88      /* Minor version number */
# define LAME_ALPHA_VERSION      0      /* Patch level of alpha version, otherwise zero */
# define LAME_BETA_VERSION       1      /* Patch level of beta  version, otherwise zero */

# define PSY_MAJOR_VERSION       0      /* Major version number */
# define PSY_MINOR_VERSION      85      /* Minor version number */
# define PSY_ALPHA_VERSION       0      /* Set number if this is an alpha version, otherwise zero */
# define PSY_BETA_VERSION        0      /* Set number if this is a beta version, otherwise zero */

# define MP3X_MAJOR_VERSION      0      /* Major version number */
# define MP3X_MINOR_VERSION     82      /* Minor version number */
# define MP3X_ALPHA_VERSION      0      /* Set number if this is an alpha version, otherwise zero */
# define MP3X_BETA_VERSION       0      /* Set number if this is a beta version, otherwise zero */

const char*  get_lame_version       ( void );
const char*  get_lame_short_version ( void );
const char*  get_psy_version        ( void );
const char*  get_mp3x_version       ( void );
const char*  get_lame_url           ( void );
void         get_lame_version_numerical ( lame_version_t *const lvp );

#endif  /* LAME_VERSION_H */

/* End of version.h */
