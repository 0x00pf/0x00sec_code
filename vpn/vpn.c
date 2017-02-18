/*
  VPNs. tun tunnels
  Copyright (c) 2017 pico

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


#include <stdio.h>
#include <stdlib.h>  
#include <string.h>

#include <unistd.h> 

#include <sys/socket.h>
#include <arpa/inet.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

/* *** Tunnels from kernel **************************************/
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <linux/if.h>
#include <linux/if_tun.h>

/* From Kernel Documentation/networking/tuntap.txt*/
#define BUF_SIZE 1800   // Default MTU is 1500

int
tun_alloc (char *dev)
{
  struct ifreq ifr;
  int fd, err;
  
  if( (fd = open("/dev/net/tun", O_RDWR)) < 0 )
    {
      perror ("open(tun):");
      return -1;
    }
  
  memset(&ifr, 0, sizeof(ifr));
  
  /* Flags: IFF_TUN   - TUN device (no Ethernet headers) 
   *        IFF_TAP   - TAP device  
   *
   *        IFF_NO_PI - Do not provide packet information  
   */ 
  ifr.ifr_flags = IFF_TUN; 
  if( *dev )
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
  
  if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ){
    close(fd);
    return err;
  }
  strcpy(dev, ifr.ifr_name);
  return fd;
}


/* read 1 packet from the tunnel */
/* XXX: buf has to be at least BUF_SIZE. 
 *      This function blocks until the whole packet is read
 */
int
read_pkt (int fd, char *buf)
{
  uint16_t      len;
  int           n, pending;

  // Read Pkt size
  if ((read (fd, &len, sizeof (len))) < 0)
    {
      perror ("read_pkt(size):");
      exit (1);
    }
  len = ntohs (len);
  pending = len;
  while (pending > 0)
    {
      if ((n = read (fd, buf, pending)) < 0)
	{
	  perror ("read_pkt(data):");
	  return 0;
	}
      pending -=  n;
      buf += n;
    }
  return len;
}

int 
write_pkt (int fd, char *buf, uint16_t len)
{
  uint16_t n;
  // Write Packet size
  n = htons (len);
  if ((write (fd, &n, sizeof(n))) < 0)
    {
      perror ("write_pkt(size):");
      exit (1);
    }
  if ((write (fd, buf, len)) < 0)
    {
      perror ("write_pkt(size):");
      exit (1);
    }
  return 0;
}

/* *** EOC *********************************************/
int
server_init (int port)
{
  int                s, s1, val;
  socklen_t          clen;
  struct sockaddr_in serv, client;

  if ((s = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
      perror ("socket:");
      exit (EXIT_FAILURE);
    }
  if (setsockopt (s, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(val)) < 0) 
    {
      perror("setsockopt:");
      exit(1);
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
  fprintf (stderr, "Connection received...\n");
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
  fprintf (stderr, "connected...\n");
  return s;
}

void
async_read (int s, int s1)
{
  fd_set         rfds;
  struct timeval tv;
  int            max = s > s1 ? s : s1;
  int            len, r;
  char           buffer[BUF_SIZE];
  
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
	      len = read_pkt (s, buffer);
	      if ((write (s1, buffer, len)) < 0) 
		{
		  perror ("write(net):");
		  exit (1);
		}
	    }
	  if (FD_ISSET(s1, &rfds))
	    {
	      if ((len = read (s1, buffer, BUF_SIZE)) < 0) 
		{
		  perror ("read(tun):");
		  exit (1);
		}
	      if ((write_pkt (s, buffer, len)) < 0) exit (1);
	    }
	}
    }
}

int
main (int argc, char *argv[])
{
  int  fd;

  /* FIXME: Check command-line arguments */
  if (argv[1][0] == 'c')
    {
      if ((fd = tun_alloc (argv[2])) < 0) exit (1);
      async_read (client_init (argv[3], atoi(argv[4])), fd);
    }
  else if (argv[1][0] == 's')
    {
      if ((fd = tun_alloc (argv[2])) < 0) exit (1);
      async_read (server_init (atoi(argv[3])), fd);
    }
		  
  return 0;
}
