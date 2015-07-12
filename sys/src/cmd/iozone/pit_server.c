/******************************************************************************
* File: pit_server.c
*
* Description: Contains source code for an IPv6-capable 'PIT' server.
* This is a derivative of the tod6 (time-of-day) server that was written 
* by John Wenker.
* .......
* Author of tod6: John Wenker, Sr. Software Engineer,
*                  Performance Technologies, San Diego, USA
* .......
* The program tod6 was a time of day server. It has beeen modified
* to provide a microsecond timestamp on request.  Modified and adapted 
* for PIT purposes by Don Capps.  [ capps@iozone.org ]
* 
* This server sends the current value of gettimeofday() in 
* microseconds back to the client, as a numerical string.
*
* /etc/services should contain "PIT" with a specified port value.
*
******************************************************************************/
/*
** System header files.
*/
#include <errno.h>        /* errno declaration & error codes.            */
#include <netdb.h>        /* getaddrinfo(3) et al.                       */
#include <netinet/in.h>   /* sockaddr_in & sockaddr_in6 definition.      */
#include <stdio.h>        /* printf(3) et al.                            */
#include <stdlib.h>       /* exit(2).                                    */
#include <string.h>       /* String manipulation & memory functions.     */
#if defined(_SUA_)
#include <poll.h>         /* poll(2) and related definitions.            */
#else
#include <sys/poll.h>     /* poll(2) and related definitions.            */
#endif
#include <sys/socket.h>   /* Socket functions (socket(2), bind(2), etc). */
#include <time.h>         /* time(2) & ctime(3).                         */
#include <sys/time.h>     /* gettimeofday 				 */
#include <unistd.h>       /* getopt(3), read(2), etc.                    */
/* Include for Cygnus development environment for Windows */
#if defined (Windows)
#include <Windows.h>
int errno;
#endif

#if defined(_SUA_)
extern char *optarg, *opterr;
#endif

/*
** Constants.
**
** Please remember to add PIT service to the /etc/services file.
*/
#define DFLT_SERVICE "PIT"       /* Programmable Interdimensional Timer      */

#define INVALID_DESC -1          /* Invalid file descriptor.                 */
#define MAXCONNQLEN  3           /* Max nbr of connection requests to queue. */
#define MAXTCPSCKTS  2           /* One TCP socket for IPv4 & one for IPv6.  */
#define MAXUDPSCKTS  2           /* One UDP socket for IPv4 & one for IPv6.  */
#define VALIDOPTS    "vh:p:"     /* Valid command options.                   */
/*
** Simple boolean type definition.
*/
int false = 0;
int true = 1;
/*
** Prototypes for internal helper functions.
*/
static int  openSckt( const char *service,
                      const char *protocol,
                      int         desc[ ],
                      size_t     *descSize );
static void pit( int    tSckt[ ],
                 size_t tScktSize,
                 int    uSckt[ ],
                 size_t uScktSize );
/*
** Global data objects.
*/
static char        hostBfr[ NI_MAXHOST ];   /* For use w/getnameinfo(3).    */
static const char *pgmName;                 /* Program name w/o dir prefix. */
static char        servBfr[ NI_MAXSERV ];   /* For use w/getnameinfo(3).    */
static int     	   verbose = 0;             /* Verbose mode indication.     */
struct timeval tm;  /* Timeval structure, used with gettimeofday() */
char   timeStr[40]; /* String for time in microseconds */
char service_name[20];
int need;
/*
** Usage macro for command syntax violations.
*/
#define USAGE                                       \
        {                                           \
           fprintf( stderr,                         \
                    "Usage: %s [-v] -p service \n",   \
                    pgmName );                      \
           exit( 127 );                             \
        }  /* End USAGE macro. */
/*
** Macro to terminate the program if a system call error occurs.  The system
** call must be one of the usual type that returns -1 on error.  
*/
#define CHK(expr)                                                   \
        do                                                          \
        {                                                           \
           if ( (expr) == -1 )                                      \
           {                                                        \
              fprintf( stderr,                                      \
                       "%s (line %d): System call ERROR - %s.\n",   \
                       pgmName,                                     \
                       __LINE__,                                    \
                       strerror( errno ) );                         \
              exit( 1 );                                            \
           }   /* End IF system call failed. */                     \
        } while ( false )
/******************************************************************************
* Function: main
*
* Description:
*    Set up a PIT server and handle network requests.  This server
*    handles both TCP and UDP requests.
*
* Parameters:
*    The usual argc and argv parameters to a main() function.
*
* Return Value:
*    This is a daemon program and never returns.  However, in the degenerate
*    case where no sockets are created, the function returns zero.
******************************************************************************/
int main( int   argc,
          char *argv[ ] )
{
   int         opt;
   int         tSckt[ MAXTCPSCKTS ];     /* Array of TCP socket descriptors. */
   size_t      tScktSize = MAXTCPSCKTS;  /* Size of uSckt (# of elements).   */
   int         uSckt[ MAXUDPSCKTS ];     /* Array of UDP socket descriptors. */
   size_t      uScktSize = MAXUDPSCKTS;  /* Size of uSckt (# of elements).   */

   strcpy(service_name,DFLT_SERVICE);
   /*
   ** Set the program name (w/o directory prefix).
   */
   pgmName = strrchr( argv[ 0 ], '/' );
   pgmName = pgmName == NULL  ?  argv[ 0 ]  :  pgmName + 1;
   /*
   ** Process command options.
   */
   opterr = 0;   /* Turns off "invalid option" error messages. */
   while ( ( opt = getopt( argc, argv, VALIDOPTS ) ) >= 0 )
   {
      switch ( opt )
      {
         case 'v':   /* Verbose mode. */
         {
            verbose = true;
            break;
         }
         case 'p':   /* Get the port number */
         {
            strcpy(service_name,optarg);
	    need++;
            break;
         }
         default:
         {
            USAGE;
         }
      }  /* End SWITCH on command option. */
   }  /* End WHILE processing options. */

   if(need < 1)
   {
	USAGE;
	exit;
   }
   /*
   ** Open both a TCP and UDP socket, for both IPv4 & IPv6, on which to receive
   ** service requests.
   */
   if ( ( openSckt( service_name, "tcp", tSckt, &tScktSize ) < 0 ) ||
        ( openSckt( service_name, "udp", uSckt, &uScktSize ) < 0 ) )
   {
      exit( 1 );
   }
   /*
   ** Run the Programmable Interdimensional Timer server.
   */
   if ( ( tScktSize > 0 ) || ( uScktSize > 0 ) )
   {
      pit( tSckt,         /* pit() never returns. */
           tScktSize,
           uSckt,
           uScktSize );
   }
   /*
   ** Since pit() never returns, execution only gets here if no sockets were
   ** created.
   */
   if ( verbose )
   {
      fprintf( stderr,
               "%s: No sockets opened... terminating.\n",
               pgmName );
   }
   return 0;
}  /* End main() */
/******************************************************************************
* Function: openSckt
*
* Description:
*    Open passive (server) sockets for the indicated inet service & protocol.
*    Notice in the last sentence that "sockets" is plural.  During the interim
*    transition period while everyone is switching over to IPv6, the server
*    application has to open two sockets on which to listen for connections...
*    one for IPv4 traffic and one for IPv6 traffic.
*
* Parameters:
*    service  - Pointer to a character string representing the well-known port
*               on which to listen (can be a service name or a decimal number).
*    protocol - Pointer to a character string representing the transport layer
*               protocol (only "tcp" or "udp" are valid).
*    desc     - Pointer to an array into which the socket descriptors are
*               placed when opened.
*    descSize - This is a value-result parameter.  On input, it contains the
*               max number of descriptors that can be put into 'desc' (i.e. the
*               number of elements in the array).  Upon return, it will contain
*               the number of descriptors actually opened.  Any unused slots in
*               'desc' are set to INVALID_DESC.
*
* Return Value:
*    0 on success, -1 on error.
******************************************************************************/
static int openSckt( const char *service,
                     const char *protocol,
                     int         desc[ ],
                     size_t     *descSize )
{
   struct addrinfo *ai;
   int              aiErr;
   struct addrinfo *aiHead;
   struct addrinfo  hints    = { .ai_flags  = AI_PASSIVE,    /* Server mode.  */
                                 .ai_family = PF_UNSPEC };   /* IPv4 or IPv6. */
   size_t           maxDescs = *descSize;
   /*
   ** Initialize output parameters.  When the loop completes, *descSize is 0.
   */
   while ( *descSize > 0 )
   {
      desc[ --( *descSize ) ] = INVALID_DESC;
   }
   /*
   ** Check which protocol is selected (only TCP and UDP are valid).
   */
   if ( strcmp( protocol, "tcp" ) == 0 )        /* TCP protocol.     */
   {
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_protocol = IPPROTO_TCP;
   }
   else if ( strcmp( protocol, "udp" ) == 0 )   /* UDP protocol.     */
   {
      hints.ai_socktype = SOCK_DGRAM;
      hints.ai_protocol = IPPROTO_UDP;
   }
   else                                         /* Invalid protocol. */
   {
      fprintf( stderr,
               "%s (line %d): ERROR - Unknown transport "
               "layer protocol \"%s\".\n",
               pgmName,
               __LINE__,
               protocol );
      return -1;
   }
   /*
   ** Look up the service's "well-known" port number.  Notice that NULL is being
   ** passed for the 'node' parameter, and that the AI_PASSIVE flag is set in
   ** 'hints'.  Thus, the program is requesting passive address information.
   ** The network address is initialized to :: (all zeros) for IPv6 records, or
   ** 0.0.0.0 for IPv4 records.
   */
   if ( ( aiErr = getaddrinfo( NULL,
                               service,
                               &hints,
                               &aiHead ) ) != 0 )
   {
      fprintf( stderr,
               "%s (line %d): ERROR - %s.\n",
               pgmName,
               __LINE__,
               gai_strerror( aiErr ) );
      return -1;
   }
   /*
   ** For each of the address records returned, attempt to set up a passive
   ** socket.
   */
   for ( ai = aiHead;
         ( ai != NULL ) && ( *descSize < maxDescs );
         ai = ai->ai_next )
   {
      if ( verbose )
      {
         /*
         ** Display the current address info.   Start with the protocol-
         ** independent fields first.
         */
         fprintf( stderr,
                  "Setting up a passive socket based on the "
                  "following address info:\n"
                  "   ai_flags     = 0x%02X\n"
                  "   ai_family    = %d (PF_INET = %d, PF_INET6 = %d)\n"
                  "   ai_socktype  = %d (SOCK_STREAM = %d, SOCK_DGRAM = %d)\n"
                  "   ai_protocol  = %d (IPPROTO_TCP = %d, IPPROTO_UDP = %d)\n"
                  "   ai_addrlen   = %d (sockaddr_in = %lu, "
                  "sockaddr_in6 = %lu)\n",
                  ai->ai_flags,
                  ai->ai_family,
                  PF_INET,
                  PF_INET6,
                  ai->ai_socktype,
                  SOCK_STREAM,
                  SOCK_DGRAM,
                  ai->ai_protocol,
                  IPPROTO_TCP,
                  IPPROTO_UDP,
                  ai->ai_addrlen,
                  sizeof( struct sockaddr_in ),
                  sizeof( struct sockaddr_in6 ) );
         /*
         ** Now display the protocol-specific formatted socket address.  Note
         ** that the program is requesting that getnameinfo(3) convert the
         ** host & service into numeric strings.
         */
         getnameinfo( ai->ai_addr,
                      ai->ai_addrlen,
                      hostBfr,
                      sizeof( hostBfr ),
                      servBfr,
                      sizeof( servBfr ),
                      NI_NUMERICHOST | NI_NUMERICSERV );
         switch ( ai->ai_family )
         {
            case PF_INET:   /* IPv4 address record. */
            {
               struct sockaddr_in *p = (struct sockaddr_in*) ai->ai_addr;
               fprintf( stderr,
                        "   ai_addr      = sin_family:   %d (AF_INET = %d, "
                        "AF_INET6 = %d)\n"
                        "                  sin_addr:     %s\n"
                        "                  sin_port:     %s\n",
                        p->sin_family,
                        AF_INET,
                        AF_INET6,
                        hostBfr,
                        servBfr );
               break;
            }  /* End CASE of IPv4. */
            case PF_INET6:   /* IPv6 address record. */
            {
               struct sockaddr_in6 *p = (struct sockaddr_in6*) ai->ai_addr;
               fprintf( stderr,
                        "   ai_addr      = sin6_family:   %d (AF_INET = %d, "
                        "AF_INET6 = %d)\n"
                        "                  sin6_addr:     %s\n"
                        "                  sin6_port:     %s\n"
                        "                  sin6_flowinfo: %d\n"
                        "                  sin6_scope_id: %d\n",
                        p->sin6_family,
                        AF_INET,
                        AF_INET6,
                        hostBfr,
                        servBfr,
                        p->sin6_flowinfo,
                        p->sin6_scope_id );
               break;
            }  /* End CASE of IPv6. */
            default:   /* Can never get here, but just for completeness. */
            {
               fprintf( stderr,
                        "%s (line %d): ERROR - Unknown protocol family (%d).\n",
                        pgmName,
                        __LINE__,
                        ai->ai_family );
               freeaddrinfo( aiHead );
               return -1;
            }  /* End DEFAULT case (unknown protocol family). */
         }  /* End SWITCH on protocol family. */
      }  /* End IF verbose mode. */
      /*
      ** Create a socket using the info in the addrinfo structure.
      */
      CHK( desc[ *descSize ] = socket( ai->ai_family,
                                       ai->ai_socktype,
                                       ai->ai_protocol ) );
      /*
      ** Here is the code that prevents "IPv4 mapped addresses", as discussed
      ** in Section 22.1.3.1.  If an IPv6 socket was just created, then set the
      ** IPV6_V6ONLY socket option.
      */
      if ( ai->ai_family == PF_INET6 )
      {
#if defined( IPV6_V6ONLY )
         /*
         ** Disable IPv4 mapped addresses.
         */
         int v6Only = 1;
         CHK( setsockopt( desc[ *descSize ],
                          IPPROTO_IPV6,
                          IPV6_V6ONLY,
                          &v6Only,
                          sizeof( v6Only ) ) );
#else
         /*
         ** IPV6_V6ONLY is not defined, so the socket option can't be set and
         ** thus IPv4 mapped addresses can't be disabled.  Print a warning
         ** message and close the socket.  Design note: If the
         ** #if...#else...#endif construct were removed, then this program
         ** would not compile (because IPV6_V6ONLY isn't defined).  That's an
         ** acceptable approach; IPv4 mapped addresses are certainly disabled
         ** if the program can't build!  However, since this program is also
         ** designed to work for IPv4 sockets as well as IPv6, I decided to
         ** allow the program to compile when IPV6_V6ONLY is not defined, and
         ** turn it into a run-time warning rather than a compile-time error.
         ** IPv4 mapped addresses are still disabled because _all_ IPv6 traffic
         ** is disabled (all IPv6 sockets are closed here), but at least this
         ** way the server can still service IPv4 network traffic.
         */
         fprintf( stderr,
                  "%s (line %d): WARNING - Cannot set IPV6_V6ONLY socket "
                  "option.  Closing IPv6 %s socket.\n",
                  pgmName,
                  __LINE__,
                  ai->ai_protocol == IPPROTO_TCP  ?  "TCP"  :  "UDP" );
         CHK( close( desc[ *descSize ] ) );
         continue;   /* Go to top of FOR loop w/o updating *descSize! */
#endif /* IPV6_V6ONLY */
      }  /* End IF this is an IPv6 socket. */
      /*
      ** Bind the socket.  Again, the info from the addrinfo structure is used.
      */
      CHK( bind( desc[ *descSize ],
                 ai->ai_addr,
                 ai->ai_addrlen ) );
      /*
      ** If this is a TCP socket, put the socket into passive listening mode
      ** (listen is only valid on connection-oriented sockets).
      */
      if ( ai->ai_socktype == SOCK_STREAM )
      {
         CHK( listen( desc[ *descSize ],
                      MAXCONNQLEN ) );
      }
      /*
      ** Socket set up okay.  Bump index to next descriptor array element.
      */
      *descSize += 1;
   }  /* End FOR each address info structure returned. */
   /*
   ** Dummy check for unused address records.
   */
   if ( verbose && ( ai != NULL ) )
   {
      fprintf( stderr,
               "%s (line %d): WARNING - Some address records were "
               "not processed due to insufficient array space.\n",
               pgmName,
               __LINE__ );
   }  /* End IF verbose and some address records remain unprocessed. */
   /*
   ** Clean up.
   */
   freeaddrinfo( aiHead );
   return 0;
}  /* End openSckt() */
/******************************************************************************
* Function: pit
*
* Description:
*    Listen on a set of sockets and send the current microsecond counter
*    that was produced by gettimeofday(), to any clients.  This function 
*    never returns.
*
* Parameters:
*    tSckt     - Array of TCP socket descriptors on which to listen.
*    tScktSize - Size of the tSckt array (nbr of elements).
*    uSckt     - Array of UDP socket descriptors on which to listen.
*    uScktSize - Size of the uSckt array (nbr of elements).
*
* Return Value: None.
******************************************************************************/
static void pit( int    tSckt[ ],
                 size_t tScktSize,
                 int    uSckt[ ],
                 size_t uScktSize )
{
   char                     bfr[ 256 ];
   ssize_t                  count;
   struct pollfd           *desc;
   size_t                   descSize = tScktSize + uScktSize;
   int                      idx;
   int                      newSckt;
   struct sockaddr         *sadr;
   socklen_t                sadrLen;
   struct sockaddr_storage  sockStor;
   int                      status;
   size_t                   timeLen;
   time_t                   timeVal;
   ssize_t                  wBytes;
   unsigned long long	    secs;
   int 	    		    ret;
   /*
   ** Allocate memory for the poll(2) array.
   */
   desc = malloc( descSize * sizeof( struct pollfd ) );
   if ( desc == NULL )
   {
      fprintf( stderr,
               "%s (line %d): ERROR - %s.\n",
               pgmName,
               __LINE__,
               strerror( ENOMEM ) );
      exit( 1 );
   }
   /*
   ** Initialize the poll(2) array.
   */
   for ( idx = 0;     idx < descSize;     idx++ )
   {
      desc[ idx ].fd      = idx < tScktSize  ?  tSckt[ idx ]
                                             :  uSckt[ idx - tScktSize ];
      desc[ idx ].events  = POLLIN;
      desc[ idx ].revents = 0;
   }
   /*
   ** Main PIT server loop.  Handles both TCP & UDP requests.  This is
   ** an interative server, and all requests are handled directly within the
   ** main loop.
   */
   while ( true )   /* Do forever. */
   {
      /*
      ** Wait for activity on one of the sockets.  The DO..WHILE construct is
      ** used to restart the system call in the event the process is
      ** interrupted by a signal.
      */
      do
      {
         status = poll( desc,
                        descSize,
                        -1 /* Wait indefinitely for input. */ );
      } while ( ( status < 0 ) && ( errno == EINTR ) );
      CHK( status );   /* Check for a bona fide system call error. */
      /*
      ** Get the current time.
      */
#if defined(Windows)
   LARGE_INTEGER freq,counter;
   double wintime,bigcounter;
   /* For Windows the time_of_day() is useless. It increments in 55 milli 
    * second increments. By using the Win32api one can get access to the 
    * high performance measurement interfaces. With this one can get back 
    * into the 8 to 9 microsecond resolution.
    */
      QueryPerformanceFrequency(&freq);
      QueryPerformanceCounter(&counter);
      bigcounter=(double)counter.HighPart *(double)0xffffffff +
              (double)counter.LowPart;
      wintime = (double)(bigcounter/(double)freq.LowPart);
      secs = (long long)(wintime * 1000000);
#else
      ret = gettimeofday( &tm,0 );
      secs = ((unsigned long long)tm.tv_sec * 1000000) 
		+ (unsigned long long)tm.tv_usec;
#endif

      ret = sprintf(timeStr,"%llu",secs);
      timeLen = strlen( timeStr );
      /*
      ** Process sockets with input available.
      */
      for ( idx = 0;     idx < descSize;     idx++ )
      {
         switch ( desc[ idx ].revents )
         {
            case 0:        /* No activity on this socket; try the next. */
               continue;
            case POLLIN:   /* Network activity.  Go process it.         */
               break;
            default:       /* Invalid poll events.                      */
            {
               fprintf( stderr,
                        "%s (line %d): ERROR - Invalid poll event (0x%02X).\n",
                        pgmName,
                        __LINE__,
                        desc[ idx ].revents );
               exit( 1 );
            }
         }  /* End SWITCH on returned poll events. */
         /*
         ** Determine if this is a TCP request or UDP request.
         */
         if ( idx < tScktSize )
         {
            /*
            ** TCP connection requested.  Accept it.  Notice the use of
            ** the sockaddr_storage data type.
            */
            sadrLen = sizeof( sockStor );
            sadr    = (struct sockaddr*) &sockStor;
            CHK( newSckt = accept( desc[ idx ].fd,
                                   sadr,
                                   &sadrLen ) );
            CHK( shutdown( newSckt,       /* Server never recv's anything. */
                           SHUT_RD ) );
            if ( verbose )
            {
               /*
               ** Display the socket address of the remote client.  Begin with
               ** the address-independent fields.
               */
               fprintf( stderr,
                        "Sockaddr info for new TCP client:\n"
                        "   sa_family = %d (AF_INET = %d, AF_INET6 = %d)\n"
                        "   addr len  = %d (sockaddr_in = %lu, "
                        "sockaddr_in6 = %lu)\n",
                        sadr->sa_family,
                        AF_INET,
                        AF_INET6,
                        sadrLen,
                        sizeof( struct sockaddr_in ),
                        sizeof( struct sockaddr_in6 ) );
               /*
               ** Display the address-specific fields.
               */
               getnameinfo( sadr,
                            sadrLen,
                            hostBfr,
                            sizeof( hostBfr ),
                            servBfr,
                            sizeof( servBfr ),
                            NI_NUMERICHOST | NI_NUMERICSERV );
               /*
               ** Notice that we're switching on an address family now, not a
               ** protocol family.
               */
               switch ( sadr->sa_family )
               {
                  case AF_INET:   /* IPv4 address. */
                  {
                     struct sockaddr_in *p = (struct sockaddr_in*) sadr;
                     fprintf( stderr,
                              "   sin_addr  = sin_family: %d\n"
                              "               sin_addr:   %s\n"
                              "               sin_port:   %s\n",
                              p->sin_family,
                              hostBfr,
                              servBfr );
                     break;
                  }  /* End CASE of IPv4. */
                  case AF_INET6:   /* IPv6 address. */
                  {
                     struct sockaddr_in6 *p = (struct sockaddr_in6*) sadr;
                     fprintf( stderr,
                              "   sin6_addr = sin6_family:   %d\n"
                              "               sin6_addr:     %s\n"
                              "               sin6_port:     %s\n"
                              "               sin6_flowinfo: %d\n"
                              "               sin6_scope_id: %d\n",
                              p->sin6_family,
                              hostBfr,
                              servBfr,
                              p->sin6_flowinfo,
                              p->sin6_scope_id );
                     break;
                  }  /* End CASE of IPv6. */
                  default:   /* Can never get here, but for completeness. */
                  {
                     fprintf( stderr,
                              "%s (line %d): ERROR - Unknown address "
                              "family (%d).\n",
                              pgmName,
                              __LINE__,
                              sadr->sa_family );
                     break;
                  }  /* End DEFAULT case (unknown address family). */
               }  /* End SWITCH on address family. */
            }  /* End IF verbose mode. */
            /*
            ** Send the PIT to the client.
            */
            wBytes = timeLen;
            while ( wBytes > 0 )
            {
               do
               {
                  count = write( newSckt,
                                 timeStr,
                                 wBytes );
               } while ( ( count < 0 ) && ( errno == EINTR ) );
               CHK( count );   /* Check for an error. */
               wBytes -= count;
            }  /* End WHILE there is data to send. */
            CHK( close( newSckt ) );
         }  /* End IF this was a TCP connection request. */
         else
         {
            /*
            ** This is a UDP socket, and a datagram is available.  The funny
            ** thing about UDP requests is that this server doesn't require any
            ** client input; but it can't send the PIT unless it knows a client
            ** wants the data, and the only way that can occur with UDP is if
            ** the server receives a datagram from the client.  Thus, the
            ** server must receive _something_, but the content of the datagram
            ** is irrelevant.  Read in the datagram.  Again note the use of
            ** sockaddr_storage to receive the address.
            */
            sadrLen = sizeof( sockStor );
            sadr    = (struct sockaddr*) &sockStor;
            CHK( count = recvfrom( desc[ idx ].fd,
                                   bfr,
                                   sizeof( bfr ),
                                   0,
                                   sadr,
                                   &sadrLen ) );
            /*
            ** Display whatever was received on stdout.
            */
            if ( verbose )
            {
               ssize_t rBytes = count;
               fprintf( stderr,
                        "%s: UDP datagram received (%ld bytes).\n",
                        pgmName,
                        count );
               while ( count > 0 )
               {
                  fputc( bfr[ rBytes - count-- ],
                         stdout );
               }
               if ( bfr[ rBytes-1 ] != '\n' )
                  fputc( '\n', stdout );   /* Newline also flushes stdout. */
               /*
               ** Display the socket address of the remote client.  Address-
               ** independent fields first.
               */
               fprintf( stderr,
                        "Remote client's sockaddr info:\n"
                        "   sa_family = %d (AF_INET = %d, AF_INET6 = %d)\n"
                        "   addr len  = %d (sockaddr_in = %lu, "
                        "sockaddr_in6 = %lu)\n",
                        sadr->sa_family,
                        AF_INET,
                        AF_INET6,
                        sadrLen,
                        sizeof( struct sockaddr_in ),
                        sizeof( struct sockaddr_in6 ) );
               /*
               ** Display the address-specific information.
               */
               getnameinfo( sadr,
                            sadrLen,
                            hostBfr,
                            sizeof( hostBfr ),
                            servBfr,
                            sizeof( servBfr ),
                            NI_NUMERICHOST | NI_NUMERICSERV );
               switch ( sadr->sa_family )
               {
                  case AF_INET:   /* IPv4 address. */
                  {
                     struct sockaddr_in *p = (struct sockaddr_in*) sadr;
                     fprintf( stderr,
                              "   sin_addr  = sin_family: %d\n"
                              "               sin_addr:   %s\n"
                              "               sin_port:   %s\n",
                              p->sin_family,
                              hostBfr,
                              servBfr );
                     break;
                  }  /* End CASE of IPv4 address. */
                  case AF_INET6:   /* IPv6 address. */
                  {
                     struct sockaddr_in6 *p = (struct sockaddr_in6*) sadr;
                     fprintf( stderr,
                              "   sin6_addr = sin6_family:   %d\n"
                              "               sin6_addr:     %s\n"
                              "               sin6_port:     %s\n"
                              "               sin6_flowinfo: %d\n"
                              "               sin6_scope_id: %d\n",
                              p->sin6_family,
                              hostBfr,
                              servBfr,
                              p->sin6_flowinfo,
                              p->sin6_scope_id );
                     break;
                  }  /* End CASE of IPv6 address. */
                  default:   /* Can never get here, but for completeness. */
                  {
                     fprintf( stderr,
                              "%s (line %d): ERROR - Unknown address "
                              "family (%d).\n",
                              pgmName,
                              __LINE__,
                              sadr->sa_family );
                     break;
                  }  /* End DEFAULT case (unknown address family). */
               }  /* End SWITCH on address family. */
            }  /* End IF verbose mode. */
            /*
            ** Send the PIT to the client.
            */
            wBytes = timeLen;
            while ( wBytes > 0 )
            {
               do
               {
                  count = sendto( desc[ idx ].fd,
                                  timeStr,
                                  wBytes,
                                  0,
                                  sadr,        /* Address & address length   */
                                  sadrLen );   /*    received in recvfrom(). */
               } while ( ( count < 0 ) && ( errno == EINTR ) );
               CHK( count );   /* Check for a bona fide error. */
               wBytes -= count;
            }  /* End WHILE there is data to send. */
         }  /* End ELSE a UDP datagram is available. */
         desc[ idx ].revents = 0;   /* Clear the returned poll events. */
      }  /* End FOR each socket descriptor. */
   }  /* End WHILE forever. */
}  /* End pit() */

