#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/userfaultfd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <poll.h>

unsigned char x86_hello_bin[] = {
  0xb8, 0x01, 0x00, 0x00, 0x00, 0xbf, 0x01, 0x00, 0x00, 0x00, 0x48, 0x8d,
  0x35, 0x13, 0x00, 0x00, 0x00, 0xba, 0x0d, 0x00, 0x00, 0x00, 0x0f, 0x05,
  0xb8, 0x3c, 0x00, 0x00, 0x00, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x05,
  0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x21,
  0x0a
};
unsigned int x86_hello_bin_len = 49;

int main(void)
{
  int    uffd; 
  size_t pagesize = sysconf(_SC_PAGE_SIZE);
  
  if ((uffd = syscall(SYS_userfaultfd, O_CLOEXEC | O_NONBLOCK)) == -1) return 1;
  
  struct uffdio_api api;
  memset (&api,0, sizeof(struct uffdio_api));
  api.api = UFFD_API;
  
  if (ioctl(uffd, UFFDIO_API, &api) == -1) return 2;

  char *code = mmap(NULL, pagesize, PROT_READ | PROT_EXEC,
		      MAP_PRIVATE | MAP_ANONYMOUS, -1,0);
  
  struct uffdio_register reg;
  reg.range.start = (unsigned long)code;
  reg.range.len = pagesize;
  reg.mode = UFFDIO_REGISTER_MODE_MISSING;

  if (ioctl(uffd, UFFDIO_REGISTER, &reg) == -1) return 3;
  
  char *buf = aligned_alloc(pagesize, pagesize);
  memcpy (buf, x86_hello_bin, x86_hello_bin_len);
  
  struct uffdio_copy copy;
  memset (&copy, 0, sizeof (struct uffdio_copy));
  copy.src = (unsigned long) buf; 
  copy.dst = (unsigned long) code;
  copy.len = pagesize;
  if (ioctl(uffd, UFFDIO_COPY, &copy) == -1) return 4;
  
  ((int(*)())code)();
  return 0;
}
