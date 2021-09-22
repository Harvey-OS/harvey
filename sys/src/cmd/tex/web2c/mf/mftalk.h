/* mftalk.h -- METAFONT server/client protocol (not as complex as
   it sounds).

   Copyright (C) 1994 Ralph Schleicher  */

/* This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef MFTALK_H
#define MFTALK_H

/* This is the server to client protocol:

   'r' [0|1] <left> <bottom> <right> <top>	Fill rectangle.
   'l' [0|1] <col> <row> <num> <col> ...	Draw a line.
   'f'						Update screen.
   ^D						End of transmission.

   And there is one client to server command:

   ^F						Acknowledgment.  */

#define MF_RECT		((int) 'r')
#define MF_LINE		((int) 'l')
#define MF_FLUSH	((int) 'f')
#define MF_EXIT		((int) ('D' - 64))
#define MF_ACK		((int) ('F' - 64))

#define	MF_WHITE	0
#define	MF_BLACK	1

#endif /* !MFTALK_H */
