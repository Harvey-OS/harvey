/*
 *   httpget.c
 *
 *   Oliver Fromme  <oliver.fromme@heim3.tu-clausthal.de>
 *   Wed Apr  9 20:57:47 MET DST 1997
 */

#undef ALSA

#if !defined(WIN32) && !defined(GENERIC)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef PLAN9
#include <errno.h>
#else
#include <sys/errno.h>
#endif
#include <ctype.h>


extern int errno;

#include "mpg123.h"

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

void writestring (int fd, char *string)
{
	int result, bytes = strlen(string);

	while (bytes) {
		if ((result = write(fd, string, bytes)) < 0 && errno != EINTR) {
			perror ("write");
			exit (1);
		}
		else if (result == 0) {
			fprintf (stderr, "write: %s\n",
				"socket closed unexpectedly");
			exit (1);
		}
		string += result;
		bytes -= result;
	}
}

void readstring (char *string, int maxlen, FILE *f)
{
#if 0
	char *result;
#endif
	int pos = 0;

	while(1) {
		if( read(fileno(f),string+pos,1) == 1) {
			pos++;
			if(string[pos-1] == '\n') {
				string[pos] = 0;
				break;
			}
		}
		else if(errno != EINTR) {
			fprintf (stderr, "Error reading from socket or unexpected EOF.\n");
			exit(1);
		}
	}
#if 0
	do {
		result = fgets(string, maxlen, f);
	} while (!result  && errno == EINTR);
	if (!result) {
		fprintf (stderr, "Error reading from socket or unexpected EOF.\n");
		exit (1);
	}
#endif

}

void encode64 (char *source,char *destination)
{
  static char *Base64Digits =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int n = 0;
  int ssiz=strlen(source);
  int i;

  for (i = 0 ; i < ssiz ; i += 3) {
    unsigned int buf;
    buf = ((unsigned char *)source)[i] << 16;
    if (i+1 < ssiz)
      buf |= ((unsigned char *)source)[i+1] << 8;
    if (i+2 < ssiz)
      buf |= ((unsigned char *)source)[i+2];

    destination[n++] = Base64Digits[(buf >> 18) % 64];
    destination[n++] = Base64Digits[(buf >> 12) % 64];
    if (i+1 < ssiz)
      destination[n++] = Base64Digits[(buf >> 6) % 64];
    else
      destination[n++] = '=';
    if (i+2 < ssiz)
      destination[n++] = Base64Digits[buf % 64];
    else
      destination[n++] = '=';
  }
  destination[n++] = 0;
}

/* VERY  simple auth-from-URL grabber */
int getauthfromURL(char *url,char *auth)
{
  char *pos;

  *auth = 0;

  if (!(strncmp(url, "http://", 7)))
    url += 7;

  if( (pos = strchr(url,'@')) ) {
    int i;
    for(i=0;i<pos-url;i++) {
      if( url[i] == '/' )
         return 0;
    }
    strncpy(auth,url,pos-url);
    auth[pos-url] = 0;
    strcpy(url,pos+1);
    return 1;
  }
  return 0;
}

char *url2hostport (char *url, char **hname, unsigned long *hip, unsigned int *port)
{
	char *cptr;
	struct hostent *myhostent;
	struct in_addr myaddr;
	int isip = 1;

	if (!(strncmp(url, "http://", 7)))
		url += 7;
	cptr = url;
	while (*cptr && *cptr != ':' && *cptr != '/') {
		if ((*cptr < '0' || *cptr > '9') && *cptr != '.')
			isip = 0;
		cptr++;
	}
	*hname = strdup(url); /* removed the strndup for better portability */
	if (!(*hname)) {
		*hname = NULL;
		return (NULL);
	}
	(*hname)[cptr - url] = 0;
	if (!isip) {
		if (!(myhostent = gethostbyname(*hname)))
			return (NULL);
		memcpy (&myaddr, myhostent->h_addr, sizeof(myaddr));
		*hip = myaddr.s_addr;
	}
	else
		if ((*hip = inet_addr(*hname)) == INADDR_NONE)
			return (NULL);
	if (!*cptr || *cptr == '/') {
		*port = 80;
		return (cptr);
	}
	*port = atoi(++cptr);
	while (*cptr && *cptr != '/')
		cptr++;
	return (cptr);
}

char *proxyurl = NULL;
unsigned long proxyip = 0;
unsigned int proxyport;

#define ACCEPT_HEAD "Accept: audio/mpeg, audio/x-mpegurl, */*\r\n"

char *httpauth = NULL;
char httpauth1[256];

int http_open (char *url)
{
	char *purl, *host, *request, *sptr;
	int linelength;
	unsigned long myip;
	unsigned int myport;
	int sock;
	int relocate, numrelocs = 0;
	struct sockaddr_in server;
	FILE *myfile;

	if (!proxyip) {
		if (!proxyurl)
			if (!(proxyurl = getenv("MP3_HTTP_PROXY")))
				if (!(proxyurl = getenv("http_proxy")))
					proxyurl = getenv("HTTP_PROXY");
		if (proxyurl && proxyurl[0] && strcmp(proxyurl, "none")) {
			host = NULL;
			if (!(url2hostport(proxyurl, &host, &proxyip, &proxyport))) {
				fprintf (stderr, "Unknown proxy host \"%s\".\n",
					host ? host : "");
				exit (1);
			}
			if (host)
				free (host);
		}
		else
			proxyip = INADDR_NONE;
	}
	
	if ((linelength = strlen(url)+200) < 1024)
		linelength = 1024;
	if (!(request = malloc(linelength)) || !(purl = malloc(1024))) {
		fprintf (stderr, "malloc() failed, out of memory.\n");
		exit (1);
	}
	strncpy (purl, url, 1023);
	purl[1023] = '\0';

        getauthfromURL(purl,httpauth1);

	do {
		strcpy (request, "GET ");
		if (proxyip != INADDR_NONE) {
			if (strncmp(url, "http://", 7))
				strcat (request, "http://");
			strcat (request, purl);
			myport = proxyport;
			myip = proxyip;
		}
		else {
			host = NULL;
			if (!(sptr = url2hostport(purl, &host, &myip, &myport))) {
				fprintf (stderr, "Unknown host \"%s\".\n",
					host ? host : "");
				exit (1);
			}
			strcat (request, sptr);
		}
		sprintf (request + strlen(request),
			" HTTP/1.0\r\nUser-Agent: %s/%s\r\n",
			prgName, prgVersion);
		if (host) {
			sprintf(request + strlen(request),
				"Host: %s:%u\r\n", host, myport);
			free (host);
		}

		strcat (request, ACCEPT_HEAD);
		server.sin_family = AF_INET;
		server.sin_port = htons(myport);
		server.sin_addr.s_addr = myip;
		if ((sock = socket(PF_INET, SOCK_STREAM, 6)) < 0) {
			perror ("socket");
			exit (1);
		}
		if (connect(sock, (struct sockaddr *)&server, sizeof(server))) {
			perror ("connect");
			exit (1);
		}

		if (strlen(httpauth1) || httpauth) {
			char buf[1023];
			strcat (request,"Authorization: Basic ");
                        if(strlen(httpauth1))
                          encode64(httpauth1,buf);
                        else
			  encode64(httpauth,buf);
			strcat (request,buf);
			strcat (request,"\r\n");
		}
		strcat (request, "\r\n");

		writestring (sock, request);
		if (!(myfile = fdopen(sock, "rb"))) {
			perror ("fdopen");
			exit (1);
		};
		relocate = FALSE;
		purl[0] = '\0';
		readstring (request, linelength-1, myfile);
		if ((sptr = strchr(request, ' '))) {
			switch (sptr[1]) {
				case '3':
					relocate = TRUE;
				case '2':
					break;
				default:
					fprintf (stderr, "HTTP request failed: %s",
						sptr+1); /* '\n' is included */
					exit (1);
			}
		}
		do {
			readstring (request, linelength-1, myfile);
			if (!strncmp(request, "Location:", 9))
				strncpy (purl, request+10, 1023);
		} while (request[0] != '\r' && request[0] != '\n');
	} while (relocate && purl[0] && numrelocs++ < 5);
	if (relocate) {
		fprintf (stderr, "Too many HTTP relocations.\n");
		exit (1);
	}
	free (purl);
	free (request);

	return sock;
}

#else
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern int errno;

#include "mpg123.h"

void writestring (int fd, char *string)
{
}

void readstring (char *string, int maxlen, FILE *f)
{
}

char *url2hostport (char *url, char **hname, unsigned long *hip, unsigned int *port)
{
}

char *proxyurl = NULL;
unsigned long proxyip = 0;
unsigned int proxyport;

#define ACCEPT_HEAD "Accept: audio/mpeg, audio/x-mpegurl, */*\r\n"

int http_open (char *url)
{
}
#endif

/* EOF */

