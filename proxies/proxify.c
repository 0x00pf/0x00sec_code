/*
  How does those hackers tools work?. ProxyChains.  
  proxify library
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

/* GNU extension for RTDL_NEXT definition */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>


int connect(int sockfd, const struct sockaddr *addr,
	    socklen_t addrlen)
{
  int                 s, port;
  struct sockaddr_in  serv, *cli;
  char                req[1024], *ip;
  int                 (*real_connect) (int sockfd, 
				       const struct sockaddr *addr,
				       socklen_t addrlen);
  

  /* get pointer to original connect function */
  real_connect = dlsym (RTLD_NEXT, "connect");

  /* Obtain text info to build the proxy connection request */
  cli = (struct sockaddr_in*)addr;

  ip   = inet_ntoa (cli->sin_addr);
  port = ntohs (cli->sin_port);

  /* Create a new socket as the other one is currently trying to connect*/
  /* Otherwise we get a 'connect:: Operation now in progress' error */
  if ((s = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
      perror ("socket:");
      exit (EXIT_FAILURE);
    }

  /* Swap file descriptors */
  close (sockfd);
  dup2 (s,sockfd);

  /* Connect to proxy */
  serv.sin_family = AF_INET;
  /* FIXME: You should check if the env vars exist, and
   *        fallback to a default value otherwise */
  serv.sin_port = htons(atoi(getenv("MY_PROXY_PORT")));
  serv.sin_addr.s_addr = inet_addr(getenv("MY_PROXY"));

  if (real_connect (s, (struct sockaddr *) &serv, sizeof(serv)) < 0) 
    {
      perror("connect:");
      exit (EXIT_FAILURE);
    }

  /* Send proxy connection requests... Only one request in this example */
  sprintf (req, "GET http://%s:%d HTTP/1.1\r\n\r\n", ip, port);
  write (s, req, strlen(req));

  return 0;
}
