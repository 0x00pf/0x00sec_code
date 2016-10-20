/*
  How does those hackers tools work?. ProxyChains.  
  Simple HTTP Proxy
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

  https://0x00sec.org/t/how-does-those-hackers-tools-work-proxychains/426
**********************************************************************/


#include <stdio.h>
#include <stdlib.h>  
#include <string.h>

#include <unistd.h> 

#include <sys/socket.h>
#include <arpa/inet.h>

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

  int enable = 1;
  if (setsockopt (s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
      perror ("setsockopt(SO_REUSEADDR):");
      exit (EXIT_FAILURE);
    }
  
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
	      if ((len = read (s, buffer, 1024)) <= 0) 	
		{
		  close (s);
		  close (s1);
		  exit (1);
		}

	      write (s1, buffer, len);
	    }
	  if (FD_ISSET(s1, &rfds))
	    {
	      memset (buffer, 0, 1024);
	      if ((len = read (s1, buffer, 1024)) <= 0) 
		{
		  close (s);
		  close (s1);
		  exit (1);
		}


	      write (s, buffer, len);
	    }
	}
    }
}

int
process_request (char *buffer)
{
  int  port, c;
  char *aux;
  char ip[1024];

  sscanf (buffer, "GET http://%s/", ip);
  if ((aux = strchr (ip, ':'))) 
    {
      *aux = 0;
      sscanf (aux+1, "%d", &port);
    }
  else port = 80;

  printf ("Request to connect to: %s(%d)\n", ip, port);
  c = client_init (ip, port);

  return c;
}

int
main (int argc, char *argv[])
{
  int s, c;
  char buffer[2048];

  s = server_init (atoi(argv[1]));
  read (s, buffer, 2048);
  printf ("Request: '%s'\n", buffer);
  c = process_request (buffer);

  async_read (s, c);

  return 0;
}
